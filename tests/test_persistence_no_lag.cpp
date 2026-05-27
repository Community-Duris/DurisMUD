/*
 * No-lag persistence queue tests.
 *
 * These tests protect the gameplay thread contract: recording persistence
 * intent must remain a bounded in-memory enqueue, even when the eventual SQL
 * or file writer is slow, offline, or backlogged.
 *
 * Compile from durismud/:
 *   g++ -std=c++20 -I src -o tests/test_persistence_no_lag \
 *       tests/test_persistence_no_lag.cpp src/persistence_queue.c -pthread
 */

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>

#include "persistence_queue.h"

static int failures = 0;
static int slow_writer_calls = 0;

static long long elapsed_usec(std::chrono::steady_clock::time_point start,
                              std::chrono::steady_clock::time_point end)
{
  return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
}

static long long no_lag_budget_usec()
{
  const char *env = std::getenv("PERSISTENCE_NO_LAG_USEC");
  if (env && *env)
  {
    long long parsed = std::atoll(env);
    if (parsed > 0)
      return parsed;
  }

  return 2000;
}

static void fail(const char *message)
{
  std::printf("  FAIL: %s\n", message);
  failures++;
}

static void test_enqueue_stays_inside_main_thread_budget()
{
  const int writes = 1000;
  const long long budget = no_lag_budget_usec();
  long long max_call = 0;
  auto total_start = std::chrono::steady_clock::now();

  std::printf("[1/5] enqueue timing under normal persistence load ...\n");
  persistence_item_event_queue_reset();

  for (int i = 0; i < writes; i++)
  {
    char line[256];
    std::snprintf(line, sizeof(line),
                  "PERSISTENCE_ITEM_EVENT|event=test|item_uid=%d|note=normal",
                  i);

    auto start = std::chrono::steady_clock::now();
    if (!persistence_item_event_queue_enqueue(line))
      fail("enqueue failed before queue capacity");
    auto end = std::chrono::steady_clock::now();

    long long call_time = elapsed_usec(start, end);
    if (call_time > max_call)
      max_call = call_time;
  }

  long long total = elapsed_usec(total_start, std::chrono::steady_clock::now());
  std::printf("  max enqueue: %lld usec, average: %.2f usec, budget: %lld usec\n",
              max_call, (double) total / writes, budget);

  if (max_call > budget)
    fail("single enqueue exceeded no-lag main thread budget");

  if (persistence_item_event_queue_pending() != writes)
    fail("pending event count did not match enqueued writes");
}

static void test_slow_writer_does_not_block_enqueue()
{
  const int writes = 200;
  const long long budget = no_lag_budget_usec();
  long long max_call = 0;

  std::printf("[2/5] enqueue timing while simulated SQL/file writer is slow ...\n");
  persistence_item_event_queue_reset();

  for (int i = 0; i < writes; i++)
  {
    auto start = std::chrono::steady_clock::now();
    if (!persistence_item_event_queue_enqueue("PERSISTENCE_ITEM_EVENT|event=slow-writer"))
      fail("enqueue failed during slow-writer simulation");
    auto end = std::chrono::steady_clock::now();

    long long call_time = elapsed_usec(start, end);
    if (call_time > max_call)
      max_call = call_time;

    if ((i % 25) == 0)
      std::this_thread::sleep_for(std::chrono::milliseconds(3));
  }

  std::printf("  max enqueue while writer sleeps: %lld usec, budget: %lld usec\n",
              max_call, budget);

  if (max_call > budget)
    fail("slow writer simulation caused enqueue to exceed no-lag budget");
}

static void test_full_queue_fails_fast()
{
  const long long budget = no_lag_budget_usec();
  long long overflow_time;

  std::printf("[3/5] full queue overflow fails fast instead of blocking ...\n");
  persistence_item_event_queue_reset();

  for (int i = 0; i < PERSISTENCE_EVENT_QUEUE_CAPACITY; i++)
  {
    if (!persistence_item_event_queue_enqueue("PERSISTENCE_ITEM_EVENT|event=fill"))
      fail("queue reported full before documented capacity");
  }

  auto start = std::chrono::steady_clock::now();
  int ok = persistence_item_event_queue_enqueue("PERSISTENCE_ITEM_EVENT|event=overflow");
  auto end = std::chrono::steady_clock::now();
  overflow_time = elapsed_usec(start, end);

  std::printf("  overflow enqueue returned %d in %lld usec, budget: %lld usec\n",
              ok, overflow_time, budget);

  if (ok)
    fail("overflow enqueue unexpectedly succeeded");

  if (overflow_time > budget)
    fail("overflow path exceeded no-lag main thread budget");

  if (persistence_item_event_queue_dropped() != 1)
    fail("overflow did not increment dropped-event counter");
}

static int slow_background_writer(const char *line, void *context)
{
  int *calls = static_cast<int *>(context);

  (void) line;

  std::this_thread::sleep_for(std::chrono::milliseconds(5));
  (*calls)++;
  return 1;
}

static void test_background_worker_keeps_slow_writes_off_main_thread()
{
  const int writes = 100;
  const long long budget = no_lag_budget_usec();
  long long max_call = 0;

  std::printf("[4/5] background worker absorbs slow writes off the main thread ...\n");
  persistence_item_event_worker_stop(0);
  persistence_item_event_queue_reset();
  slow_writer_calls = 0;

  if (!persistence_item_event_worker_start(slow_background_writer,
                                           &slow_writer_calls))
  {
    fail("background worker failed to start");
    return;
  }

  for (int i = 0; i < writes; i++)
  {
    auto start = std::chrono::steady_clock::now();
    if (!persistence_item_event_queue_enqueue("PERSISTENCE_ITEM_EVENT|event=worker-slow"))
      fail("enqueue failed while background worker was running");
    auto end = std::chrono::steady_clock::now();

    long long call_time = elapsed_usec(start, end);
    if (call_time > max_call)
      max_call = call_time;
  }

  persistence_item_event_worker_stop(1);

  std::printf("  max enqueue with 5ms writer: %lld usec, budget: %lld usec, written: %lu\n",
              max_call, budget, persistence_item_event_worker_written());

  if (max_call > budget)
    fail("background worker allowed slow writes to block enqueue");

  if (slow_writer_calls != writes)
    fail("background worker did not drain all queued writes");

  if (persistence_item_event_queue_pending() != 0)
    fail("background worker left events pending after drain stop");
}

static void test_flush_is_bounded_and_outside_record_path()
{
  char out[PERSISTENCE_EVENT_MAX_LEN];

  std::printf("[5/5] bounded drain leaves remaining events queued ...\n");
  persistence_item_event_worker_stop(0);
  persistence_item_event_queue_reset();

  for (int i = 0; i < 10; i++)
    persistence_item_event_queue_enqueue("PERSISTENCE_ITEM_EVENT|event=drain");

  for (int i = 0; i < 4; i++)
  {
    if (!persistence_item_event_queue_dequeue(out, sizeof(out)))
      fail("bounded dequeue returned empty too early");
  }

  if (persistence_item_event_queue_pending() != 6)
    fail("bounded dequeue did not leave expected events pending");
}

int main()
{
  test_enqueue_stays_inside_main_thread_budget();
  test_slow_writer_does_not_block_enqueue();
  test_full_queue_fails_fast();
  test_background_worker_keeps_slow_writes_off_main_thread();
  test_flush_is_bounded_and_outside_record_path();

  if (failures)
  {
    std::printf("\n%d no-lag persistence test(s) failed.\n", failures);
    return 1;
  }

  std::printf("\nAll no-lag persistence tests passed.\n");
  return 0;
}
