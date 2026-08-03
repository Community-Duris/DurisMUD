#include "timers.h"
#include <stdlib.h>
#include "assocs.h"
#include "epic.h"
#include "ships/ships.h"
#include "sql.h"
#include "utility.h"

extern time_t get_time();

#ifdef __NO_MYSQL__
void set_timer(const char *name) {}

void set_timer(const char *name, int date) {}

int get_timer(const char *name) { return 0; }
#else
void set_timer(const char *name) { set_timer(name, get_time()); }

void set_timer(const char *name, int date)
{
	if (!qry("REPLACE INTO timers (name, date) VALUES ('%s', '%d')", name, date))
		logit(LOG_DEBUG, "set_timer: failed to save timer %s", name ? name : "<null>");
}

int get_timer(const char *name)
{
	if (!qry("SELECT date FROM timers WHERE name = '%s'", name))
	{
		return 0;
	}

	MYSQL_RES *res = mysql_store_result(DB);
	if (!res) {
		logit(LOG_DEBUG, "%s: mysql_store_result failed", __func__);
		return FALSE;
	}

	if (mysql_num_rows(res) < 1)
	{
		mysql_free_result(res);
		return 0;
	}

	MYSQL_ROW row = mysql_fetch_row(res);

	int date = atoi(row[0]);
	mysql_free_result(res);

	return date;
}
#endif

bool has_elapsed(const char *name, int seconds)
{
	int timer = get_timer(name);

	if (get_time() > (timer + seconds))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

void timers_activity()
{
	//  prestige_update();
	zone_trophy_update();
	update_epic_zone_mods();
	cargo_activity();
}
