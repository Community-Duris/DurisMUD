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
static pthread_mutex_t persistence_sql_mutex = PTHREAD_MUTEX_INITIALIZER;
static MYSQL *persistenceDB = NULL;

static MYSQL *sql_persistence_connection(void)
{
	char db_name[50];
	MYSQL *db;
	unsigned int timeout = 10;

	if (persistenceDB && mysql_ping(persistenceDB) == 0)
		return persistenceDB;

	if (persistenceDB)
	{
		mysql_close(persistenceDB);
		persistenceDB = NULL;
	}

	snprintf(db_name, sizeof(db_name), "%s", DB_NAME);
	extern int RUNNING_PORT;
	extern int DFLT_PORT;
	if (RUNNING_PORT != DFLT_PORT)
		snprintf(db_name, sizeof(db_name), "duris_dev");

	db = mysql_init(NULL);
	if (!db)
		return NULL;

	mysql_options(db, MYSQL_OPT_READ_TIMEOUT, &timeout);
	mysql_options(db, MYSQL_OPT_WRITE_TIMEOUT, &timeout);

	if (!mysql_real_connect(db, DB_HOST, DB_USER, DB_PASSWD, db_name, DB_PORT, NULL, CLIENT_MULTI_STATEMENTS))
	{
		mysql_close(db);
		return NULL;
	}

	persistenceDB = db;
	return persistenceDB;
}

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
