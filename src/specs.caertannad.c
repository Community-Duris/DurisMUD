/* Caer tannad shout proc, nice and simple */
#include <ctype.h>
#include <list>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <time.h>
;

#include "prototypes.h"
#include "structs.h"
#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "utils.h"
#include "assocs.h"
#include "damage.h"
#include "graph.h"
#include "justice.h"
#include "reavers.h"
#include "specs.caertannad.h"
#include "specs.prototypes.h"
#include "spells.h"
#include "weather.h"

int caertannad_summon(P_char ch, P_char tch, int cmd, char *arg)
{
	int helpers[] = {78478, 78480, 0};
	if (cmd == -10)
		return TRUE;
	if (!tch && !number(0, 4))
		return shout_and_hunt(ch, 100, "&+CMages! Soldiers! To arms! &+CAnnihilate &=LC%s&n&+C!&n", NULL, helpers, 0, 0);

	return FALSE;
}
