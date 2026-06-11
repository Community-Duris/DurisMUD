/*
 *  utility.h
 *  Duris
 *
 *  Created by Torgal on 1/29/10.
 *
 */

#ifndef _UTILITY_H_
#define _UTILITY_H_

#include "structs.h"

int  GET_LVL_FOR_SKILL(P_char ch, int skill);
bool is_ansi_char(char collor_char);

void connect_rooms(int, int, int, int);
void connect_rooms(int, int, int);

void disconnect_exit(int v1, int dir);
void disconnect_rooms(int v1, int v2);

P_char get_char_online(char *name, bool include_linkdead = TRUE);

void logit(const char *, const char *, ...);
void persistence_alert(int level, const char *domain, const char *owner,
                       const char *item_uid, const char *event_id,
                       const char *action, const char *format, ...);
unsigned long long persistence_next_item_uid(void);
void persistence_assign_item_uid(P_obj obj, const char *reason);
const char *persistence_item_uid_text(P_obj obj, char *buf, int buf_size);
int persistence_write_fallback_event_line(const char *line,
                                          const char *domain,
                                          const char *owner,
                                          const char *action);
void persistence_record_item_event(const char *event_type, P_obj obj,
                                   P_char actor, const char *source,
                                   const char *target, const char *note);
int persistence_flush_item_events(int max_events);
int persistence_replay_fallback_events(void);
int persistence_pending_item_events(void);
unsigned long persistence_dropped_item_events(void);
int persistence_start_item_event_worker(void);
void persistence_stop_item_event_worker(void);
int persistence_item_event_worker_active(void);
int persistence_start_scalar_event_worker(void);
void persistence_stop_scalar_event_worker(void);
int persistence_scalar_event_worker_active(void);
int persistence_start_large_event_worker(void);
void persistence_stop_large_event_worker(void);
int persistence_large_event_worker_active(void);
int persistence_pending_scalar_events(void);
unsigned long persistence_dropped_scalar_events(void);
void utility_latency_dump(void);
void utility_latency_reset(void);
void persistence_schedule_character_save(P_char ch, int type, int delay,
                                         const char *reason);
void persistence_schedule_level_checkpoint(P_char ch, int type, int delay,
                                           const char *reason);

int cmd_from_dir(int dir);
int direction_tag(P_char ch);

const char *condition_str(P_char ch);

string pad_ansi(const char *str, int length, bool trim_to_length = FALSE);
void   trim_and_end_colorless(char *orig, char *good, int length);

P_char get_player_from_name(char *name);
int    get_player_pid_from_name(char *name);
char  *get_player_name_from_pid(int pid);

bool sub_string(const char *, const char *);
bool sub_string_cs(const char *, const char *);
bool sub_string_set(const char *, const char **);

char *coin_stringv(int amount, int padfront = 0);
char *coins_to_string(int platinum, int gold, int silver, int copper, char *color_string);

#endif // _UTILITY_H_
