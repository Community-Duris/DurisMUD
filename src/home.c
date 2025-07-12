/*
Home Construction System,
Copyright 2025, Marissa du Bois
MIT License

This copy is for the exclusive use of Duris: Land of the Bloodlust
*/

#include "utils.h"
#include "home.h"
#include <string.h>

void load_homes();
void save_homes();
void load_home(P_char ch);
void save_home(home_data *home);

void construct_home(P_char ch);
void construct_plot(P_char ch, home_data *home, int plotX, int plotY);

extern void wizlog(int level, const char *format, ...);

void load_homes() {}

void save_homes()
{
	home_data *home = home_list;

	while (home != NULL)
	{
		save_home(home);
		home = home->next_home;
	}
}

void save_home(home_data *home)
{
	if (home)
	{
	}
}

void load_home(P_char ch) {}

void construct_home(P_char ch)
{
	if (!HAS_HOME(ch))
	{
		char buf[MAX_STRING_LENGTH];
		// is allowed to construct home

		// has surveyors mark

		// is current room allowed

		// not in combat
		if (IS_CASTING(ch) || GET_OPPONENT(ch))
			return;

		home_data *new_home = (home_data *)malloc(sizeof(home_data));
		new_home->owner     = ch;
		new_home->exit_to   = ch->in_room;
		new_home->next_home = NULL;

		snprintf(buf, MAX_STRING_LENGTH, "House of %s", GET_NAME(ch));

		new_home->zone                         = (zone_data *)malloc(sizeof(zone_data));
		new_home->zone->number                 = -1; // TODO: Get a valid zone number
		new_home->zone->name                   = buf;
		new_home->zone->filename               = GET_NAME(ch); // TODO: make sure this name is valid.
		new_home->zone->mapx                   = PLOT_SIZE * MAX_PLOTS;
		new_home->zone->mapy                   = PLOT_SIZE * MAX_PLOTS;
		new_home->zone->lifespan_min           = 30;
		new_home->zone->lifespan_max           = 60;
		new_home->zone->fullreset_lifespan_min = 30;
		new_home->zone->fullreset_lifespan_max = 30;
		new_home->zone->difficulty             = 0;
		new_home->zone->flags                  = 0;
		new_home->zone->reset_mode             = 2;
		new_home->zone->top                    = -1;
		new_home->zone->real_top               = -1;
		new_home->zone->real_bottom            = -1;
		new_home->zone->hometown               = 0;
		new_home->zone->avg_mob_level          = 0;

		memset(new_home->plots, 0, sizeof(new_home->plots));

		// link home to character

		// link current room to home with portal or exit

		// construct default plot
		construct_plot(ch, new_home, 0, 0);

		// save home
		save_home(new_home);

		// link home in home list
		if (home_list)
		{
			last_home->next_home = new_home;
			last_home            = new_home;
		}
		else
		{
			home_list = new_home;
			last_home = new_home;
		}

		// send character & pets to new home
	}
}

void construct_plot(P_char ch, home_data *home, int plotX, int plotY)
{
	// owns home?
	if (HAS_HOME(ch) && home)
	{
		// can player build?

		// is plot free
		if (plotX >= MAX_PLOTS || plotY >= MAX_PLOTS || plotX < 0 || plotY < 0)
		{
			wizlog(MINLVLIMMORTAL, "Home plot was out of bounds: %s, (%d/%d)", GET_NAME(ch), plotX, plotY);
			return;
		}

		int plotIndex = plotY * MAX_PLOTS + plotX; // row-major

		// create plot
		if (!home->plots[plotIndex])
		{
			home_plot_data *new_plot = (home_plot_data *)malloc(sizeof(home_plot_data));

			// create rooms

			home->plots[plotIndex] = new_plot;
		}
	}
}