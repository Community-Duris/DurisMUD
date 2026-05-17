#include "structs.h"
#include <stdlib.h>
#include <string.h>

int           mem_use[20];
struct mm_ds *dead_trophy_pool = NULL;

#define GET_BYTE(buf)   (*(char *)((buf)++))
#define GET_SHORT(buf)  getShort(&buf)
#define GET_INTE(buf)   getInt(&buf)
#define GET_LONG(buf)   getLong(&buf)
#define GET_STRING(buf) getString(&buf)
void logit(char const *f, char const *t, ...) {}

void wizlog(char *f, char *t, ...) {}

void obj_to_room(void *object, int room) {}

void send_to_char(const char *txt, P_char ch) {}

int wear(P_char ch, P_obj obj_object, int keyword, bool showit) {}

void writeSavedItem(void *obj) {}

void all_affects(void *ch, int i) {}

P_obj read_object(int number, int type)
{
	P_obj obj;
	obj = (P_obj)malloc(sizeof(struct obj_data));
	memset(obj, 0, sizeof(struct obj_data));
	obj->R_num = number;
	return obj;
}

void obj_to_obj(void *o1, void *o2) {}

void obj_to_char(P_obj obj, P_char ch)
{
	obj->next_content = ch->carrying;
	ch->carrying      = obj;
}

void str_free(char *source)
{
	if (source)
		FREE(source);
}

int GET_LEVEL(P_char ch) { return ch->player.level; }

int vitality_limit(P_char ch) { return ch->points.base_vitality; }

int calculate_mana(P_char ch) { return 1; }

void extract_obj(void *obj, int i) { FREE(obj); }

int flag2idx(int flag)
{
	int i = 0;

	while (flag > 0)
	{
		i++;
		flag >>= 1;
	}
	return i;
}

void update_skills(P_char ch) {}

struct affected_type *get_spell_from_char(P_char ch, int spell, void *context) { return 0; }
