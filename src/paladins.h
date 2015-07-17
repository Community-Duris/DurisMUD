#ifndef __PALADINS_H__
#define __PALADINS_H__

#define IS_PALADIN_SWORD(obj) ( obj && (IS_SET(obj->extra_flags, ITEM_TWOHANDS) || obj->value[0] == WEAPON_2HANDSWORD) )

void event_apply_group_auras(P_char ch, P_char victim, P_obj obj, void *data);
void do_aura(P_char, int);
void purge_linked_auras(P_char ch);
void apply_aura_to_group(P_char ch, int aura);
void apply_aura(P_char ch, P_char victim, int aura);
void aura_broken(struct char_link_data *cld);
void send_paladin_auras(P_char, P_char);
bool has_aura(P_char, int);
const char* aura_name(int);
int aura_mod(P_char ch, int aura_type);
void spell_vortex_of_fear(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj);
bool cleave(P_char ch, P_char victim);
bool is_wielding_paladin_sword(P_char);
void spell_righteous_aura(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj);
void spell_bleak_foeman(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj);
void event_righteous_aura_check(P_char ch, P_char victim, P_obj obj, void *data);
void event_bleak_foeman_check(P_char ch, P_char victim, P_obj obj, void *data);
void spell_dread_blade(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj);
bool dread_blade_proc(P_char ch, P_char victim);
bool holy_weapon_proc(P_char ch, P_char victim);

#endif // __PALADINS_H__

