/* ----------------------------------------------------------
        Object linking to Chars
        This code is written for Duris Mud
        If you are not Duris mud, you don't have my permission
        to run it.  For permission you may contact me directly.

        Dalreth
    ----------------------------------------------------------- */

#include "prototypes.h"
#include "structs.h"
#include "comm.h"
#include "db.h"
#include "events.h"
#include "interp.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

LINKED_OBJECTS::LINKED_OBJECTS(P_obj o, int t, LINKED_OBJECTS *append)
{
	next   = append;
	type   = t;
	object = o;
}

LINKED_OBJECTS::~LINKED_OBJECTS()
{
	next   = NULL;
	type   = 0;
	object = NULL;
}

int LINKED_OBJECTS::Visible_Type()
{
	LINKED_OBJECTS *l = this;
	while (l)
	{
		if (l->type != LINKED_FOLLOWING)
			return l->type;
		l = l->next;
	}
	return 0;
}

char *LINKED_OBJECTS::Visible_Message() { return NULL; }

P_obj LINKED_OBJECTS::Visible_Object()
{
	LINKED_OBJECTS *l = this;
	while (l)
	{
		if (l->type != LINKED_FOLLOWING)
			return l->object;
		l = l->next;
	}
	return NULL;
}

void remove_linked_object(P_obj o)
{
	LINKED_OBJECTS *p = NULL;
	if (!o || !o->hitched_to)
		return;

	LINKED_OBJECTS *l = o->hitched_to->lobj;
	while (l)
	{
		if (l->object == o)
		{
			if (!p)
			{
				l->object->hitched_to->lobj = l->next;
			}
			else
			{
				p->next = l->next;
			}
			delete l;
			break;
		}
		p = l;
		l = l->next;
	}
}

int get_object_link_type(P_obj o) { return LINKED_FOLLOWING; }

bool has_linked_object(P_char ch, P_obj o)
{
	if (!ch || !o)
		return FALSE;
	LINKED_OBJECTS *l = ch->lobj;
	while (l)
	{
		if (l->object == o)
			return TRUE;
		l = l->next;
	}
	return FALSE;
}

void add_linked_object(P_char c, P_obj o, int t)
{
	if (!c || !o || !t)
		return;
	c->lobj       = new LINKED_OBJECTS(o, t, c->lobj);
	o->hitched_to = c;
}

void remove_all_linked_objects(P_char c)
{
	if (!c)
		return;
	while (c->lobj)
		remove_linked_object(c->lobj->object);
}
