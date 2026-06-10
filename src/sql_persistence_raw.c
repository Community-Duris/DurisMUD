/*
 * sql_persistence_raw.c — raw SQL execution on the persistence DB connection.
 * Separated from sql.c to keep that file manageable in size.
 * Called by the large-payload event queue worker thread.
 *
 * NOTE: Only compiles when __NO_MYSQL__ is NOT defined. The __NO_MYSQL__ stub
 * for sql_persistence_execute_raw() lives in sql.c alongside the other stubs.
 */

#include "prototypes.h"
#include "structs.h"
#include "utils.h"
#include "sql.h"
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#ifndef __NO_MYSQL__

#include <mysql.h>

extern MYSQL *DB;

/* Connection state is owned by sql.c; we just use it here. */
extern MYSQL *persistenceDB;
extern pthread_mutex_t persistence_sql_mutex;

bool sql_persistence_execute_raw(const char *sql)
{
	MYSQL *db;
	int ret;

	if (!sql || !*sql)
		return FALSE;

	pthread_mutex_lock(&persistence_sql_mutex);
	db = sql_persistence_connection();
	if (!db)
	{
		pthread_mutex_unlock(&persistence_sql_mutex);
		logit(LOG_DEBUG, "Persistence MySQL: sql_persistence_execute_raw() failed - no connection");
		return FALSE;
	}

	ret = mysql_real_query(db, sql, strlen(sql));
	if (ret)
	{
		logit(LOG_DEBUG, "Persistence MySQL error in sql_persistence_execute_raw(): %s", mysql_error(db));
		logit(LOG_DEBUG, "Persistence MySQL failed query (first 200 chars): %.200s", sql);
	}

	pthread_mutex_unlock(&persistence_sql_mutex);
	return ret == 0;
}
#endif /* __NO_MYSQL__ */
