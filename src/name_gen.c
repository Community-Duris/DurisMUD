
/****************************************************************************/
/* NAMN.C - Create random names.                                            */
/****************************************************************************/
/* Johan Danforth                                                           */
/****************************************************************************/

/****************************************************************************
 * Modified by Kvark (June 2003)
 *
 * Made for durismud.
 ****************************************************************************/

#include "prototypes.h"
#include "structs.h"
#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "arena.h"
#include "arenadef.h"
#include "justice.h"
#include "mm.h"
#include "objmisc.h"
#include "spells.h"
#include "weather.h"
/****************************************************************************/
/* Compile Time parameters                                                  */
/****************************************************************************/

#define SYLLABLES_PER_SECTION 100
#define SYLLABLE_LENGTH       100
#define NAME_LENGTH           20

/****************************************************************************/
/* prototypes                                                               */
/****************************************************************************/

int get_name(char return_name[256]);

/****************************************************************************/
/* main                                                                     */
/****************************************************************************/
int get_name(char return_namn[256])
{
	time_t t;
	int    loop;
	int    antal_start = 0;
	int    antal_mitt  = 0;
	int    antal_slut  = 0;
	char   tempstring[151];
	tempstring[0] = '\0';
	char  filnamn[256];
	char  start[SYLLABLES_PER_SECTION][SYLLABLE_LENGTH]; /* start syllable               */
	char  mitt[SYLLABLES_PER_SECTION][SYLLABLE_LENGTH];  /* middle syllable              */
	char  slut[SYLLABLES_PER_SECTION][SYLLABLE_LENGTH];  /* ending syllable              */
	char  namn[NAME_LENGTH];                             /* name                         */
	FILE *infil;
	int   cgi = 0;
	int   SEX = -1;

	memset(start, 0, SYLLABLES_PER_SECTION * SYLLABLE_LENGTH);
	memset(mitt, 0, SYLLABLES_PER_SECTION * SYLLABLE_LENGTH);
	memset(slut, 0, SYLLABLES_PER_SECTION * SYLLABLE_LENGTH);
	memset(namn, 0, NAME_LENGTH);

	SEX = number(0, 9);
	switch (SEX)
	{
		case 0:
			infil = fopen("lib/misc/names/f_male.nam", "r");
			break;
		case 1:
			infil = fopen("lib/misc/names/f_female.nam", "r");
			break;
		case 2:
			infil = fopen("lib/misc/names/ALVER.NAM", "r");
			break;
		case 3:
			infil = fopen("lib/misc/names/DEVERRY2.NAM", "r");
			break;
		case 4:
			infil = fopen("lib/misc/names/gnome2.nam", "r");
			break;
		case 5:
			infil = fopen("lib/misc/names/kender1.nam", "r");
			break;
		case 6:
			infil = fopen("lib/misc/names/orc.nam", "r");
			break;
		case 7:
			infil = fopen("lib/misc/names/DVARGAR.NAM", "r");
			break;
		case 8:
			infil = fopen("lib/misc/names/HOBER.NAM", "r");
			break;
		case 9:
			infil = fopen("lib/misc/names/kerrel.nam", "r");
			break;
	}

	if (infil == NULL)
	{
		printf("Cant open name file");
		snprintf(return_namn, 256, "Cant locate name file");
		return 0;
		/* print the name               */
	}
	/* read file until [startstav] it found (starting syllable)                 */
	while (strcmp(tempstring, "[startstav]") != 0)
	{
		memset(tempstring, 0, sizeof(tempstring));
		if (fgets(tempstring, 150, infil) == NULL)
			break;
		size_t len = strlen(tempstring);
		if (len >= 2 && tempstring[len - 2] == '\r')
			tempstring[len - 2] = '\0';
		else if (len >= 1)
			tempstring[len - 1] = '\0'; /* remove linefeed          */
	}
	/* read file until [mittstav] is found (middle syllable)                    */
	while (strcmp(tempstring, "[mittstav]") != 0)
	{
		memset(tempstring, 0, sizeof(tempstring));
		if (fgets(tempstring, 150, infil) == NULL)
			break;
		size_t len = strlen(tempstring);
		if (len >= 2 && tempstring[len - 2] == '\r')
			tempstring[len - 2] = '\0';
		else if (len >= 1)
			tempstring[len - 1] = '\0'; /* remove linefeed          */
		if ((tempstring[0] != '/') && (tempstring[0] != '['))
		{
			strncpy(start[antal_start], tempstring, strlen(tempstring));
			antal_start++;
		}
	}
	/* read file until [slutstav] is found (ending syllable)                    */
	while (strcmp(tempstring, "[slutstav]") != 0)
	{
		memset(tempstring, 0, sizeof(tempstring));
		if (fgets(tempstring, 150, infil) == NULL)
			break;
		size_t len = strlen(tempstring);
		if (len >= 2 && tempstring[len - 2] == '\r')
			tempstring[len - 2] = '\0';
		else if (len >= 1)
			tempstring[len - 1] = '\0'; /* remove linefeed          */
		if ((tempstring[0] != '/') && (tempstring[0] != '['))
		{
			strncpy(mitt[antal_mitt], tempstring, strlen(tempstring));
			antal_mitt++;
		}
	}
	/* read file until [stop] is found (end of syllables)                       */
	while (strcmp(tempstring, "[stop]") != 0)
	{
		memset(tempstring, 0, sizeof(tempstring));
		if (fgets(tempstring, 150, infil) == NULL)
			break;
		size_t len = strlen(tempstring);
		if (len >= 2 && tempstring[len - 2] == '\r')
			tempstring[len - 2] = '\0';
		else if (len >= 1)
			tempstring[len - 1] = '\0'; /* remove linefeed          */
		if ((tempstring[0] != '/') && (tempstring[0] != '['))
		{
			strncpy(slut[antal_slut], tempstring, strlen(tempstring));
			antal_slut++;
		}
	}
	fclose(infil);

	/* correct the nr of available syllables...                                 */

	antal_start--;
	antal_mitt--;
	antal_slut--;

	for (loop = 0; loop < number(1, 15); loop++) /* loop through nr of names   */
	{
		strcpy(namn, start[number(0, antal_start - 1)]); /* get a start                  */
		strcat(namn, mitt[number(0, antal_mitt - 1)]);   /* get a middle                 */
		strcat(namn, slut[number(0, antal_slut - 1)]);   /* get an ending                */
		snprintf(return_namn, MAX_STRING_LENGTH, "%s", namn);
	}

	return (SEX);
}
