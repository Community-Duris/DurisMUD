/*
***************************************************************************
 *  File: prototypes.h                                       Part of Duris *
 *  Usage: Prototypes for all functions.                                     *
 *  Copyright  1995 - Duris Systems Ltd.                                   *
 *************************************************************************** */

#ifndef _SOJ_PROTOTYPES_H_
#define _SOJ_PROTOTYPES_H_

#ifndef _SOJ_CONFIG_H_
#include "config.h"
#endif

#ifndef _SOJ_STRUCTS_H_
#include "structs.h"
#endif

#include "mail.h"
#include "account.h"
#include "new_combat_def.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>
using namespace std;

// The below line will abuse player times in game, and use it to eq-wipe every player in the game.
#define EQ_WIPE 20000000

/* global strings, these should only be used sparingly, by functions that
   build and return a string pointer.  I'm adding first to support where_obj(),
   add others if you need to, and you need more than one.  JAB */

extern char GS_buf1[MAX_STRING_LENGTH];

int new_descriptor(int s, bool ssl);
int new_connection(int s, bool ssl);

/* affects.c */
void event_short_affect(P_char, P_char , P_obj , void *);
void remove_link(P_char ch, struct char_link_data *clda);
void event_obj_affect(P_char, P_char, P_obj obj, void *af);
void initialize_links();
void linked_affect_to_char(P_char ch, struct affected_type *af, P_char source, int type);
void linked_affect_to_char_obj(P_char ch, struct affected_type *af, P_obj obj, int type);
void set_short_affected_by(P_char ch, int spell, int duration);
void set_obj_affected_extra(P_obj obj, int time, sh_int spell, sh_int data, ulong extra2);
struct obj_affect *get_spell_from_obj(P_obj, int);
void make_dry(P_char);
bool make_wet(P_char, int);
bool affect_timer(P_char, int, int);
void strip_holy_sword( P_char );

// account.c
//
void checkHatred(P_char);
void select_accountname(P_desc, char *);
void get_account_password(P_desc, char *);
void display_account_menu(P_desc, char *);
void confirm_account(P_desc, char *);
void verify_account_name(P_desc, char *);
void get_new_account_email(P_desc, char *);
void verify_new_account_email(P_desc, char *);
void get_new_account_password(P_desc, char *);
void verify_new_account_password(P_desc, char *);
void verify_new_account_information(P_desc, char *);
void account_select_char(P_desc, char *);
void account_confirm_char(P_desc, char *);
void account_new_char(P_desc, char *);
void account_delete_char(P_desc, char *);
void account_display_info(P_desc, char *);
void delete_account(P_desc, char *);
void verify_delete_account(P_desc, char *);
void generate_account_confirmation_code(P_desc, char *);
void display_account_information(P_desc d);
void clear_account(P_acct);
P_acct free_account(P_acct);
P_acct allocate_account(void);
void add_account_to_list(P_acct);
void remove_account_from_list(P_acct);
char* check_and_clear(char *);
char is_account_confirmed(P_desc);
void write_unique_ip(P_acct, FILE *);
void read_unique_ip(P_acct, FILE *);
void write_character_list(P_acct, FILE *);
void read_character_list(P_acct, FILE *);
void update_account_iplist(P_desc);
void update_character_list(P_desc, char *);
void add_ip_entry(P_acct, P_desc);
struct acct_ip *find_ip_entry(P_acct, P_desc);
int can_connect(struct acct_chars *, P_desc);
int is_char_in_game(struct acct_chars *, P_desc);
struct acct_chars *find_char_in_list(struct acct_chars *, char *);
P_char load_char_into_game(struct acct_chars *, P_desc);
void account_new_char_name(P_desc, char *);
void display_character_list(P_desc);
void display_delete_character_list(P_desc);
void add_char_to_account(P_desc);
void remove_char_from_list(P_acct, char *);
int write_account(P_acct);
int read_account(P_acct);

/* actcomm.c */

#ifdef OVL
void update_ovl(void);
#endif
void process_with_paging(P_char ch, char *comm);
bool can_talk(P_char);
bool is_overload(P_char);
bool is_silent(P_char, bool);
char *fread_action(FILE *);
int action(int);
int find_action(int);
void boot_social_messages(void);
void check_magic_doors(P_char, const char *);
void do_action(P_char, char *, int);
void do_ask(P_char, char *, int);
void do_channel(P_char, char *, int);
void do_gossip(P_char, char *, int);
void do_insult(P_char, char *, int);
/*bool CAN_GCC(P_char);*/
void do_gcc(P_char, char *, int);
void send_to_guild(P_Guild, char *, char *);
void do_project(P_char, char *, int);
void do_page(P_char, char *, int);
void do_petition(P_char, char *, int);
void do_pose(P_char, char *, int);
int say(P_char, const char *);
void do_say(P_char, char *, int);
void do_shout(P_char, char *, int);
void do_tell(P_char, char *, int);
void do_beep(P_char, char *, int);
void do_reply(P_char, char *, int);
void do_rwc(P_char, char *, int);
void do_whisper(P_char, char *, int);
void do_write(P_char, char *, int);
void do_yell(P_char, char *, int);
void mobsay(P_char, const char *);
void send_to_gods(char *);

/* actinf.c */

const char *get_class_name(P_char ch, P_char tch);
P_obj get_object_in_equip(P_char, char *, int *);
P_obj get_object_in_equip_vis(P_char, char *, int *);
char *find_ex_description(char *, struct extra_descr_data *);
char *show_obj_to_char(P_obj, P_char, int, bool);
void show_char_to_char(P_char, P_char, int);
const char *ac_to_string(int);
const char *align_to_string(int);
const char *class_to_string(P_char, char*);
//const char *exp_to_string(P_char);
const char *hitdam_roll_to_string(int);
const char *load_to_string(P_char);
const char *race_to_string(P_char);
const char *prestige_to_string(P_char);
string save_to_string(int);
const char *stat_to_string1(int);
const char *stat_to_string2(int);
const char *stat_to_string3(int);
const char *stat_to_string_damage_pulse(float);
const char *stat_to_string_spell_pulse(float);
void ShowCharSpellBookSpells(P_char, P_obj, char *);
void argument_split_2(char *, char *, char *);
void do_attributes(P_char, char *, int);
void do_consider(P_char, char *, int);
void do_credits(P_char, char *, int);
void do_map(P_char, char *, int);
void do_display(P_char, char *, int);
void do_do(P_char, char *, int);
void do_equipment(P_char, char *, int);
void do_examine(P_char, char *, int);
void do_exits(P_char, char *, int);
void do_projects(P_char, char *, int);
void do_faq(P_char, char *, int);
void do_glance(P_char, char *, int);
void do_help(P_char, char *, int);
void load_cmd_attributes();
void do_ignore(P_char, char *, int);
void do_info(P_char, char *, int);
void do_inventory(P_char, char *, int);
void do_levels(P_char, char *, int);
void do_look(P_char, char *, int);
void do_motd(P_char, char *, int);
void do_news(P_char, char *, int);
void do_cheaters(P_char, char *, int);
void do_ok(P_char, char *, int);
void do_read(P_char, char *, int);
void do_report(P_char, char *, int);
void do_rules(P_char, char *, int);
void do_score(P_char, char *, int);
void do_target(P_char, char *, int);
void do_order_target(P_char, P_char, char *, int);
void do_time(P_char, char *, int);
void do_punish(P_char, char *arg, int);
void do_title(P_char, char *arg, int);
void do_users(P_char, char *, int);
void do_weather(P_char, char *, int);
void do_who(P_char, char *, int);
void do_wizhelp(P_char, char *, int);
void do_wizlist(P_char, char *, int);
void do_world(P_char, char *, int);
void list_char_to_char(P_char, P_char, int);
void list_obj_to_char(P_obj, P_char, int, bool);
void new_look(P_char, char *, int, int);
void show_exits_to_char(P_char, int, int);
void list_scanned_chars(P_char, P_char, int, int);
void do_scan(P_char, char *, int);
void web_info(void);
void do_artireset(P_char, char *arg, int);
bool hasRequiredSlots(P_char);
void do_raid(P_char, char*, int);
void do_recall(P_char, char*, int);
void display_room_auras(P_char, int);
//void do_resetspec(P_char, char *, int);
/* actmove.c */

char *enter_message(P_char, P_char, int, char *, int, int);
char *leave_message(P_char, P_char, int, char *);
P_obj has_key(P_char, int);
int can_enter_room(P_char, int, int);
int room_crowded(P_char, int);
int do_simple_move(P_char, int, unsigned int);
int find_door(P_char, char *, char *);
int leave_by_exit(P_char, int);
int load_modifier(P_char);
void blow_char_somewhere_else(P_char, int);
void SwapCharsInList(P_char, P_char);
void do_alert(P_char, char *, int);
void do_close(P_char, char *, int);
void do_drag(P_char, char *, int);
void do_enter(P_char, char *, int);
void do_follow(P_char, char *, int);
void do_kneel(P_char, char *, int);
void do_lock(P_char, char *, int);
void do_move(P_char, char *, int);
void do_open(P_char, char *, int);
void do_terrain(P_char, char *, int);
void do_pick(P_char, char *, int);
void do_recline(P_char, char *, int);
void do_rest(P_char, char *, int);
void do_sit(P_char, char *, int);
void do_sleep(P_char, char *, int);
void do_stand(P_char, char *, int);
void do_unlock(P_char, char *, int);
void do_wake(P_char, char *, int);
void do_tupor(P_char, char *, int);

/* actnew.c */

void do_flurry_of_blows(P_char, char *);
void do_arena(P_char, char *, int);
void do_aggr(P_char, char *, int);
void do_commands(P_char, char *, int);
void do_consent(P_char, char *, int);
void do_disarm(P_char, char *, int);
void do_dodge(P_char, char *, int);
void do_gsay(P_char, char *, int);
void do_hitall(P_char, char *, int);
void do_war_cry(P_char, char *, int);
int do_roar_of_heroes(P_char);
void do_tackle(P_char, char *, int);
void do_legsweep(P_char, char *, int);
void do_grapple(P_char, char *, int);
void do_meditate(P_char, char *, int);
void stop_meditation(P_char);
void do_more(P_char, char *, int);
void do_nokill(P_char, char *, int);
void do_shapechange(P_char, char *, int);
void do_subterfuge(P_char, char *, int);
void do_trip(P_char, char *, int);
void do_dirttoss(P_char, char *, int);
void do_trap(P_char, char *, int);
P_char morph(P_char, int, int);
P_char un_morph(P_char);
void do_stampede(P_char, char *, int);
void do_charge(P_char, char *, int);
void do_lore(P_char, char *, int);
void do_craft(P_char, char *, int);
void do_make(P_char, char *, int);
void make_lock(P_char, char *);
void make_key(P_char, char *);
void do_throat_crush(P_char, char *, int);

void do_testdesc(P_char, char *, int);
void do_introduce(P_char, char *, int);

void do_hamstring(P_char, char *, int);
void do_vote(P_char, char *, int);
void do_home(P_char, char *, int);

void do_throw_potion(P_char, char *, int);
bool throw_potion(P_char, P_obj, P_char, P_obj);
int chance_throw_potion(P_char, P_char);


void do_thrust(P_char, char *, int);
void do_unthrust(P_char, char *, int);
P_obj create_thrust_item(P_obj original);

void event_living_stone_death(P_char ch, P_char victim, P_obj obj, void *data);

void do_offensive(P_char, char *, int);

/* actobj.c */

int wield_item_size(P_char ch, P_obj obj);
int get_numb_free_hands(P_char);
bool put(P_char, P_obj, P_obj, int);
int wear(P_char, P_obj, int, bool);
int remove_item(P_char, P_obj, int);
int remove_and_wear(P_char, P_obj, int, int, int, int);
bool find_chance(P_char);
bool is_salvageable(P_obj);
void do_drink(P_char, char *, int);
void do_drop(P_char, char *, int);
void do_eat(P_char, char *, int);
void do_empty(P_char, char *, int);
void do_fill(P_char, char *, int);
void do_get(P_char, char *, int);
void do_give(P_char, char *, int);
void do_grab(P_char, char *, int);
void do_junk(P_char, char *, int);
void do_pour(P_char, char *, int);
void do_put(P_char, char *, int);
void do_remove(P_char, char *, int);
void do_salvage(P_char, char *, int);
void do_search(P_char, char *, int);
void do_sip(P_char, char *, int);
void do_taste(P_char, char *, int);
void do_wear(P_char, char *, int);
void do_wield(P_char, char *, int);
void get(P_char, P_obj, P_obj, int);
void name_from_drinkcon(P_obj);
void name_to_drinkcon(P_obj, int);
void perform_wear(P_char, P_obj, int);
void weight_change_object(P_obj, int);
void do_apply_poison(P_char, char *, int);
void do_dropalldot(P_char, char *, int);
bool is_stat_max(sbyte);

/* tradeskill.c */
void do_salvation(P_char ch, char *arg, int cmd);
void do_drandebug(P_char ch, char *arg, int cmd);
int get_matstart(P_obj obj);
bool has_affect(P_obj obj);
void do_refine(P_char ch, char *arg, int cmd);
int itemvalue(P_obj obj);
void do_dice(P_char ch, char *arg, int cmd);

/* actoff.c */
bool CheckMultiProcTiming(P_char);
bool single_stab(P_char ch, P_char victim, P_obj weapon);
bool flank(P_char ch, P_char victim);
bool circle(P_char ch, P_char victim);
bool bad_flee_dir(const P_char, const int);
P_char parse_victim(P_char, char *, unsigned int);
P_char ParseTarget(P_char, char *);
bool instantkill(P_char, P_char);
bool is_in_safe(P_char ch);
bool isBashable(P_char ch, P_char victim, bool ignoreImmaterial = FALSE);
char isSpringable(P_char, P_char);
bool MobShouldFlee(P_char);
bool nokill(P_char, P_char);
bool should_not_kill(P_char, P_char);
void attack(P_char, P_char);
bool backstab(P_char, P_char);
void bash(P_char ch, P_char victim, bool _debug = FALSE);
void bodyslam(P_char, P_char);
void buck(P_char);
void do_dreadnaught(P_char, char *, int);
void do_shadowstep(P_char, char *, int);
void parlay(P_char, P_char);
void do_assist(P_char, char *, int);
void do_backstab(P_char, char *, int);
void do_bash(P_char, char *, int);
void do_garrote(P_char, char *, int);
void do_combination(P_char, char *, int);
void do_barrage(P_char, char *, int);
void do_buck(P_char, char *, int);
void do_circle(P_char, char *, int);
void do_defend(P_char, char *, int);
void do_disengage(P_char, char *, int);
void do_flee(P_char, char *, int);
void do_riff(P_char, char *, int);
void do_guard(P_char, char *, int);
void do_headbutt(P_char, char *, int);
void do_hit(P_char, char *, int);
void do_kick(P_char, char *, int);
void do_maul(P_char, char *, int);
void do_restrain(P_char, char *, int);
void do_roundkick(P_char, char *, int);
void do_kill(P_char, char *, int);
void do_murde(P_char, char *, int);
void do_murder(P_char, char *, int);
void do_order(P_char, char *, int);
void do_parlay(P_char, char *, int);
void do_shieldpunch(P_char, char *, int);
void do_sweeping_thrust(P_char, char *, int);
void do_rescue(P_char, char *, int);
void do_retreat(P_char, char *, int);
void flee_lose_exp(P_char, P_char);
void maul(P_char, P_char);
void restrain(P_char, P_char);
void rescue(P_char, P_char, bool);
void do_trample(P_char, char *, int);
float takedown_check(P_char, P_char, float, int, ulong);
void do_rearkick(P_char, char *, int);
void do_whirlwind(P_char, char *, int);
void kick(P_char, P_char);
bool roundkick(P_char, P_char);
int chance_kick(P_char, P_char);
int chance_roundkick(P_char, P_char);
void rush(P_char, P_char);
void do_rush(P_char, char *, int);
void do_flank(P_char, char *, int);
void do_battle_orders(P_char, char *, int);
void battle_orders(P_char, P_char);
void do_call_grave(P_char, char *, int);
void do_fade(P_char, char *, int);
void event_call_grave(P_char, P_char, P_obj, void *);
void event_call_grave_target(P_char, P_char, P_obj, void *);
void event_bye_grave(P_char, P_char, P_obj, void *);
void do_gaze(P_char, char *, int);
void do_consume(P_char, char *, int);
void gaze(P_char, P_char);
void do_sneaky_strike(P_char, char *, int);
void sneaky_strike(P_char, P_char);
void do_mug(P_char, char *, int);
void mug(P_char, P_char);
bool is_preparing_for_sneaky_strike(P_char);
void event_sneaky_strike(P_char, P_char, P_obj, void *);
void do_shriek(P_char, char *, int);
bool isSweepable(P_char, P_char);
bool isKickable(P_char, P_char);
bool isMaulable(P_char, P_char);
double orc_horde_dam_modifier(P_char, double, int);
bool check_crippling_strike( P_char ch );
void event_combination(P_char ch, P_char victim, P_obj obj, void *data);

/* actoth.c */

void do_area(P_char, char *, int);
void berserk(P_char ch, int duration);
void do_multiclass(P_char, char *, int);
void duergar_enlarge(int, P_char);
void do_disappear(P_char, char *, int);
void goblin_reduce(int, P_char);
int test_atm_present(P_char);
void try_to_bury(P_char, P_obj);
void try_to_hide(P_char, P_obj);
void do_balance(P_char, char *, int);
void do_berserk(P_char, char *, int);
void do_bodyslam(P_char, char *, int);
void do_breathe(P_char, char *, int);
void do_bug(P_char, char *, int);
void do_cheat(P_char, char *, int);
void do_bury(P_char, char *, int);
void do_dig(P_char, char *, int);
void do_mine(P_char, char *, int);
void do_fish(P_char, char *, int);

void do_exhume(P_char, char *, int);
void do_spawn(P_char, char *, int);
void do_summon_host(P_char, char *, int);
void do_camp(P_char, char *arg, int);
void do_climb(P_char, char *arg, int);
void do_deposit(P_char, char *, int);
void do_description(P_char, char *, int);
void do_donate(P_char, char *, int);
void do_doorbash(P_char, char *, int);
void do_doorkick(P_char, char *, int);
void do_explist(P_char, char *, int);
void do_expkkk(P_char, char *, int);  // Arih: for debugging exp bug
void do_fly(P_char, char *, int);
void do_forage(P_char, char *, int);
void do_githyanki_primeshift(P_char ch);
void do_hide(P_char, char *, int);
void do_idea(P_char, char *, int);
void do_innate(P_char, char *, int);
void do_listen(P_char, char *, int);
void do_no_buy(P_char, char *, int);
void do_not_here(P_char, char *, int);
void do_paladin_aura(P_char, P_char);
void do_quaff(P_char, char *, int);
void do_qui(P_char, char *, int);
void do_quit(P_char, char *, int);
void do_recite(P_char, char *, int);
void do_rub(P_char, char *, int);
void do_save(P_char, char *, int);
void do_save_silent(P_char, int);
void do_sneak(P_char, char *, int);
void do_split(P_char, char *, int);
void do_steal(P_char, char *, int);
void do_stomp(P_char, char *, int);
void do_sweep(P_char, char *, int);
void do_swim(P_char, char *, int);
void do_testcolor(P_char, char *, int);
void do_toggle(P_char, char *, int);
void do_typo(P_char, char *, int);
void do_use(P_char, char *, int);
void do_withdraw(P_char, char *, int);
void halfling_stealaction(P_char, char *, int);
void racial_strength(P_char);
void reward_for_bury(P_char ch, int gold);
void show_toggles(P_char);
void try_to_donate(P_char, P_obj);
void do_illithid_planeshift (P_char);
void disease_check(P_char);
void disease_add(P_char, int);
void do_suicide(P_char, char *, int);
void do_innate_throw_lightning(P_char ch);
void do_innate_anti_evil(P_char, P_char);
void do_innate_embrace_death(P_char);
void do_rage(P_char, char *, int);
void do_rampage(P_char, char *, int);
void do_infuriate(P_char, char *, int);
P_obj blood_in_room_with_me(P_char);
void do_lick(P_char, char *, int);
void do_nothing(P_char, char *, int);
void do_blood_scent(P_char, char *, int);
void do_branch(P_char, char*, int);
void branch(P_char, P_char);
void bite(P_char, P_char);
void webwrap(P_char, P_char);
void do_summon_imp(P_char, char*, int);
void innate_gaze(P_char, P_char);
/* actset.c */

char *setbit_parseArgument(char *, char *);
void do_setbit(P_char, char *, int);

/* actwiz.c */

void sprintbitde(ulong, const flagDef[], char *);
char *comma_string(long);
void GetMIA(char *, char *);
char *where_obj(P_obj, int);
int gr_idiotproof(P_char, P_char, char *, int);
struct obj_data *clone_obj(P_obj);
void NewbySkillSet(P_char, bool);
void clone_container_obj(P_obj, P_obj);
void stat_game(P_char);
void do_offlinemsg(P_char, char *, int);
void do_reload_help(P_char, char *, int);
void do_tedit(P_char, char *, int);
void do_depiss(P_char, char *, int);
void do_repiss(P_char, char *, int);
void do_approve(P_char, char *, int);
void do_advance(P_char, char *, int);
void do_affectpurge(P_char, char *, int);
void do_allow(P_char, char *, int);
void do_at(P_char, char *, int);
void do_ban(P_char, char *, int);
void do_wizhost(P_char, char *, int);
void do_clone(P_char, char *, int);
void do_deathobj(P_char, char *, int);
void do_decline(P_char, char *, int);
void do_demote(P_char, char *, int);
void do_echo(P_char, char *, int);
void do_echoa(P_char, char *, int);
void do_echou(P_char, char *, int);
void do_echoe(P_char, char *, int);
void do_echot(P_char, char *, int);
void do_echog(P_char, char *, int);
void do_echoz(P_char, char *, int);
void do_emote(P_char, char *, int);
void do_finger(P_char, char *, int);
void do_force(P_char, char *, int);
void do_freeze(P_char, char *, int);
void do_goto(P_char, char *, int);
void do_grant(P_char, char *, int);
void do_inroom(P_char, char *, int);
void do_ingame(P_char, char *, int);
void do_invite(P_char, char *, int);
void do_knock(P_char, char *, int);
void do_lag(P_char, char *, int);
void do_law_flags(P_char, char *, int);
void do_list_witness(P_char, char *, int);
void do_load(P_char, char *, int);
void do_lookup(P_char, char *, int);
void do_make_guide(P_char ch, char *argument, int cmd);
void do_newbie(P_char ch, char *argument, int cmd);
void do_poofIn(P_char, char *, int);
void do_poofOut(P_char, char *, int);
void do_poofInSound(P_char, char *, int);
void do_poofOutSound(P_char, char *, int);
void do_ptell(P_char, char *, int);
void do_purge(P_char, char *, int);
void do_questwhere(P_char, char *, int);
void do_reinitphys(P_char, char *, int);
void do_release(P_char, char *, int);
void do_reroll(P_char, char *, int);
void do_restore(P_char, char *, int);
void do_return(P_char, char *, int);
void do_revoke(P_char, char *, int);
/*void do_revoketitle(P_char, char *, int);*/
void do_secret(P_char, char *, int);
void do_setattr(P_char, char *, int);
void do_sethome(P_char, char *, int);
void revert_sethome(P_char);
void do_shutdow(P_char, char *, int);
void do_shutdown(P_char, char *, int);
void write_shutdown_info(const char *immortal_name, const char *reason);
void do_silence(P_char, char *, int);
void do_snoop(P_char, char *, int);
void do_start(P_char, int);
void do_stat(P_char, char *, int);
void do_switch(P_char, char *, int);
void do_teleport(P_char, char *, int);
void do_text_reload(P_char, char *, int);
void do_trans(P_char, char *, int);
void do_uninvite(P_char, char*, int);
void do_vis(P_char, char *, int);
void do_where(P_char, char *, int);
void do_which(P_char ch, char *args, int cmd);
void do_whois(P_char ch, char *args, int cmd);
void do_wizlock(P_char, char *, int);
void do_wizmsg(P_char, char *, int);
void do_zreset(P_char, char *, int);
void read_wizconnect_file(void);
void read_ban_file(void);
void roll_basic_attributes(P_char, int);
void sa_ageCopy(P_char, ulong, int);
void sa_byteCopy(P_char, ulong, int);
void sa_intCopy(P_char, ulong, int);
void sa_shortCopy(P_char, ulong, int);
void save_ban_file(void);
void save_wizconnect_file(void);
void do_terminate(P_char, char *, int);
void do_sacrifice(P_char, char *, int);
void do_unspec(P_char, char*, int);
void test_load_all_chars(P_char);
void do_read_player(P_char, char *, int);
void do_nchat(P_char, char *, int);
void do_tranquilize(P_char ch, char *argument, int cmd);
void do_storage(P_char ch, char *arg, int cmd);
void do_newb_spellup_all(P_char ch, char *arg, int cmd);
void do_newb_spellup(P_char ch, char *arg, int cmd);
void do_givepet(P_char ch, char *arg, int cmd);
void do_petition_block(P_char, char *, int);
void concat_which_flagsde(const char *flagType, const flagDef flagNames[], char *buf);

/* artifact.c */
void addOnGroundArtis_sql();
void addOnMobArtis_sql();
void artifact_feed_blood_sql(P_char ch, P_obj arti, int frag_gain);
void artifact_feed_sql(P_char owner, P_obj arti, int feed_seconds, bool soulCheck = FALSE);
void artifact_feed_to_min_sql( P_obj arti, int min_minutes );
void artifact_timer_sql( int vnum, char *buffer );
void artifact_update_location_sql( P_obj arti );
void do_artifact_sql(P_char, char *, int);
void event_artifact_check_bind_sql( P_char ch, P_char vict, P_obj obj, void * arg );
void event_artifact_check_poof_sql( P_char ch, P_char vict, P_obj obj, void * arg );
void event_artifact_wars_sql(P_char, P_char, P_obj, void*);
bool get_artifact_data_sql( int vnum, P_arti artidata );
bool remove_owned_artifact_sql( P_obj arti, int pid = -1 );
void remove_all_artifacts_sql( P_char ch );
void setupMortArtiList_sql(void);

/* artifact_old.c */
void UpdateArtiBlood(P_char, P_obj, int);
void setupMortArtiList(void);
bool add_owned_artifact(P_obj, P_char, long unsigned);
int remove_owned_artifact(P_obj, P_char, int);
int get_current_artifact_info(int, int, char *, int *, time_t *, int *, int, time_t *);
void do_artifact(P_char, char *, int);
void artifact_feed_to_min( P_obj arti, int min_minutes );
void feed_artifact(P_char ch, P_obj obj, int feed_seconds, int bypass);
void artifact_switch_check(P_char ch, P_obj obj);
void event_check_arti_poof( P_char ch, P_char vict, P_obj obj, void * arg );
void event_artifact_wars(P_char, P_char, P_obj, void*);
void dropped_arti_hunt();
void save_artifact_data( P_char );
// automatic_rules.c
int is_Raidable(P_char, char *, int);
void removeArtiData( char *name );

/* bard.c */

//bool SINGING(P_char); <-- Turned into macro shown below.
#define SINGING(ch) (IS_ALIVE(ch) && IS_AFFECTED3(ch, AFF3_SINGING))

P_obj has_instrument(P_char);
void do_play(P_char, char *, int);
void stop_singing(P_char);
void do_wail(P_char, char *, int);
void spell_shatter(int, P_char, char*, int, P_char, P_obj);

/* beh_magic.c */

void spell_beholder_sleep(int, P_char, P_char, P_obj);
void spell_beholder_paralyze(int, P_char, P_char, P_obj);
void spell_beholder_disintegrate(int, P_char, P_char, P_obj);
void spell_beholder_fear(int, P_char, P_char, P_obj);
void spell_beholder_slowness(int, P_char, P_char, P_obj);
void spell_beholder_damage(int, P_char, P_char, P_obj);
void spell_beholder_telekinesis(int, P_char, P_char, P_obj);
void spell_beholder_dispelmagic(int, P_char, P_char, P_obj);

/* board.c */

int board(P_obj, P_char, int, char *);
void board_fix_long_desc(int, char *);
void board_load_board(const char *, char **, char **, int *);
void board_reset_board(char **, char **, int *);
void board_save_board(const char *, char **, char **, int *);
void initialize_boards(void);

/* chess.c */

int ChessPushSuccessful(P_char, P_obj, char *);
int ClearBoard(P_char, P_obj);
int CountChessBoards(void);
int GetChessCoord(char *);
void CreateNewChessBoard(P_obj);
void DispChessBoard(P_char, P_obj);
void NukeChessBoard(P_obj);

/* comm.c */
void append_prompt(P_char, char*);
int wizconnectsite(char *, char *, int);
int find_color_entry(int);
int get_from_q(struct txt_q *, char *);
int init_socket(int);
int process_input(P_desc);
int process_output(P_desc);
void write_to_pc_log(P_char, const char *, int);
void initialize_logs(P_char ch, bool reset_logs);
void clear_logs(P_char);
void act(const char *, int, P_char, P_obj, void *, int);
void close_socket(P_desc);
void close_sockets(int);
void coma(int);
void flush_queues(P_desc);
void nonblock(int);
void parse_name(P_desc, char *);
void perform_complex(P_char, P_char, P_obj, P_obj, char *, int, int);
void perform_to_all(const char *, P_char);
void send_to_all(const char *);
void send_to_char_f(P_char ch, const char *fmt, ... );
void send_to_char(const char *, P_char);
void send_to_char(const char *, P_char, int);
bool send_to_pid(const char*, int);
void send_to_except(const char *, P_char);
void send_to_outdoor(const char *);
void send_to_room_f(int room, const char *fmt, ... );
void send_to_room(const char *, int);
void send_to_room_except(const char *, int, P_char);
void send_to_room_except_two(const char *, int, P_char, P_char);
void send_to_zone(int, const char *);
void send_to_zone_indoor(int, const char *);
void send_to_zone_outdoor(int, const char *);
void send_to_weather_sector(int, const char *);
void send_to_nearby_rooms(int, const char *);
void write_to_q(const char *, struct txt_q *, const int);
const char *delete_doubledollar(const char *);

/* condition.c */

char *item_condition(P_obj);
int DamageOneItem(P_char, int, P_obj, bool);
void MakeScrap(P_char, P_obj);
void DamageAllStuff(P_char, int);
void DamageStuff(P_char, int);
void DestroyStuff(P_char, int);

/* db.c */

P_char read_mobile(int, int);
P_obj read_object(int, int);
char *file_to_string(const char *);
char *fread_string(FILE *);
int is_empty(int);
int real_zone(const int);
int real_zone0(const int);
int real_mobile(const int);
int real_mobile0(const int);
int real_object(const int);
int real_object0(const int);
int real_room(const int);
int real_room0(const int);
int writePet(P_char);
int writeShopKeeper(P_char);
void MemReport(void);
void boot_db(int);
void boot_pose_messages(void);
void boot_world(int);
void boot_zones(int);
void clear_char(P_char);
void clear_object(P_obj);
void ensure_pconly_pool(void);
void free_char(P_char);
void free_obj(P_obj);
void init_char(P_char);
void renum_world(void);
void renum_zone_table(void);
void reset_char(P_char);
void reset_time(void);
void reset_zone(int, int);
void setup_dir(FILE *, int, int);
void skip_fread(FILE *);
void weather_setup(void);
int get_mob_table(int);
int get_obj_table(int);
void room_event(P_char, P_char, P_obj, void*);
int InsertIntoFile(const char* filename, const char* text);

/* debug.c */

void cmdlog(P_char, char *);
void do_debug(P_char, char *, int);
void do_mreport(P_char, char *, int);
void hour_debug(void);
void init_cmdlog(void);
void loop_debug(void);

/* drannak.c */
void event_update_surnames(P_char ch, P_char victim, P_obj, void *data);
bool quested_spell(P_char ch, int spl);
int vnum_in_inv(P_char ch, int vnum);
void vnum_from_inv(P_char ch, int item, int count);
void do_surname(P_char, char *, int);
void set_surname(P_char ch, int num);
void clear_surname(P_char ch);
void display_surnames(P_char ch);
bool lightbringer_proc(P_char ch, P_char victim, bool phys);
bool intercept_defensiveproc(P_char, P_char);
char get_alias(P_char ch, char *argument);
void create_alias_file(const char *dir, char *name);
void create_alias_name(char *name);
int equipped_value(P_char ch);
void newbie_reincarnate(P_char ch);
void random_recipe(P_char ch, P_char victim);
P_obj random_zone_item(P_char ch);
void do_conjure(P_char ch, char *argument, int cmd);
void create_spellbook_file(P_char ch);
bool new_summon_check(P_char ch, P_char selected);
void learn_conjure_recipe(P_char ch, P_char victim);
bool minotaur_race_proc(P_char, P_char);
void do_dismiss(P_char ch, char *argument, int cmd);
bool valid_conjure(P_char, P_char);
int calculate_shipfrags(P_char);
void randomizeitem(P_char, P_obj);
bool calmcheck(P_char ch);
void modenhance(P_char, P_obj, P_obj);
void enhance(P_char, P_obj, P_obj);
void do_enhance(P_char ch, char *argument, int cmd);
int get_progress(P_char ch, int ach, uint required);
void thanksgiving_proc(P_char ch);
void christmas_proc(P_char ch);
void enhancematload(P_char ch, P_char killer);
void add_bloodlust(P_char ch, P_char victim);
bool add_epiccount(P_char ch, int gain);


/* editor.c */
void edit_free(struct edit_data *);
void edit_string_add(struct edit_data *, char *);
void edit_start(P_desc desc, char *old_text, int max_lines,
           void (*callback)(P_desc, int, char *), int callback_data);

/* events.c */

void clear_char_nevents(P_char, int, void*);
void load_event_names();
bool RemoveEvent(void);
__attribute__((deprecated)) bool Schedule(int, long, int, void *, void *);
void set_event_time(P_event e1, int secs);
int event_time(P_event, int);
int Berserk(P_char, int);
void CharWait(P_char, int);
void ClearCharEvents(P_char);
void ClearObjEvents(P_obj);
void clear_events_type(P_char, int);
void Events(void);
void ReSchedule(void);
void StartRegen(P_char, int);
void Stun(P_char, P_char, int, bool);
void init_events(void);
typedef void (*event_func)(P_char ch, P_char victim, P_obj obj, void *data);
void add_event(event_func, int, P_char, P_char, P_obj, int, void*, int);

P_nevent get_scheduled(P_char, event_func_type);
P_nevent get_scheduled(P_obj, event_func_type);
P_nevent get_scheduled(event_func_type);
P_nevent get_next_scheduled_char(P_nevent, event_func_type);
P_nevent get_next_scheduled_obj(P_nevent, event_func_type);
void disarm_char_nevents(P_char, event_func_type);
void disarm_obj_nevents(P_obj, event_func_type);
int ne_event_time(P_nevent);
void zone_purge(int);

/* new_events.c */

void check_nevents();
void disarm_single_event(P_nevent);

// epic.c
void refund_epic_skills(P_char ch);

/* fight.c */
bool rapier_dirk(P_char, P_char);
int calculate_thac_zero(P_char, int);
bool opposite_racewar(P_char ch, P_char victim);
void displayHardCore(P_char ch, char *arg, int cmd);
void displayLeader(P_char ch, char *arg, int cmd);
void displayRelic(P_char ch, char *arg, int cmd);
int leapSucceed(P_char, P_char);
int damage_modifier(P_char, P_char, int);
#if 0
int get_char_dodge_skill(P_char);
int get_char_parry_skill(P_char);
#endif
#ifdef REALTIME_COMBAT
int CharNumberOfAttacks(P_char);
int Combat_Tick_Maint(P_char);
void SingleCombatCall(P_char);
#endif
P_char ForceReturn(P_char);
bool AdjacentInRoom(P_char, P_char);
bool PhasedAttack(P_char, int);
bool damage(P_char, P_char, double, int);
int raw_damage(P_char ch, P_char vict, double dam, uint flags, struct damage_messages *messages);
int spell_damage(P_char ch, P_char vict, double dam, int type, uint flags, struct damage_messages *messages);
int melee_damage(P_char ch, P_char vict, double dam, int type, struct damage_messages *messages);
int PartySizeMod(int, int, int, int);
int TryRiposte(P_char, P_char);
int vamp(P_char, double, double);
void heal(P_char, P_char, int, int);
bool blind(P_char, P_char, int);
void retarget_event(P_char ch, P_char victim, P_obj obj, void *data);
void MoveAllAttackers(P_char,P_char);
void StopAllAttackers(P_char);
void StopMercifulAttackers(P_char);
void appear(P_char ch, bool removeHide = TRUE);
int attack_back(P_char, P_char, int);
void change_alignment(P_char, P_char);
void check_killer(P_char, P_char);
void death_cry(P_char);
void death_rattle(P_char);
void die(P_char, P_char);
void do_trophy(P_char, char *, int);
void group_gain(P_char, P_char);
float group_exp_modifier(P_char ch);
#ifndef NEW_COMBAT
bool hit(P_char, P_char, P_obj);
int chance_to_hit(P_char, P_char, int, P_obj);
bool weapon_proc(P_obj, P_char, P_char);
int calculate_ac(P_char);
#endif
void load_messages(void);
P_obj make_corpse(P_char, int);
void make_bloodstain(P_char);
#ifndef NEW_COMBAT
void perform_violence(void);
#endif
void set_fighting(P_char, P_char);
bool set_fighting(P_char, P_char, bool);
void set_destroying(P_char, P_obj);
void stop_fighting(P_char);
void stop_destroying(P_char);
void engage(P_char, P_char);
void soul_taking_check(P_char, P_char);
/*
void swapWeapon(P_char);
*/
void update_pos(P_char);
bool can_damage(P_char, P_char);  /* TASFALEN */
/*
void swapWeapon2(P_char, int, int);
*/
bool can_hit_target(P_char , P_char);

/* files.c */

void moveToBackup(char *name);
int writeCharacter(P_char, int, int);
void restore_houses();
void writeShapechangeData(P_char ch);
int writeWitness(char *, wtns_rec *);
int register_ship(int);
int ship_registered (int);
int restoreWitness(char *, P_char);
bool writeObjectlist(P_obj, int);
char *getString(char **);
int confiscate_item(P_char, int);
int convert_stat(int);
int countEquip(P_char);
int countInven(P_obj);
int deleteCharacter(P_char, bool bDeleteLocker = true);
int deletePet(char *);
int deleteShopKeeper(int);
P_obj read_one_object(char*);
int restoreAffects(char *, P_char);
int restoreCharOnly(P_char, char *);
P_char restorePet(char *);
P_char restoreShopKeeper(int);
int restoreItemsOnly(P_char, int);
P_obj restoreObjects(char *, P_char, int);
int restorePasswdOnly(P_char, char *);
int restoreSkills(char *, P_char, int);
int restoreStatus(char *, P_char);
int restorePetStatus(char *, P_char);
int restoreWitnessed(char *, P_char);
void updateShortAffects( P_char ch );
int writeAffects(char *, struct affected_type *);
int writeItems(char *, P_char);
int writeSkills(char *, P_char, int);
int writeStatus(char *, P_char, bool);
int writePetStatus(char *, P_char);
int writeWitnessed(char *, P_char);
uint getInt(char **);
unsigned long getLong(char **);
ulong ObjUniqueFlags(P_obj, P_obj);
ush_int getShort(char **);
void PurgeCorpseFile(P_obj);
void confiscate_all(P_char);
//void recalc_base_hits(P_char);
void restoreCorpses(void);
void writeCorpse(P_obj);
int writeObject(P_obj, int, ulong, int, int, char*);
int write_one_object(P_obj, char*);
void restoreSavedItems(void);
void writeSavedItem(P_obj);
void PurgeSavedItemFile(P_obj);
void restore_allpets (void);
int deleteJailItems(P_char);
int writeJailItems(P_char);
int restoreJailItems(P_char);
int deleteHouseObj(int);

/* fraglist.c */
int fragWorthy(P_char ch, P_char victim);
void deleteFragEntry(char names[15 ][MAX_STRING_LENGTH ], int frags[15 ], int pos);
void insertFragEntry(char names[15 ][MAX_STRING_LENGTH ], int frags[15 ], char *name, int newFrags, int pos);
void do_fraglist(P_char ch, char *arg, int cmd);
void checkFragList(P_char ch);

/* random.mob.c */
P_char create_random_mob(int theme, int mob_level);
void do_namedreport(P_char ch, char *argument, int cmd);

/* random.zone.c */
void display_random_zones(P_char ch);
int reset_lab(int type);

/* randomeq.c */

void create_randoms();
bool check_random_drop(P_char ch, P_char mob, bool piece);
P_obj create_random_eq(int charlvl, int moblvl, int item_type, int material_type);
P_obj create_random_eq_new(P_char killer, P_char mob, int item_type, int material_type);
P_obj setprefix_obj(P_obj obj, float modifier, int affectnumber);
P_obj create_stones(P_char ch);
P_obj create_material(int index);
P_obj create_material(P_char ch, P_char mob);
P_obj setsuffix_obj(P_obj obj, int modifier, int suffix);
P_obj setsuffix_obj_new(P_obj obj);

P_obj create_sigil(int x);

int create_lab(int type);


/* hardcore.c */
int getHardCorePts(P_char);
void deleteHallEntry(char names[15][MAX_STRING_LENGTH], int frags[15], int pos, char killer[15][MAX_STRING_LENGTH]);
void insertHallEntry(char names[15][MAX_STRING_LENGTH], int frags[15], char *name, int newFrags, int pos, char killer[15][MAX_STRING_LENGTH], char *killername);
void checkHallOfFame(P_char ch, char thekiller[1024]);
void writeHallOfFame(P_char ch, char thekiller[1024]);

/* leaderboard.c */
void checkLeaderBoard(P_char ch);
long getLeaderBoardPts(P_char ch);

/* period.list.c */
void place_period_books();
void display_book(P_char ch);
float getTomeTropy(int id);
void deletePeriodEntry(char names[15 ][MAX_STRING_LENGTH ], int frags[15 ], int pos, char killer[10][MAX_STRING_LENGTH]);
void insertPeriodEntry(char names[15 ][MAX_STRING_LENGTH ], int frags[15 ], char *name, int newFrags, int pos, char killer[10][MAX_STRING_LENGTH], char *killername);
void checkPEriodOfFame(P_char ch, char thekiller[1024]);
void writePeriodOfFame(P_char ch, char thekiller[1024]);
void displayPERIODCore(P_char ch, char *arg, int cmd);
/* graph.c */


byte find_first_step(int src, int target, long hunt_flags, int is_ship, int wagon_type, int *ttl_steps);
ubyte *find_the_path(int from, int to, int *max_steps, long hunt_flags);
int how_close(int src, int target, int max_steps);
byte line_of_sight_dir(int, int);

/* group.c */

int on_front_line(P_char);
int get_numb_chars_in_group(struct group_list *);
int is_guild_golem(P_char, P_char);
void do_group (P_char, char *, int);
void do_disband (P_char, char *, int);
bool group_remove_member(P_char);
bool group_add_member(P_char, P_char);
void fix_group_ranks(P_char);
int verify_group_formation(P_char, int);
/* guild.c */
char *replace(char *g_string, char *replace_from, char *replace_to);
char *replace_it(char *g_string, char *replace_from, char *replace_to);
int readGuildFile(P_char ch, int zonenum);
int sackGuild(int oldguild, int guildnumber , int newguild);
bool CharHasSpec(P_char);
char *how_good(int, int);
int CharMaxSkill(P_char, int);
int FindHomeTown(P_char);
int GetClassType(P_char);
int GetPrimeStat(P_char, int);
int GetSkillType(int);
P_char FindTeacher(P_char);
int IsTaughtHere(P_char, int);
int RobCash(P_char, int);
int SkillRaiseCost(P_char, int);
int SpellCopyCost(P_char, int);
void do_practice(P_char, char *, int);
void do_descend(P_char, char *, int);
void do_ascend(P_char, char *, int);
void do_remort(P_char, char *, int);
void do_spec(P_char, char *, int);
void do_skills(P_char, char *, int);
void do_spells(P_char, char *, int);
bool notch_skill(P_char, int, float);
void SetGuildSpellLvl(void);
void update_skills(P_char);
string list_spells(int, int);
string list_skills(int, int);
string list_songs(int, int);

/* handler.c */

void generic_char_event(P_char ch, P_char victim, P_obj obj, void *data);
//void generic_char_event(void);
void sun_damage_check(P_char);
P_char get_char(char *);
P_char get_char2(char *);
P_char get_char_ranged (const char *, P_char, int, int);
P_char get_char_ranged_vis (P_char, char *, int);
P_char get_char_num(int);
P_char get_char_room(const char *, int);
P_char get_char_room_vis(P_char, const char *);
P_char get_char_vis(P_char, const char *);
P_char get_pcchar(P_char, char *, int);
P_obj create_money(int, int, int, int);
void money_to_inventory(P_char ch);
P_obj get_obj(char *);
P_obj get_obj_in_list(char *, P_obj);
P_obj get_obj_in_list_num(int num, P_obj);
P_obj get_obj_in_list_vis(P_char ch, char *name, P_obj list, bool no_tracks = TRUE);
P_obj get_obj_num(int);
P_obj get_obj_vis(P_char ch, char *name, int zrange = 0);
P_obj get_obj_equipped( P_char ch, char *arg );
P_obj unequip_char(P_char, int, bool = FALSE);
void unequip_all(P_char);
struct affected_type *get_spell_from_char(P_char ch, int spell, void *context = NULL);
struct room_affect *get_spell_from_room(P_room, int );
bool affected_by_spell(P_char, int);
int affected_by_spell_count(P_char, int);
bool affected_by_spell_flagged(P_char, int, uint);
bool affected_by_skill(P_char ch, int skill);
bool isname(const char *, const char *);
char *FirstWord(char *);
int can_char_use_item(P_char, P_obj);
int char_light(P_char);
int generic_find(char *, int, P_char, P_char *, P_obj *);
int get_number(char **);
int room_light(int, int);
void resolve_poison(P_char, int);
void Decay(P_obj);
void ac_stopAllFromConsenting(P_char);
void ac_stopAllFromIgnoring(P_char);
void add_coins(P_obj, int, int, int, int);
void affect_from_char(P_char, int);
void add_tag_to_char(P_char ch, int tag, int value, int flags);
void add_counter(P_char ch, int tag, int value, int duration);
void add_counter(P_char ch, int tag);
void remove_counter(P_char ch, int tag);
void remove_counter(P_char ch, int tag, int modifier);
int counter(P_char ch, int tag);
void affect_join(P_char, struct affected_type *, int, int);
void affect_remove(P_char, struct affected_type *);
struct affected_type* affect_to_char(P_char, struct affected_type *);
void affect_to_char_with_messages(P_char, struct affected_type *, char*, char*);
struct room_affect *affect_to_room(int, struct room_affect *);
void affect_room_remove(int, struct room_affect *);
char affect_total(P_char, int);
void all_affects(P_char, int);
void apply_affs(P_char, int);
void balance_affects(P_char);
void char_from_room(P_char);
bool char_to_room(P_char, int, int);
void equip_char(P_char, P_obj, int, int);
void extract_char(P_char);
void extract_obj(P_obj obj, int gone_for_good = FALSE);  // Only use gone_for_good for purging arti data.
                                                         //   If it was just a temp object, _don't_ use.
                                                         // If it was actually in game, takeable by players,
                                                         //   and it's going away completely, then use.
void obj_from_char(P_obj);
void obj_from_obj(P_obj);
void obj_from_room(P_obj);
void obj_to_char(P_obj obj, P_char ch);
void obj_to_obj(P_obj, P_obj);
void obj_to_room(P_obj, int);
void object_list_new_owner(P_obj, P_char);
void update_char_objects(P_char);
void update_con_bonus(P_char);
void update_object(P_obj, int);
struct char_link_data* link_char(P_char ch, P_char target, ush_int type);
P_char get_linked_char(P_char ch, ush_int type);
P_char get_linking_char(P_char ch, ush_int type);
void unlink_char(P_char ch, P_char target, ush_int type);
void clear_links(P_char ch, ush_int type);
void clear_links( P_char ch, P_obj obj, int flag );
void clear_all_links(P_char ch);
bool is_linked_to(P_char target, P_char ch, ush_int type);
bool is_linked_to(P_char target, P_obj obj, ush_int type);
int is_linked_from(P_char target, P_char ch, ush_int type);
P_char in_command_aura(P_char);
//struct affected_type* get_aura_affects(P_char);
struct obj_affect* get_obj_affect(P_obj, int);
int obj_affect_time(P_obj, struct obj_affect*);
void set_obj_affected(P_obj, int, sh_int, sh_int);
int affect_from_obj(P_obj, sh_int);


int io_agi_defense(P_char);
int io_con_hitp(P_char);


/* innates.c */
void assign_innates();
string list_innates(int, int, int);
void do_innate_decrepify(P_char, P_char);
bool has_divine_force(P_char);
bool rapier_dirk_check(P_char);
int attuned_to_terrain(P_char);
void do_engulf(P_char, char *, int);
void engulf(P_char, P_char);
void do_slime(P_char, char *, int);
void slime(P_char, P_char);
void do_squidrage(P_char ch, char *arg, int cmd);
int get_innate_from_skill( int skill );
int get_level_from_innate( P_char ch, int innate );
/* interp.c */

bool special(P_char, int, char *);
char *one_argument(const char *, char *);
char lower(char);
int fill_word(char *);
bool is_abbrev(const char *, const char *); // Checks to see if char *a is a prefix of char* b
bool ends_with(const char *, const char *); // Checks to see if char *a ends with suffix char* b
bool is_number(char *);
bool is_real_number(char *);
int old_search_block(const char *, uint, uint, const char **, int);
int search_block(char *, const char **, int);
void GRANTSET(int, int);
void argument_interpreter(char *, char *, char *);
void assign_command_pointers(void);
void assign_grant_commands(void);
void command_interpreter(P_char, char *);
void do_confirm(P_char, bool);
void half_chop(char *, char *, char *);
char *lohrr_chop(char *, char *);

/* justice.c */

void check_item(P_char);
void crime_add(int, char *, const char *, int, int, time_t, int, int);
crm_rec *crime_find(crm_rec *, char *, const char *, int, int, int, crm_rec *);
int crime_remove(int, crm_rec *);
void witness_add(P_char, P_char, P_char, int, int);
wtns_rec *witness_find(wtns_rec *, char *, char *, int, int, wtns_rec *);
P_char justice_make_guard(int);
void justice_delete_guard(P_char);
int justice_send_guards(int, P_char, int, int);
void do_sorta_yell (P_char, char *);
void justice_hometown_echo(int, const char *);
void do_justice(P_char, char *, int);
void JusticeGuardMove(P_char, char *, int);
void JusticeGuardHunt(P_char);
void justice_set_outcast(P_char ch, int town);
int witness_remove(P_char, wtns_rec *);
void do_report_crime(P_char, char *, int);
int justice_is_criminal(P_char);
void PC_SET_TOWN_JUSTICE_FLAGS(P_char ch, int flag, int town);
void justice_action_invader(P_char ch);
void event_justice_raiding(P_char, P_char, P_obj, void *);
void justice_action_wanted(P_char ch);
void justice_action_arrest(P_char, P_char);
void justice_guard_remove(P_char ch);
int shout_and_hunt(P_char ch, int max_distance, const char *shout_str,
                   int (*locator_proc)(P_char, P_char, int, char *),
                   int vnums[], ulong act_mask, ulong no_act_mask);
void justice_engine(int);
void justice_dispatch_guard(int , char *, char *, int);
void justice_sentence_outcast(P_char, int);
void justice_send_witness(P_char, P_char, P_char, int, int);
void justice_judge(P_char, int);
void load_justice_area(void);
void witness_scan(P_char, P_char, int, int, int);
void set_town_flag_justice(P_char, int);
void clean_town_justice(void);

/* languages.c */

char *language_CRYPT(P_char, P_char, char *);
char *language_known(P_char, P_char);
char *language_singlepass(P_char, int, char *);
char casecorrect(int);
char low_case(int);
int npc_get_pseudo_language_skill(P_char, int);
int npc_get_pseudo_spoken_language(P_char);
int on_aakkonen(int);
int on_vokaali(int);
void do_speak(P_char, char *, int);
void init_defaultlanguages(P_char);
void language_gain(P_char, P_char, int);
void language_show(P_char);

/* limits.c */

int frags_lvl_adjustment(P_char ch, int howmuch);
int graf(P_char, int, int, int, int, int, int, int, int);
int hit_limit(P_char);
int hit_regen(P_char, bool);
int mana_limit(P_char);
int mana_regen(P_char, bool);
int vitality_limit(P_char);
int move_regen(P_char, bool);
void advance_level(P_char);
void illithid_advance_level(P_char);
void check_idling(P_char);
int gain_condition(P_char, int, int);
int adjust_lvl_from_frags_period(P_char, int mod);
int frag_lvl_adjustment(P_char, int mod);
int tick_location_lvl_adjustment(P_char);
int gain_exp(P_char, P_char, int, int);
void gain_practices(P_char);
void lose_level(P_char);
void lose_practices(P_char);
void point_update(void);
void clear_title(P_char);

/* breath_weapons.c */
void breath_weapon_fire(int, P_char, char *, int, P_char, P_obj);
void breath_weapon_lightning(int, P_char, char *, int, P_char, P_obj);
void breath_weapon_frost(int, P_char, char *, int, P_char, P_obj);
void breath_weapon_acid(int, P_char, char *, int, P_char, P_obj);
void breath_weapon_poison(int, P_char, char *, int, P_char, P_obj);
void breath_weapon_sleep(int, P_char, char *, int, P_char, P_obj);
void breath_weapon_fear(int, P_char, char *, int, P_char, P_obj);
void breath_weapon_paralysis(int, P_char, char *, int, P_char, P_obj);
void breath_weapon_shadow_1(int, P_char, char *, int, P_char, P_obj);
void breath_weapon_shadow_2(int, P_char, char *, int, P_char, P_obj);
void breath_weapon_blind(int, P_char, char *, int, P_char, P_obj);
void breath_weapon_crimson(int, P_char, char *, int, P_char, P_obj);
void breath_weapon_jasper(int, P_char, char *, int, P_char, P_obj);
void breath_weapon_azure(int, P_char, char *, int, P_char, P_obj);
void breath_weapon_basalt(int, P_char, char *, int, P_char, P_obj);
void breath_weapon_crimson_2(int, P_char, char *, int, P_char, P_obj);
void breath_weapon_azure_2(int, P_char, char *, int, P_char, P_obj);
void breath_weapon_jasper_2(int, P_char, char *, int, P_char, P_obj);
void breath_weapon_basalt_2(int, P_char, char *, int, P_char, P_obj);

/* magic.c */

/* commented out declarations before have been "cleaned up", and are temporarily
 * declared in the spell.c section of this file until we finish cleaning up
 * and move everything back to its proper place. 06 May 2003 dbb */
 
void spell_mielikki_vitality(int, P_char, char *, int, P_char, P_obj);
void do_nothing_spell(int, P_char, char *, int, P_char, P_obj);
bool can_relocate_to(P_char, P_char);
void cure_arrow_wound(P_char);
void set_up_portals(P_char, P_obj, P_obj, int);
bool can_do_general_portal( int level, P_char ch, P_char victim,
                            struct portal_settings *settings,
                            struct portal_create_messages *messages );
bool spell_general_portal( int level, P_char ch, P_char victim,
                           struct portal_settings *settings,
                           struct portal_create_messages *messages, bool isOneWay = false );
bool is_portal(P_obj);
bool can_conjure_lesser_elem(P_char, int);
bool can_conjure_greater_elem(P_char, int);
int can_call_woodland_beings(P_char, int);
int can_raise_undead(P_char, int);
int can_raise_draco(P_char, int, bool);
bool should_area_hit(P_char, P_char);
bool BardAffectCheck(P_char, P_char, int);
bool check_item_teleport(P_char, char *, int);
int is_any_draco(P_char);
int has_greater_draco(P_char);
int KludgeDuration(P_char, int, int);
int Summonable(P_char);
int get_room_in_zone(int, P_char);
int modify_by_specialization(P_char, int, int);
P_char stack_area(P_char, int, int);
void AgeChar(P_char, int);
void BackToUsualForm(P_char);
void creeping_kludge(void);
void darkness_dissipate_event(P_char ch, P_char victim, P_obj obj, void *data);
void airy_water_dissipate_event(void);
void starshell_dissipate_event(void);
void cdoom_event(P_char, P_char);
void heat_blind(P_char);
void silence_starshell_event(void);
int count_undead(P_char ch);
int count_pets(P_char ch);
void pleasantry(P_char ch);
void do_pleasantry(P_char, char *, int);
void spell_chaotic_ripple(int, P_char, char*, int, P_char, P_obj);
void spell_blink(int, P_char, char*, int, P_char, P_obj);
void spell_moonstone(int, P_char, char *, int, P_char, P_obj);
void spell_pleasantry(int, P_char, char *, int, P_char, P_obj); /* NEW */
void spell_acid_breath(int, P_char, char*, int, P_char, P_obj);
void spell_awareness(int, P_char, P_char, P_obj);
void spell_blinding_breath(int, P_char, char*, int, P_char, P_obj);
void spell_burning_hands(int, P_char, char *, int, P_char, P_obj); /* NEW */
void spell_call_lightning(int, P_char, P_char, P_obj);
void spell_chain_lightning(int, P_char, char *, int, P_char, P_obj);
void spell_ring_lightning(int, P_char, char *, int, P_char, P_obj);
void spell_nether_gate(int, P_char, P_char, P_obj);
void spell_control_weather(int, P_char, P_char, P_obj);
void spell_create_water(int, P_char, char *, int, P_char, P_obj);
void spell_innate_darkness(int, P_char, P_char, P_obj);
void spell_energy_drain(int, P_char, char*, int, P_char, P_obj);
void spell_life_leech(int, P_char, char*, int, P_char, P_obj);
void spell_faerie_fire(int, P_char, char*, int, P_char, P_obj);
void spell_faerie_fog(int, P_char, char*, int, P_char, P_obj);
void spell_farsee(int, P_char, char*, int, P_char, P_obj);
void spell_fear(int, P_char, char*, int, P_char, P_obj);
void spell_feeblemind(int, P_char, char*, int, P_char, P_obj);
void spell_fire_breath(int, P_char, char*, int, P_char, P_obj);
void spell_basalt_light_2(int, P_char, char*, int, P_char, P_obj);
void spell_basalt_light(int, P_char, char*, int, P_char, P_obj);
void spell_jasper_light_2(int, P_char, char*, int, P_char, P_obj);
void spell_jasper_light(int, P_char, char*, int, P_char, P_obj);
void spell_azure_light_2(int, P_char, char*, int, P_char, P_obj);
void spell_azure_light(int, P_char, char*, int, P_char, P_obj);
void spell_crimson_light_2(int, P_char, char*, int, P_char, P_obj);
void spell_crimson_light(int, P_char, char*, int, P_char, P_obj);
void spell_fireball(int, P_char, char*, int, P_char, P_obj);
void spell_living_stone(int, P_char, char *, int, P_char, P_obj);
void spell_elemental_swarm(int, P_char, char *, int, P_char, P_obj);
void spell_greater_living_stone(int, P_char, char *, int, P_char, P_obj);
void spell_fireshield(int, P_char, char*, int, P_char, P_obj);
void spell_firestorm(int, P_char, char*, int, P_char, P_obj);
void spell_fly(int, P_char, char *, int, P_char, P_obj);
void spell_frost_breath(int, P_char, char*, int, P_char, P_obj);
void spell_destroy_undead(int, P_char, char *, int, P_char, P_obj);
void spell_taint(int, P_char, char *, int, P_char, P_obj);
void spell_gas_breath(int, P_char, char*, int, P_char, P_obj);
void spell_gate(int, P_char, P_char, P_obj);
void spell_globe(int, P_char, char *, int, P_char, P_obj);
void spell_group_globe(int, P_char, char *, int, P_char, P_obj);
void spell_harm(int, P_char, char *, int, P_char, P_obj);
void astral_banishment(P_char ch, P_char victim, int hwordtype, int level);
void spell_voice_of_creation(int, P_char, char *, int, P_char, P_obj);
void spell_holy_word(int, P_char, char *, int, P_char, P_obj);
void spell_ice_storm(int, P_char, char *, int, P_char, P_obj);
void spell_regeneration(int, P_char, char *, int, P_char, P_obj);
void spell_incendiary_cloud(int, P_char, char *, int, P_char, P_obj);
void spell_obtenebration(int, P_char, char *, int, P_char, P_obj);
void spell_immolate(int, P_char, char *, int, P_char, P_obj);
void spell_acidimmolate(int, P_char, char *, int, P_char, P_obj);
void spell_levitate(int, P_char, char *, int, P_char, P_obj);
void spell_lightning_bolt(int, P_char, char *, int, P_char, P_obj);
void spell_lightning_breath(int, P_char, char*, int, P_char, P_obj);
void spell_lightning_shield(int, P_char, char*, int, P_char, P_obj);
void spell_locate_object(int, P_char, char *);
void spell_magic_missile(int, P_char, char *, int, P_char, P_obj);
void spell_magma_burst(int, P_char, char *, int, P_char, P_obj);
void spell_solar_flare(int, P_char, char *, int, P_char, P_obj);
void spell_major_magical_resistance(int, P_char, P_char, P_obj);
void spell_major_physical_resistance(int, P_char, P_char, P_obj);
void spell_meteorswarm(int, P_char, char*, int, P_char, P_obj);
void spell_minor_creation(int, P_char, P_char, P_obj);
void spell_minor_globe(int, P_char, char *, int, P_char, P_obj);
void spell_minor_paralysis(int, P_char, char*, int, P_char, P_obj);
void spell_mordenkainens_lucubration(int, P_char, char*, int, P_char, P_obj);
void spell_plane_shift(int, P_char, P_char, P_obj);
void spell_polymorph_object(int, P_char, P_char, P_obj);
void spell_prismatic_spray(int, P_char, char *, int, P_char, P_obj);
void spell_pword_blind(int, P_char, char *, int, P_char, P_obj);
void spell_pword_kill(int, P_char, char *, int, P_char, P_obj);
void spell_pword_stun(int, P_char, char *, int, P_char, P_obj);
void spell_ray_of_enfeeblement(int, P_char, char *, int, P_char, P_obj);
void spell_recharger(int, P_char, char*, int, P_char, P_obj);
void spell_rejuvenate_major(int, P_char, char*, int, P_char, P_obj);
void spell_rejuvenate_minor(int, P_char, char*, int, P_char, P_obj);
void spell_shadow_breath_1(int, P_char, char*, int, P_char, P_obj);
void spell_shadow_breath_2(int, P_char, char*, int, P_char, P_obj);
void spell_shocking_grasp(int, P_char, char *, int, P_char, P_obj);
void spell_silence(int, P_char, char *, int, P_char, P_obj);
void spell_sleep(int, P_char, char*, int, P_char, P_obj);
void spell_slow_poison(int, P_char, char *, int, P_char, P_obj);
void spell_sticks_to_snakes(int, P_char, char*, int, P_char, P_obj);
void spell_spore_cloud(int, P_char, char*, int, P_char, P_obj);
void spell_sunray(int, P_char, char*, int, P_char, P_obj);
void spell_turn_undead(int, P_char, char *, int, P_char, P_obj);
void spell_unholy_word(int, P_char, char *, int, P_char, P_obj);
void spell_vigorize_critic(int, P_char, char *, int, P_char, P_obj);
void spell_vigorize_light(int, P_char, char *, int, P_char, P_obj);
void spell_vigorize_serious(int, P_char, char *, int, P_char, P_obj);
void spell_vitality(int, P_char, char *, int, P_char, P_obj);
void spell_vitalize_undead(int, P_char, char *, int, P_char, P_obj);
void spell_vitalize_mana(int, P_char, char *, int, P_char, P_obj);
void spell_waterbreath(int, P_char, char *, int, P_char, P_obj);
void spell_wither(int, P_char, char *, int, P_char, P_obj);
void spell_death_blessing(int, P_char, char *, int, P_char, P_obj);
void spell_channel(int, P_char, P_char, P_obj);
void spell_healing_salve(int , P_char , char *, int , P_char , P_obj );
void spell_flare(int , P_char , char *, int , P_char , P_obj );
//void event_plague(P_char ch, P_char vict, P_obj obj, void *data);
void spell_plague(int , P_char , char *, int , P_char , P_obj );
void spell_blackmantle(int , P_char , char *, int , P_char , P_obj );
void spell_banish(int , P_char , char *, int , P_char , P_obj );
void spell_word_proc(int level, P_char, int, int);
void spell_wraithform(int, P_char, P_char, char *);
void spell_greater_wraithform(int, P_char, P_char, char *);
void teleport_to(P_char, int, int);
void unequip_char_dale(P_obj);
void zone_spellmessage(int, bool, const char *, const char *msg_dir = NULL);
void zone_powerspellmessage(int, const char *);
int CheckMindflayerPresence(P_char);
void charm_generic(int, P_char, P_char);
void cont_light_dissipate_event(P_char ch, P_char victim, P_obj obj, void *data);
void spell_stornogs_spheres(int, P_char, char *, int, P_char, P_obj);
void spell_decaying_flesh(int, P_char, char *, int, P_char, P_obj);
void spell_group_stornog(int, P_char, char *, int, P_char, P_obj);
void spell_ether_sense(int, P_char, char *, int, P_char, P_obj);
void spell_solbeeps_missile_barrage(int level, P_char ch, char *arg, int type, P_char victim, P_obj tar_obj);
void event_holy_dharma(P_char ch, P_char victim, P_obj obj, void *data);
void spell_holy_dharma(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj);
void event_electrical_execution(P_char ch, P_char victim, P_obj obj, void *data);
void spell_electrical_execution(int, P_char, char *, int, P_char, P_obj);
void spell_invigorate(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj);
void spell_life_bolt(int, P_char, char *, int, P_char, P_obj);
void spell_enervation(int, P_char, char *, int, P_char, P_obj);
void spell_restore_spirit(int, P_char, char *, int, P_char, P_obj);
void spell_repair_one_item(int, P_char, char *, int, P_char, P_obj);
void spell_corpse_portal(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj);
void spell_contain_being(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj);
void spell_infernal_fury(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj);
int get_spell_component(P_char, int, int);

/* smagic.c */

int can_summon_beast(P_char, int);
int can_summon_greater_beast(P_char, int);
P_char summon_beast_common(int, P_char, int, int, const char *, const char *, int, bool);

void spell_tempest (int, P_char, char*, int, P_char, P_obj);
void spell_wind_rage (int, P_char, char*, int, P_char, P_obj);
void spell_ethereal_alliance(int level, P_char ch, char *arg, int type, P_char victim, P_obj tar_obj);

void spell_napalm(int, P_char, char*, int, P_char, P_obj);
void spell_strong_acid(int, P_char, char*, int, P_char, P_obj);
void spell_glass_bomb(int, P_char, char*, int, P_char, P_obj);
void spell_grease(int, P_char, char*, int, P_char, P_obj);
void spell_nitrogen(int, P_char, char*, int, P_char, P_obj);

void spell_elemental_fury (int, P_char, char*, int, P_char, P_obj);
void spell_elemental_affinity (int, P_char, char*, int, P_char, P_obj);
void spell_gaseous_cloud (int, P_char, char*, int, P_char, P_obj);
void spell_molten_spray (int, P_char, char*, int, P_char, P_obj);
void spell_ravenflight (int, P_char, char*, int, P_char, P_obj);
void spell_greater_ravenflight (int, P_char, char*, int, P_char, P_obj);
void spell_restore_item(int, P_char, char*, int, P_char, P_obj);
void spell_ice_missile (int, P_char, char*, int, P_char, P_obj);
void spell_call_of_the_wild (int, P_char, char*, int, P_char, P_obj);
void spell_corrosive_blast (int, P_char, char*, int, P_char, P_obj);
void spell_restoration (int, P_char, char*, int, P_char, P_obj);
void spell_flameburst (int, P_char, char*, int, P_char, P_obj);
void spell_scalding_blast (int, P_char, char*, int, P_char, P_obj);
void spell_fire_ward (int, P_char, char*, int, P_char, P_obj);
void spell_cold_ward (int, P_char, char*, int, P_char, P_obj);
void spell_reveal_true_form (int, P_char, char*, int, P_char, P_obj);
void spell_greater_spirit_ward (int, P_char, char*, int, P_char, P_obj);
void spell_spirit_ward (int, P_char, char*, int, P_char, P_obj);
void spell_greater_mending (int, P_char, char*, int, P_char, P_obj);
void spell_lesser_mending (int, P_char, char*, int, P_char, P_obj);
void spell_mending (int, P_char, char*, int, P_char, P_obj);
void spell_wellness (int, P_char, char*, int, P_char, P_obj);
void spell_wellness_one (int, P_char, char*, int, P_char, P_obj);
void spell_summon_spirit (int, P_char, char*, int, P_char, P_obj);
void spell_summon_beast (int, P_char, char*, int, P_char, P_obj);
void spell_greater_summon_beast (int, P_char, char*, int, P_char, P_obj);
void spell_sense_spirit (int, P_char, char*, int, P_char, P_obj);
void spell_greater_spirit_sight (int, P_char, char*, int, P_char, P_obj);
void spell_spirit_sight (int, P_char, char*, int, P_char, P_obj);
void spell_purify_spirit (int, P_char, char*, int, P_char, P_obj);
void spell_malison (int, P_char, char*, int, P_char, P_obj);
void spell_lionrage (int, P_char, char*, int, P_char, P_obj);
void spell_shrewtameness (int, P_char, char*, int, P_char, P_obj);
void spell_elephantstrength (int, P_char, char*, int, P_char, P_obj);
void spell_bearstrength (int, P_char, char*, int, P_char, P_obj);
void spell_hawkvision (int, P_char, char*, int, P_char, P_obj);
void spell_mousestrength (int, P_char, char*, int, P_char, P_obj);
void spell_molevision (int, P_char, char*, int, P_char, P_obj);
void spell_pantherspeed (int, P_char, char*, int, P_char, P_obj);
void spell_snailspeed (int, P_char, char*, int, P_char, P_obj);
void spell_wolfspeed (int, P_char, char*, int, P_char, P_obj);
void spell_greater_sustenance (int, P_char, char*, int, P_char, P_obj);
void spell_sustenance (int, P_char, char*, int, P_char, P_obj);
void spell_transfer_wellness (int, P_char, char*, int, P_char, P_obj);
void spell_bloodhound(int, P_char, char*, int, P_char, P_obj);
void spell_spirit_armor (int, P_char, char*, int, P_char, P_obj);
void spell_greater_pythonsting (int, P_char, char*, int, P_char, P_obj);
void spell_pythonsting (int, P_char, char*, int, P_char, P_obj);
void spell_greater_earthen_grasp (int, P_char, char*, int, P_char, P_obj);
void spell_earthen_grasp (int, P_char, char*, int, P_char, P_obj);
void spell_earthen_rain (int, P_char, char*, int, P_char, P_obj);
void spell_scathing_wind (int, P_char, char*, int, P_char, P_obj);
void spell_spirit_anguish (int, P_char, char*, int, P_char, P_obj);
void spell_greater_spirit_anguish (int, P_char, char*, int, P_char, P_obj);
void spell_greater_soul_disturbance (int, P_char, char*, int, P_char, P_obj);
void spell_soul_disturbance (int, P_char, char*, int, P_char, P_obj);
void spell_scorching_touch (int, P_char, char*, int, P_char, P_obj);
void spell_arieks_shattering_iceball (int, P_char, char*, int, P_char, P_obj);
void spell_reveal_spirit_essence (int, P_char, char*, int, P_char, P_obj);
void spell_spirit_jump (int, P_char, char*, int, P_char, P_obj);
void spell_etherportal (int, P_char, char*, int, P_char, P_obj);
void spell_beastform(int, P_char, char*, int, P_char, P_obj);
void spell_indomitability (int, P_char, char*, int, P_char, P_obj);
void spell_spirit_walk (int, P_char, char*, int, P_char, P_obj);
void spell_essence_of_the_wolf (int, P_char, char*, int, P_char, P_obj);
void spell_firebrand(int, P_char, char *, int, P_char, P_obj);
bool fear_check(P_char ch);
bool critical_disarm(P_char ch, P_char victim);
bool critical_attack(P_char ch, P_char victim, int msg);
void spell_cascading_elemental_beam (int, P_char, char*, int, P_char, P_obj);
void spell_guardian_spirits(int, P_char, char*, int, P_char, P_obj);
void guardian_spirits_messages(P_char, P_char);
void spell_torment_spirits(int, P_char, char *, int, P_char, P_obj);
void event_torment_spirits(P_char ch, P_char victim, P_obj obj, void *data);

/* mail.c */

void push_free_list(long);
long pop_free_list(void);
mail_index_type *find_char_in_index(char *);
void write_to_file(void *, unsigned int, long);
void read_from_file(void *, unsigned int, long);
void index_mail(char *, long);
int scan_mail_file(void);
int has_mail(char *);
void store_mail(char *, char *, char *);
char *read_delete(char *, char *);
int mail_ok(struct char_data *);
P_char find_mailman(P_char);
void do_mail(P_char, char *, int);
void postmaster_send_mail(P_char, P_char, char *);
int postmaster_check_mail(P_char, P_char);
void postmaster_receive_mail(P_char, P_char);

/* map.c */
char *strip_color(const char *);
void map_look(P_char, int);
void display_map(P_char, int, int);
bool is_in_line_of_sight_dir(P_char, P_char, int);
int calculate_map_distance(int, int);
unsigned int calculate_relative_room(unsigned int, int, int);
void random_encounters(P_char);
int randobjs_to_mob(P_char);
P_obj ran_magical(P_char);
P_obj ran_obj(P_char, ulong);

/* memorize.c */
int IS_PART_NOT_CASTER(P_char ch);

void balance_align(P_char ch);

bool book_class( P_char ch );
bool is_mage_spell(int);
bool is_shaman_spell(int);
P_obj FindSpellBookWithSpell(P_char, int, int);
P_obj Find_process_entry(P_char, P_obj, int);
P_obj SpellBookAtHand(P_char);
bool meming_class(P_char);
bool praying_class(P_char);
void* has_memorized(P_char, int);
int knows_spell(P_char, int);
void use_spell(P_char, int);
int AddSpellToSpellBook(P_char, P_obj, int);
int get_max_circle(P_char);
int GetPagesInBook(P_obj);
int get_spell_circle(P_char, int);
int get_spell_circle(int, int, int);
int get_skill_level(int, int, int);
int get_song_level(int, int, int);
int GetTotalPagesInBook(P_obj);
int IS_SEMI_CASTER(P_char);
int IS_PARTIAL_CASTER(P_char);
int IS_CASTER(P_char);
int ScriberSillyChecks(P_char, int);
P_obj SpellInSpellBook(P_char, int, int);
int SpellInThisSpellBook(struct extra_descr_data *, int);
int SpellInThisSpellBook_p(struct extra_descr_data *, int);
int max_spells_in_circle(P_char, int);
int spells_memmed_in_circle(P_char, int);
struct extra_descr_data *find_spell_description(P_obj);
void AddScribingAffect(P_char);
void SetBookNewbySpells(P_char);
void SetSpellCircles(void);
void add_scribe_data(int, P_char, P_obj, int, P_obj, P_char, void (*done_func) (P_char) = NULL);
void add_scribing(P_char, int, P_obj, int, P_obj, P_char);
void change_spells_in_circle(P_char, int, int);
void check_for_scribe_nukage_object(P_obj);
void do_forget(P_char, char *, int);
void do_memorize(P_char, char *, int);
void do_assimilate(P_char, char *, int);
void do_scribe(P_char, char *, int);
void do_teach(P_char, char *, int);
void handle_scribe(P_char, P_char, P_obj, void*);
void handle_spell_mem(P_char);
void handle_undead_mem(P_char);
void stop_memorizing(P_char);
int forget_spells(P_char, int);
void do_stance(P_char, char *, int);
int memorize_last_spell(P_char ch);
void bad_spell_check( P_char ch);

/* memory.c */

void dump_mem_log(void);
void* __malloc(size_t size, char* tag, char* file, int line);
void* __realloc(void* p, size_t size, char *file, int line);
void __free(void* p, char *file, int line);
//void *debug_calloc(size_t nobj, size_t size, char *file, int line);
//void *debug_realloc(void *p, size_t size, char *file, int line);
//void debug_free(void *p, char *file, int line);

/* mobact.c */

void event_agg_attack(P_char, P_char, P_obj, void*);
int char_deserves_helping(const P_char, const P_char, int);
int no_chars_in_room_deserve_helping(const P_char);
int room_has_evil_enemy(const P_char);
int room_has_good_enemy(const P_char);
int mobact_trapHandle(P_char);
void remember(struct char_data *ch, struct char_data *victim);
void remember(struct char_data *ch, struct char_data *victim, bool check_group_remember);
bool forget(struct char_data *ch, struct char_data *victim);
bool CheckForRemember(P_char ch);
P_char FindDispelTarget(P_char, int);
P_char FindJuiciestTarget(P_char);
P_char PickTarget(P_char);
bool CastClericSpell(P_char, P_char, int);
bool CastPaladinSpell(P_char, P_char, int);
bool CastAntiPaladinSpell(P_char, P_char, int);
bool CastMageSpell(P_char, P_char, int);
bool CastWarlockSpell(P_char, P_char, int);
bool PsionicistCombat(P_char);
bool WillPsionicistSpell(P_char, P_char);
bool CastRangerSpell(P_char, P_char, int);
bool CastReaverSpell(P_char, P_char, int);
bool CastIllusionistSpell(P_char, P_char, int);
bool CastDruidSpell(P_char, P_char, int);
bool CastBlighterSpell(P_char, P_char, bool);
void BreathWeapon(P_char, int);
int DemonCombat(P_char);
bool DragonCombat(P_char, int);
bool In_Adjacent_Room(P_char, P_char);
bool InitNewMobHunt(P_char ch);
bool MobCastSpell(P_char, P_char, P_obj, int, int);
bool MobKnowsSpell(P_char, int);
bool MobThief(P_char);
bool MobWarrior(P_char);
bool MobReaver(P_char);
bool MobRanger(P_char);
bool MobMonk(P_char);
bool MobAlchemist(P_char);
bool MobBerserker(P_char);
bool NewMobAct(P_char, int);
void event_mob_hunt(P_char ch, P_char victim, P_obj obj, void *data);
bool NewMobHunt(void);
bool MobDestroyWall(P_char ch, int dir, bool bTryHit = false);
bool MobDestroyWall(P_char ch, P_obj wall, bool bTryHit = false);
bool TryToGetHome(P_char);
bool npc_has_spell_slot(P_char, int);
int AlignRestriction(P_char, P_obj);
int CountToughness(P_char, P_char);
bool resists_spell(P_char, P_char);
int get_innate_resistance(P_char);
int FreshCorpse(int);
int GetLowestSpellCircle(int);
int GetLowestSpellCircle_p(int);
int GetMobMagicResistance(P_char);
int IS_CLERIC(P_char);
int IS_MAGE(P_char);
int IS_HOLY(P_char);
int IS_THIEF(P_char);
int IS_WARRIOR(P_char);
int IsBetterObject(P_char, P_obj, int);
int ItemsIn(P_obj);
int MobCanGo(P_char, int);
int RateObject(P_char, int, P_obj);
int UndeadCombat(P_char);
int AngelCombat(P_char);
int GenMobCombat(P_char);
int BeholderCombat(P_char);
int MobItemUse(P_char, int);
void mobact_rescueHandle(P_char, P_char);
void AddCharToZone(P_char);
void AddToRememberArray(P_char, int);
void CheckEqWorthUsing(P_char, P_obj);
void DelCharFromZone(P_char);
void GhostFearEffect(P_char);
void MobCombat(P_char);
void MobHuntCheck(P_char, P_char);
void MobStartFight(P_char, P_char);
void MobRetaliateRange(P_char, P_char);
void SetRememberArray(void);
void StompAttack(P_char);
void SweepAttack(P_char);
void clearRememberArray(void);
void mobact_memoryHandle(P_char);
void mobile_activity(void);
void restore_npc_spell (P_char);
void send_to_zone_func(int, int, const char *);
void start_npc_spell_mem(P_char, int);
void ZombieCombat(P_char, P_char);
void SkeletonCombat(P_char, P_char);
void SpectreCombat(P_char, P_char);
void WraithCombat(P_char, P_char);
void ShadowCombat(P_char, P_char);
void DriderCombat(P_char, P_char);
void PwormCombat(P_char, P_char);
void VampireCombat(P_char);
bool CastShamanSpell(P_char, P_char, int);
bool CastEtherSpell(P_char, P_char, int);
void clearMemory(P_char ch);
void give_proper_stat(P_char);
P_char pick_target(P_char, unsigned int);
int dummy_function(P_char, P_char, int, char*);
int babau_combat(P_char, P_char, int, char*);
int summon_new_demon(P_char, int);
bool should_teacher_move(P_char);
void startPvP( P_char ch, bool racewar );

/* mobconv.c */

void set_npc_multi(P_char);
void convertMob(P_char);
int GetFormType(P_char);

// mobpatrol
void event_patrol_move(P_char ch, P_char vict, P_obj obj, void *data);

/* modify.c */

void page_string_real(struct descriptor_data *d, char *str);
int replace_str(char **, char *, char *, int, int);
void format_text(char **, int, struct descriptor_data *, int);
void parse_action(int, char *, struct descriptor_data *);
char *stripcr(char *, const char *);
void string_add(struct descriptor_data *, char *);
void quad_arg(char *, int *, char *, int *, char *);
void do_string(P_char, char *, int);
void do_rename(P_char, char *, int);
char *one_word(char *, char *);
//void clear_help_index(struct help_index_element **list_head, const int help_size);
//struct help_index_element *build_help_index(FILE *, int *);
void night_watchman(void);
void check_reboot(void);
char *next_page(char *, struct descriptor_data *);
int count_pages(char *, struct descriptor_data *);
void paginate_string(char *, struct descriptor_data *);
void page_string(struct descriptor_data *, char *, int);
void show_string(struct descriptor_data *, const char *);

/* storage_lockers.c */

bool rename_locker(P_char ch, char *old_charname, char *new_charname);
void for_debug_print_char_list(P_char ch);

/* mount.c */

bool check_valid_ride(P_char);
void do_dismount(P_char, char *, int);
void do_mount(P_char, char *, int);
void stop_riding(P_char);
void do_hitch_vehicle(P_char, char *, int);
void do_unhitch_vehicle(P_char, char *, int);
void update_char_in_vehicle(P_obj obj);
void init_wagons(void);
int wagon(P_obj, P_char, int, char *);
int wagon_exit_room(int, P_char, int, char *);
int wagon_pull (P_char, int);
int num_char_in_vehicle(P_obj);
int stable_master (P_char, P_char, int, char *);
bool is_natural_mount(P_char ch, P_char mount);

/* nanny.c */

int tossHint( P_char ch );
void loadHints();
void approve_name(char *name);
void create_denied_file(const char *, char *);
int getNewPCidNumb(void);
void setNewPCidNumbfromFile(void);
char *statstr(int);
bool _parse_name(char *, char *);
bool has_avail_class(P_desc);
void display_classtable(P_desc);
extern int invitemode;
int display_avail_classes(P_desc, int);
int find_hometown(int, bool);
int find_starting_alignment(int, int);
int number_of_players(void);
ulong init_law_flags(P_char);
void add_stat_bonus(P_char, int, int);
void deny_name(char *);
void display_characteristics(P_desc);
void display_stats(P_desc);
void echo_off(P_desc);
void echo_on(P_desc);
int alt_hometown_check(P_char, int, int);
void enter_game(P_desc);
void find_starting_location(P_char, int);
void init_height_weight(P_char);
void load_obj_to_newbies(P_char);
void nanny(P_desc, char *);
void newby_announce(P_desc);
void print_recommended_action(P_desc);
void select_alignment(P_desc, char *);
void select_bonus(P_desc, char *);
void select_class(P_desc, char *);
void select_class_info(P_desc, char *);
void select_hometown(P_desc, char *);
void select_keepchar(P_desc, char *);
void select_main_menu(P_desc, char *);
void select_name(P_desc, char *, int);
void select_pwd(P_desc, char *);
void select_race(P_desc, char *);
void select_reroll(P_desc, char *);
void select_sex(P_desc, char *);
void select_terminal(P_desc, char *);
void set_char_size(P_char);
void set_char_height_weight(P_char);
void show_avail_classes(P_desc);
void show_avail_hometowns(P_desc);
void wimps_in_approve_queue(void);
bool valid_password(P_desc, char *);
bool pfile_exists(const char *, char *);
void event_autosave(P_char, P_char, P_obj, void*);
void update_ingame_racewar( int racewar );

/* new_combat.c */
/*
int pick_a_arm(P_char);
int pick_a_head(P_char);
int pick_a_limb(P_char);
int pick_a_body(P_char);
int pick_a_any(P_char);
int pick_a_leg(P_char);
*/
int calcChDamagetoVictwithInnateArmor(P_char ch, P_char victim, P_obj weap,
                     const int dam, const int loc, const int specific_body_loc,
                     int *damDefl, int *damAbsorb, int *innateArmorBlocks, int *weapDamage);
int calcChDamagetoVictwithArmor(P_char ch, P_char victim, P_obj weap,
                                const int dam, const int body_loc, const int specific_body_loc,
                                P_obj *armor_damaged, int *damDefl, int *damAbsorb,
                                int *armorBlocks, int *weapDamage);
void displayWeaponDamage(const int weap_type, const P_char ch, const P_obj object);
int applyDamagetoObject(P_char ch, P_obj object, const unsigned int dam);
void victParry(const P_char ch, const P_char victim, P_obj weapon,
               const int body_loc_target, const int parryrand, const int chance);
void victDodge(const P_char ch, const P_char victim, const int weaptype,
               const int body_loc_target, const int dodgerand, const int chance);
int getBodypartWeight(const P_char vict, const int loc);
void createBodypartinRoom(const int room, const int loc, const char *bodypart, const P_char vict);
/*int victLostLowerArm(P_char victim, const int loc);*/
int checkEffectsofLocDamage(P_char ch, P_char victim, const int loc, const int dam);
int victDamage(P_char ch, P_char victim, const int barehanded,
               const int weaptype, const int dam, const int loc);
void victMiss(const P_char ch, const P_char victim, const int weaptype, const int loc,
              const int barehanded);
int getBodyTarget(const P_char ch);
void displayArmorAbsorbedAllDamageMessage(
    const P_char ch, const P_char victim, const int barehanded, const int weaptype,
    const int body_loc_target, const P_obj armor_hit);

#ifdef NEW_COMBAT
int hit(P_char, P_char, P_obj, const int, const int, const int, const int);
void perform_violence(void);
#endif

int stat_shops(int, P_char, int, char *);

/* necromancy.c */
void event_pet_death(P_char ch, P_char victim, P_obj obj, void *data);
int setup_pet(P_char mob, P_char ch, int duration, int flag);
int can_raise_greater_draco(P_char ch);
void spell_corpseform(int, P_char, char *, int, P_char, P_obj);
void event_corpseform_wearoff(P_char, P_char, P_obj, void *);
void spell_undead_to_death(int, P_char, char*, int, P_char, P_obj);

/* new_combat_bpdam.c */

int victDamagedHead(P_char ch, P_char vict, const int loc, const int dam);
int victLostHead(P_char ch, P_char vict, const int loc, const int dam);
int victDamagedEye(P_char ch, P_char vict, const int loc, const int dam);
int victLostEye(P_char ch, P_char vict, const int loc, const int dam);
int victDamagedEar(P_char ch, P_char vict, const int loc, const int dam);
int victLostEar(P_char ch, P_char vict, const int loc, const int dam);
int victDamagedNeck(P_char ch, P_char vict, const int loc, const int dam);
int victLostNeck(P_char ch, P_char vict, const int loc, const int dam);
int victDamagedUpperTorso(P_char ch, P_char vict, const int loc, const int dam);
int victLostUpperTorso(P_char ch, P_char vict, const int loc, const int dam);
int victDamagedLowerTorso(P_char ch, P_char vict, const int loc, const int dam);
int victLostLowerTorso(P_char ch, P_char vict, const int loc, const int dam);
int victDamagedUpperArm(P_char ch, P_char vict, const int loc, const int dam);
int victLostUpperArm(P_char ch, P_char vict, const int loc, const int dam);
int victDamagedLowerArm(P_char ch, P_char vict, const int loc, const int dam);
int victLostLowerArm(P_char ch, P_char vict, const int loc, const int dam);
int victDamagedElbow(P_char ch, P_char vict, const int loc, const int dam);
int victLostElbow(P_char ch, P_char vict, const int loc, const int dam);
int victDamagedWrist(P_char ch, P_char vict, const int loc, const int dam);
int victLostWrist(P_char ch, P_char vict, const int loc, const int dam);
int victDamagedHand(P_char ch, P_char vict, const int loc, const int dam);
int victLostHand(P_char ch, P_char vict, const int loc, const int dam);
int victDamagedUpperLeg(P_char ch, P_char vict, const int loc, const int dam);
int victLostUpperLeg(P_char ch, P_char vict, const int loc, const int dam);
int victDamagedLowerLeg(P_char ch, P_char vict, const int loc, const int dam);
int victLostLowerLeg(P_char ch, P_char vict, const int loc, const int dam);
int victDamagedKnee(P_char ch, P_char vict, const int loc, const int dam);
int victLostKnee(P_char ch, P_char vict, const int loc, const int dam);
int victDamagedFoot(P_char ch, P_char vict, const int loc, const int dam);
int victLostFoot(P_char ch, P_char vict, const int loc, const int dam);
int victDamagedAnkle(P_char ch, P_char vict, const int loc, const int dam);
int victLostAnkle(P_char ch, P_char vict, const int loc, const int dam);
int victDamagedChin(P_char ch, P_char vict, const int loc, const int dam);
int victLostChin(P_char ch, P_char vict, const int loc, const int dam);
int victDamagedShoulder(P_char ch, P_char vict, const int loc, const int dam);
int victLostShoulder(P_char ch, P_char vict, const int loc, const int dam);
int victDamagedBody(P_char ch, P_char vict, const int loc, const int dam);
int victLostBody(P_char ch, P_char vict, const int loc, const int dam);
int victDamagedWing(P_char ch, P_char vict, const int loc, const int dam);
int victLostWing(P_char ch, P_char vict, const int loc, const int dam);
int victDamagedBeak(P_char ch, P_char vict, const int loc, const int dam);
int victLostBeak(P_char ch, P_char vict, const int loc, const int dam);
int victDamagedJoint(P_char ch, P_char vict, const int loc, const int dam);
int victLostJoint(P_char ch, P_char vict, const int loc, const int dam);
int victDamagedEyestalk(P_char ch, P_char vict, const int loc, const int dam);
int victLostEyestalk(P_char ch, P_char vict, const int loc, const int dam);

/* new_combat_user.c */

const char *getBodyLocColorPercent(const int percent);
void display_condition_body_loc(const P_char ch, const P_char vict,
                                const int loc);
void display_condition_paired_body_loc(const P_char ch, const P_char vict,
                                       const int loc, const int loc2);
void do_condition(P_char ch, char *argument, int cmd);

/* new_combat_util.c */

/*int getTopLoc(int);*/
void healCondition(P_char, int);
void regenCondition(P_char, int);
int racePhysHumanoid(const int race);
int racePhysFourArmedHumanoid(const int race);
int racePhysQuadruped(const int race);
int racePhysCentaur(const int race);
int racePhysBird(const int race);
int racePhysWingedHumanoid(const int race);
int racePhysWingedQuadruped(const int race);
int racePhysNoExtremities(const int race);
int racePhysInsectoid(const int race);
int racePhysArachnid(const int race);
int racePhysBeholder(const int race);
int getNumbBodyLocsbyRace(const int race);
int getNumbBodyLocsbyPhysType(const int physType);
int getPhysTypebyRace(const int race);
int bodyLocisUpperArms(const int physType, const int loc);
int bodyLocisUpperWrists(const int physType, const int loc);
int bodyLocisUpperHands(const int physType, const int loc);
int bodyLocisHead(const int physType, const int loc);
int bodyLocisChin(const int physType, const int loc);
int bodyLocisLeftEye(const int physType, const int loc);
int bodyLocisRightEye(const int physType, const int loc);
int bodyLocisEar(const int physType, const int loc);
int bodyLocisNeck(const int physType, const int loc);
int bodyLocisUpperTorso(const int physType, const int loc);
int bodyLocisLowerTorso(const int physType, const int loc);
int bodyLocisUpperShoulders(const int physType, const int loc);
int bodyLocisLegs(const int physType, const int loc);
int bodyLocisFeet(const int physType, const int loc);
int bodyLocisLowerArms(const int physType, const int loc);
int bodyLocisLowerWrists(const int physType, const int loc);
int bodyLocisLowerHands(const int physType, const int loc);
int bodyLocisHorseBody(const int physType, const int loc);
int bodyLocisRearLegs(const int physType, const int loc);
int bodyLocisRearFeet(const int physType, const int loc);

const char *getBodyLocStrn(const int loc, const P_char ch);
const int getBodyLocMaxHP(const P_char ch, const int loc);
int getBodyLocCurrHP(const P_char ch, const int loc);
int getCharToHitValClassandLevel(const P_char ch);
int getChartoHitSkillMod(const int wpn_skl_lvl);
int getVictimtoHitMod(const P_char ch, const P_char victim);
int bodyLocisLow(const int body_loc_target);
int bodyLocisMiddle(const int body_loc_target);
int bodyLocisHigh(const int body_loc_target);
int getBodyLocTargettingtoHitMod(const P_char ch, const P_char victim,
      const int body_loc_target, const int weaptype);
const char *getWeaponUseString(const int weaptype);
const char *getWeaponHitVerb(const int weaptype, const int tochar);
int getWeaponSkillNumb(const P_obj weapon);
int getCharWeaponSkillLevel(const P_char, const P_obj);
/*
int WeaponSkill_num(const P_char ch);
*/
int getNPCweaponSkillLevel(const P_char ch, const int wpn_skill);
int canCharDodgeParry(const P_char vict, const P_char attacker);
char targetisArms(const int body_loc);
char targetisHands(const int body_loc);
int calcChDamagetoVict(P_char ch, P_char victim, P_obj weap,
                       const int body_loc, const int wpn_skl,
                       const int wpn_skl_lvl, const int hit_type,
                       const int crit_hit);
int getCharParryVal(const P_char vict, const P_char attacker,
                    const int body_loc_target, const P_obj weapon);
const char *getParryEaseString(const int passedby, const int tochar);
int getCharDodgeVal(const P_char vict, const P_char attacker,
                    const int body_loc_target, const P_obj weapon);
const char *getDodgeEaseString(const int passedby, const int tochar);

/* new_skills.c */

void grapple_event (P_char, P_char);
bool check_skill_usage(P_char, int);
int CanDoFightMove(P_char, P_char);
int GetConditionModifier(P_char);
int CountNumFollowers(P_char);
int CountNumShadowers(P_char);
int MonkAcBonus(P_char);
int MonkDamage(P_char);
int MonkNumberOfAttacks(P_char);
int wornweight(P_char);
void FreeShadowedData(P_char, P_char);
void MonkSetSpecialDie(P_char);
void MoveShadower(P_char, int);
void StopShadowers(P_char);
void chant_buddha_palm(P_char, char *, int);
void chant_calm(P_char, char *, int);
void chant_diamond_soul(P_char, char *, int);
void chant_heroism(P_char, char *, int);
void chant_quivering_palm(P_char, char *, int);
void chant_regenerate(P_char, char *, int);
void chant_fist_of_dragon(P_char, char *, int);
void do_awareness(P_char, char *, int);
void do_bandage(P_char, char *, int);
void do_chant(P_char, char *, int);
void do_dragon_punch(P_char, char *, int);
void do_feign_death(P_char, char *, int);
void do_first_aid(P_char, char *, int);
void do_layhand(P_char, char *, int);
void do_shadow(P_char, char *, int);
void do_springleap(P_char, char *, int);
void do_summon_mount(P_char, char *, int);
void do_summon_warg(P_char, char *, int);
void do_summon_orc(P_char, char *, int);
void do_summon_generic(P_char, char *, int);
void do_ogre_roar(P_char, char *, int);
void do_carve(P_char, char *, int);
void do_bind(P_char, char *, int);
void do_unbind(P_char, char *, int);
void do_capture(P_char, char *, int);
void capture(P_char, P_char);
void do_appraise(P_char, char *, int);
void chant_chi_purge(int, P_char, char*, int, P_char, P_obj);
void do_chi(P_char, char*, int);
void displacement_event(P_char ch, P_char victim, P_obj obj, void *data);
void do_lotus(P_char, char*, int);
void lotus_event(P_char ch, P_char victim, P_obj obj, void *data);
void do_true_strike(P_char, char*, int);
void reconstruction(P_char);
void do_RemoveSpecTimer(P_char, char*, int);
int CountNumGreaterElementalFollowersInSameRoom(P_char);

/* objconv.c */

int GetCircle(int spl);
void convertObj(P_obj obj);
void randomizeObj(P_obj, int);

/* objmisc.c */

int get_weapon_msg(P_obj weapon);
int getWeaponDamType(const int weaptype);
float getMaterialDeflection(const int, const P_obj weap);
float getArmorDeflection(const P_obj armor, const P_obj weap);
float getMaterialAbsorbtion(const int, const P_obj weap);
float getArmorAbsorbtion(const P_obj armor, const P_obj weap);
int getMaterialMaxSP(const int material);
int getItemMaxSP(const P_obj item);
void setItemMaxSP(P_obj item);
int getItemCurrentSP(const P_obj item);

/* prompt.c */

void UpdateScreen(P_char, int);
void InitScreen(P_char);
char *make_bar(long, long, long);
void make_prompt(P_desc);

/* processlogin.c */

int InitConnectManager(int);    /* (int port) */
bool ProcessNewConnect(int);    /* (int socket) */
bool ConnectDone (void);
bool GetNewDesc(void);
int newConnection(int);
int newDescriptor(int);
int bannedsite(char *, int);

/* psionics.c */

void do_drain(P_char, char *, int);
void do_absorbe(P_char, char *, int);
void do_gith_neckbite(P_char, char *, int);
void spell_molecular_control(int, P_char, char*, int, P_char, P_obj);
void spell_molecular_agitation(int, P_char, char*, int, P_char, P_obj);
void spell_adrenaline_control(int, P_char, char*, int, P_char, P_obj);
void spell_aura_sight(int, P_char, char*, int, P_char, P_obj);
void spell_awe(int, P_char, char*, int, P_char, P_obj);
void spell_ballistic_attack(int, P_char, char*, int, P_char, P_obj);
void spell_biofeedback(int, P_char, char*, int, P_char, P_obj);
void spell_cell_adjustment(int, P_char, char*, int, P_char, P_obj);
void spell_combat_mind(int, P_char, char*, int, P_char, P_obj);
void spell_ego_blast(int, P_char, char*, int, P_char, P_obj);
void spell_control_flames(int, P_char, char*, int, P_char, P_obj);
void spell_mind_travel(int, P_char, char*, int, P_char, P_obj);
void spell_create_sound(int, P_char, char*, int, P_char, P_obj);
void spell_death_field(int, P_char, char*, int, P_char, P_obj);
void spell_detonate(int, P_char, char*, int, P_char, P_obj);
void spell_fire_aura(int, P_char, char*, int, P_char, P_obj);
void spell_disintegrate_object(int, P_char, char*, int, P_char, P_obj);
void spell_displacement(int, P_char, char*, int, P_char, P_obj);
void spell_domination(int, P_char, char*, int, P_char, P_obj);
void spell_ectoplasmic_form(int, P_char, char*, int, P_char, P_obj);
void spell_ego_whip(int, P_char, char*, int, P_char, P_obj);
void spell_energy_containment(int, P_char, char*, int, P_char, P_obj);
void spell_enhance_armor(int, P_char, char*, int, P_char, P_obj);
void spell_enhanced_strength(int, P_char, char*, int, P_char, P_obj);
void spell_enhanced_dexterity(int, P_char, char*, int, P_char, P_obj);
void spell_enhanced_agility(int, P_char, char*, int, P_char, P_obj);
void spell_enhanced_constitution(int, P_char, char*, int, P_char, P_obj);
void spell_enrage(int, P_char, char*, int, P_char, P_obj);
void spell_flesh_armor(int, P_char, char*, int, P_char, P_obj);
void spell_inertial_barrier(int, P_char, char*, int, P_char, P_obj);
void spell_inflict_pain(int, P_char, char*, int, P_char, P_obj);
void spell_intellect_fortress(int, P_char, char*, int, P_char, P_obj);
void spell_lend_health(int, P_char, char*, int, P_char, P_obj);
void spell_levitation(int, P_char, char *, int, P_char, P_obj);
void spell_confuse(int, P_char, char*, int, P_char, P_obj);
void spell_wormhole(int, P_char, char*, int, P_char, P_obj);
void spell_ether_portal(int, P_char, char*, int, P_char, P_obj);
void spell_mind_blank(int, P_char, char*, int, P_char, P_obj);
void spell_sight_link(int, P_char, char*, int, P_char, P_obj);
void spell_cannibalize(int, P_char, char*, int, P_char, P_obj);
void spell_tower_iron_will(int, P_char, char*, int, P_char, P_obj);
void spell_innate_blast(int, P_char, char *, int, P_char, P_obj);
void spell_ether_warp(int, P_char, char*, int, P_char, P_obj);
void spell_mental_anguish(int, P_char, char*, int, P_char, P_obj);
void spell_spinal_corruption(int, P_char, char*, int, P_char, P_obj);
void spell_memory_block(int, P_char, char*, int, P_char, P_obj);
void spell_psionic_cloud(int, P_char, char*, int, P_char, P_obj);
void spell_psychic_crush(int, P_char, char*, int, P_char, P_obj);
void spell_pyrokinesis(int, P_char, char*, int, P_char, P_obj);
void spell_ethereal_rift(int, P_char, char*, int, P_char, P_obj);
void spell_radial_navigation(int, P_char, char*, int, P_char, P_obj);
void spell_sever_link(int, P_char, char *, int, P_char, P_obj);
void spell_thought_beacon(int, P_char, char *, int, P_char, P_obj);
void spell_divine_blessing(int, P_char, char *, int, P_char, P_obj);
void spell_shadow_projection(int, P_char, char *, int, P_char, P_obj);
void spell_excogitate(int, P_char, char *, int, P_char, P_obj);
void event_psionic_wave_blast(P_char, P_char , P_obj , void *data);
void spell_psionic_wave_blast(int, P_char, char *, int, P_char, P_obj);
void spell_depart(int, P_char, char *, int, P_char, P_obj);
void spell_celerity(int, P_char, char *, int, P_char, P_obj);

/* quest.c */

int binary_search(int, int, int);
int find_quester_id(int);
int partition(int, int);
int quester(P_char, P_char, int, char *);
void assign_the_questers(void);
void boot_the_quests(void);
void quick_sort_quest_index(int, int);
void tell_quest(int, P_char);
float getQuestTropy(int);
bool has_quest(P_char);
bool has_quest_ask(int qi);
bool has_quest_complete(int qi);

/* randobj.c */

void do_randobj(P_char, char *, int);
P_obj createRandomItem(P_char, P_char, int, int, int);

/* random.c */

//char *initstate(unsigned int, char *, int);
//char *setstate(char *);
int irand(int);
long erandom(void);
long lrand(long num);
void esrand(unsigned int);
void setrandom(void);

/*  arena.c */
void initialize_arena(void);
int arena_team_count(int team);
void arena_char_spawn( P_char ch );
int arena_id(P_char ch);
int arena_player(P_char ch);
int arena_team(P_char ch);
void arena_activity();
void show_stats_to_char(P_char ch);

/* shop.c */

P_obj accept_gem_for_debt(P_char, P_char, int);
void restore_shopkeepers (void);
void push(struct stack_data *stack, int pushval);
int topp(struct stack_data *stack);
int pop(struct stack_data *stack);
void evaluate_operation(struct stack_data *ops, struct stack_data *vals);
int find_oper_num(char token);
int evaluate_expression(P_obj obj, char *expr);
int is_ok(P_char keeper, P_char ch, int shop_nr);
int same_obj(P_obj obj1, P_obj obj2);
char *times_message(P_obj obj, char *name, int num);
P_obj get_slide_obj_vis(P_char ch, char *name, P_obj list);
P_obj get_hash_obj_vis(P_char ch, char *name, P_obj list);
P_obj get_purchase_obj(P_char ch, char *arg, P_char keeper, int shop_nr, int msg);
int trade_with(P_obj item, int shop_nr, char repairing);
int shop_producing(P_obj item, int shop_nr);
P_obj get_selling_obj(P_char ch, char *name, P_char keeper, int shop_nr, int msg, char repairing);
void shopping_buy(char *arg, P_char ch, P_char keeper, int shop_nr);
void shopping_sell(char *arg, P_char ch, P_char keeper, int shop_nr);
void shopping_value(char *arg, P_char ch, P_char keeper, int shop_nr);
void shopping_list(char *arg, P_char ch, P_char keeper, int shop_nr);
void shopping_kill(char *arg, P_char ch, P_char keeper, int shop_nr);
void shopping_repair(char *arg, P_char ch, P_char keeper, int shop_nr);
int shop_keeper(P_char keeper, P_char ch, int cmd, char *arg);
int add_to_list(struct shop_buy_data *list, int type, int *len, int *val);
int read_type_list(FILE *shop_f, struct shop_buy_data *list, int max);
void boot_the_shops(void);
void assign_the_shopkeepers(void);

/* signals.c */

/*
void checkpointing(void);
void fork_request(void);
void hupsig(void);
void logsig(void);
void reaper(void);
void shutdown_notice(void);
void shutdown_request(void);
*/
void signal_setup(void);

/* sojourn.c */

struct timeval timediff(struct timeval *, struct timeval *);
void AddDeadChar(P_char);
void AddDeadObj(P_obj);
void game_loop(int, int);
void game_up_message(int);
void run_the_game(int, int);

/* sparser.c */

bool parse_spell_arguments(P_char ch, struct spell_target_data* data, char *argument);
bool NewSaves(P_char, int, int);
bool cast_common(P_char, char *);
bool cast_common_generic(P_char, int);
bool circle_follow(P_char, P_char);
void event_falling_char(P_char ch, P_char victim, P_obj obj, void *data);
bool falling_char(P_char, const int, bool caller_is_event);
void event_falling_obj(P_char ch, P_char victim, P_obj obj, void *data);
bool falling_obj(P_obj, int, bool caller_is_event);
bool saves_spell(P_char, int);
char *skip_spaces(char *);
int SpareCastStack(void);
int SpellCastChance(P_char, int);
int SpellCastStack(void);
int SpellCastTime(P_char, int);
int find_save(P_char, int);
int kala_spell(int);
int skilltype_of_spell(int);
void KnockOut(P_char, int);
void NukeRedunantSpellcast(P_char);
void SpellCastProcess(P_char, struct spellcast_datatype *);
void SpellCastShow(P_char, int);
void StopCasting(P_char);
void add_follower(P_char, P_char);
void petrestore(P_char, char *);
void affect_update(void);
void assign_spell_pointers(void);
void die_follower(P_char);
void do_cast(P_char, char *, int);
void do_will(P_char, char *, int);
void do_powercast(P_char, char *, int);
void nuke_spellcast(struct spellcast_datatype *);
void say_spell(P_char, int);
void short_affect_update(void);
void short_pc_update(void);
void stop_all_followers(P_char);
void stop_follower(P_char);
bool checkTotem (P_char ch, P_obj obj, int skill);
bool hasTotem (P_char ch, int skill);
void wear_off_message(P_char, struct affected_type*);
bool has_innate(P_char, int);
bool check_innate_time(P_char, int, int duration = 0);
const char* get_god_name(P_char);


/* spec.assign.c */

void assign_mobiles(void);
void assign_objects(void);
void assign_rooms(void);
void item_procs(void);
void room_procs(void);

/* specials.c */

void return_from_poly_obj(P_char);
P_obj find_key(P_char, int);
bool check_get_disarmed_obj(P_char, P_char, P_obj);
bool transact(P_char, P_obj, P_char, int);
int OutlawAggro(P_char, const char *);
long pow10(long);
void exec_social(P_char, char *, int, int *, void **);
void firesector(P_char);
void npc_steal(P_char, P_char);
void underwatersector(P_char);
void swimming_char(P_char);
void negsector(P_char);

/* spells.c */

/* items prefixed with spell_ have been cleaned up and their function definitions are found now in magic.c
 * these will gradually obsolete the spell_ declarations in the magic.c section of this file
 *  May 05 2003 dbb */

int has_soulbind(P_char ch);
void do_soulbind(P_char ch, char *argument, int cmd);
void load_soulbind(P_char ch);
void remove_soulbind(P_char ch);

void spell_earthen_maul(int, P_char, char *, int, P_char, P_obj);
void spell_acid_stream(int, P_char, char *, int, P_char, P_obj);
void spell_anti_magic_ray(int, P_char, char *, int, P_char, P_obj);
void spell_prismatic_ray(int, P_char, char *, int, P_char, P_obj);
void spell_divine_fury(int, P_char, char *, int, P_char, P_obj);
void spell_stornogs_lowered_magical_res(int, P_char, char *, int, P_char, P_obj);
void spell_command(int, P_char, char *, int, P_char, P_obj);
void spell_mass_barkskin(int, P_char, char *, int, P_char, P_obj);
void spell_holy_light(int, P_char, char *, int, P_char, P_obj);
void spell_mage_flame(int, P_char, char *, int, P_char, P_obj);
void spell_globe_of_darkness(int, P_char, char *, int, P_char, P_obj);
void spell_deflect(int, P_char, char *, int, P_char, P_obj);
void spell_full_harm(int, P_char, char *, int, P_char, P_obj);
void spell_acid_blast(int, P_char, char *, int, P_char, P_obj);
void spell_age(int, P_char, char *, int, P_char, P_obj);
void spell_air_form(int, P_char, char *, int, P_char, P_obj);
void spell_airy_water(int, P_char, char *, int, P_char, P_obj);
void spell_animate_dead(int, P_char, char *, int, P_char, P_obj);
void spell_armor(int, P_char, char *, int, P_char, P_obj);
void spell_virtue(int, P_char, char *, int, P_char, P_obj);
void spell_nova(int, P_char, char *, int, P_char, P_obj);
void spell_spore_burst(int, P_char, char *, int, P_char, P_obj);
void spell_siren_song(int, P_char, char *, int, P_char, P_obj);
void spell_harmonic_resonance(int, P_char, char *, int, P_char, P_obj);
void spell_elemental_aura(int, P_char, char *, int, P_char, P_obj);
void spell_summon_insects(int, P_char, char *, int, P_char, P_obj);
void spell_consecrate_land(int, P_char, char *, int, P_char, P_obj);
void spell_binding_wind(int, P_char, char *, int, P_char, P_obj);
void spell_wind_tunnel(int, P_char, char *, int, P_char, P_obj);
void spell_disease(int, P_char, char *, int, P_char, P_obj);
void spell_endurance(int, P_char, char *, int, P_char, P_obj);
void spell_aid(int, P_char, char *, int, P_char, P_obj);
void spell_wandering_woods(int, P_char, char *, int, P_char, P_obj);
void spell_flame_blade(int, P_char, char *, int, P_char, P_obj);
void spell_serendipity(int, P_char, char *, int, P_char, P_obj);
void spell_tranquility(int, P_char, char *, int, P_char, P_obj);
void spell_fortitude(int, P_char, char *, int, P_char, P_obj);
void spell_unholy_wind(int, P_char, char *, int, P_char, P_obj);
void spell_shadow_vision(int, P_char, char *, int, P_char, P_obj);
void spell_animal_vision(int, P_char, char *, int, P_char, P_obj);
void spell_tree(int, P_char, char *, int, P_char, P_obj);
void cast_earthen_tomb(int, P_char, char *, int, P_char, P_obj);
void spell_entropy_storm(int, P_char, char *, int, P_char, P_obj);
void cast_life_ward(int, P_char, char *, int, P_char, P_obj);
void spell_shadow_gate(int, P_char, char *, int, P_char, P_obj);
void spell_negative_energy_barrier(int, P_char, char *, int, P_char, P_obj);
void cast_nether_gate(int, P_char, char *, int, P_char, P_obj);
void spell_alter_energy_polarity(int, P_char, char *, int, P_char, P_obj);
void spell_dispel_lifeforce(int, P_char, char *, int, P_char, P_obj);
void spell_negative_energy_vortex(int, P_char, char *, int, P_char, P_obj);
void spell_shadow_aura(int, P_char, char *, int, P_char, P_obj);
void spell_channel_negative_energy(int, P_char, char *, int, P_char, P_obj);
void spell_nether_touch(int, P_char, char *, int, P_char, P_obj);
void spell_frostbite(int, P_char, char *, int, P_char, P_obj);
void spell_devitalize(int, P_char, char *, int, P_char, P_obj);
void spell_purge_living(int, P_char, char *, int, P_char, P_obj);
void spell_invoke_negative_energy(int, P_char, char *, int, P_char, P_obj);
void spell_lifelust(int, P_char, char *, int, P_char, P_obj);
void spell_sense_holiness(int, P_char, char *, int, P_char, P_obj);
void spell_agility(int, P_char, char *, int, P_char, P_obj);
void cast_awareness(int, P_char, char *, int, P_char, P_obj);
void spell_barkskin(int, P_char, char *, int, P_char, P_obj);
void spell_bigbys_clenched_fist(int, P_char, char *, int, P_char, P_obj);
void spell_rope_trick(int, P_char, char *, int, P_char, P_obj);
void spell_bigbys_crushing_hand(int, P_char, char *, int, P_char, P_obj);
void spell_bless(int, P_char, char *, int, P_char, P_obj);
void spell_blindness(int, P_char, char *, int, P_char, P_obj);
void cast_call_lightning(int, P_char, char *, int, P_char, P_obj);
void spell_cause_critical(int, P_char, char *, int, P_char, P_obj);
void spell_cause_light(int, P_char, char *, int, P_char, P_obj);
void spell_cause_serious(int, P_char, char *, int, P_char, P_obj);
void spell_powercasttning(int, P_char, char *, int, P_char, P_obj);
void spell_charm_person(int, P_char, char *, int, P_char, P_obj);
void spell_chill_touch(int, P_char, char *, int, P_char, P_obj);
void spell_clairvoyance(int, P_char, char *, int, P_char, P_obj);
void spell_coldshield(int, P_char, char *, int, P_char, P_obj);
void spell_color_spray(int, P_char, char *, int, P_char, P_obj);
void spell_command_horde(int, P_char, char *, int, P_char, P_obj);
void spell_command_undead(int, P_char, char *, int, P_char, P_obj);
void spell_comprehend_languages(int, P_char, char *, int, P_char, P_obj);
void spell_cone_of_cold(int, P_char, char *, int, P_char, P_obj);
void spell_conjour_elemental(int, P_char, char *, int, P_char, P_obj);
void spell_conjour_greater_elemental(int, P_char, char *, int, P_char, P_obj);
void spell_call_woodland_beings(int, P_char, char *, int, P_char, P_obj);
void spell_mirror_image(int, P_char, char *, int, P_char, P_obj);
void spell_continual_light(int, P_char, char *, int, P_char, P_obj);
void cast_control_weather(int, P_char, char *, int, P_char, P_obj);
void spell_call_titan(int, P_char, char *, int, P_char, P_obj);
void spell_create_dracolich(int, P_char, char *, int, P_char, P_obj);
void spell_create_food(int, P_char, char *, int, P_char, P_obj);
void spell_pulchritude(int, P_char, char *, int, P_char, P_obj);
void spell_create_spring(int, P_char, char *, int, P_char, P_obj);
void spell_creeping(int, P_char, char *, int, P_char, P_obj);
void spell_cdoom(int, P_char, char *, int, P_char, P_obj);
void spell_cure_blind(int, P_char, char *, int, P_char, P_obj);
void spell_cure_disease(int, P_char, char *, int, P_char, P_obj);
void spell_cure_critic(int, P_char, char *, int, P_char, P_obj);
void spell_cure_light(int, P_char, char *, int, P_char, P_obj);
void spell_cure_serious(int, P_char, char *, int, P_char, P_obj);
void spell_curse(int, P_char, char *, int, P_char, P_obj);
void spell_cyclone(int, P_char, char *, int, P_char, P_obj);
void spell_doom_blade(int, P_char, char *, int, P_char, P_obj);
void spell_darkness(int, P_char, char *, int, P_char, P_obj);
void spell_detect_evil(int, P_char, char *, int, P_char, P_obj);
void spell_detect_good(int, P_char, char *, int, P_char, P_obj);
void spell_detect_invisibility(int, P_char, char *, int, P_char, P_obj);
void spell_detect_magic(int, P_char, char *, int, P_char, P_obj);
void spell_detect_poison(int, P_char, char *, int, P_char, P_obj);
void spell_dexterity(int, P_char, char *, int, P_char, P_obj);
void spell_dimension_door(int, P_char, char *, int, P_char, P_obj);
void spell_disintegrate(int, P_char, char *, int, P_char, P_obj);
void spell_dispel_evil(int, P_char, char *, int, P_char, P_obj);
void spell_dispel_good(int, P_char, char *, int, P_char, P_obj);
void spell_dispel_invisible(int, P_char, char *, int, P_char, P_obj);
void spell_dispel_magic(int, P_char, char *, int, P_char, P_obj);
void spell_dread_wave(int, P_char, char *, int, P_char, P_obj);
void spell_earthquake(int, P_char, char *, int, P_char, P_obj);
void spell_embalm(int, P_char, char *, int, P_char, P_obj);
void spell_mass_embalm(int, P_char, char *, int, P_char, P_obj);
void spell_enchant_weapon(int, P_char, char *, int, P_char, P_obj);
void spell_ethereal_grounds(int, P_char, char *, int, P_char, P_obj);
void spell_unmaking(int, P_char, char *, int, P_char, P_obj);
void spell_flamestrike(int, P_char, char *, int, P_char, P_obj);
void spell_full_heal(int, P_char, char *, int, P_char, P_obj);
void cast_gate(int, P_char, char *, int, P_char, P_obj);
void spell_lodestone_vision(int, P_char, char *, int, P_char, P_obj);
void spell_haste(int, P_char, char *, int, P_char, P_obj);
void spell_heal(int, P_char, char *, int, P_char, P_obj);
void spell_natures_touch(int, P_char, char *, int, P_char, P_obj);
void spell_mend_soul(int, P_char, char *, int, P_char, P_obj);
void spell_heal_undead(int, P_char, char *, int, P_char, P_obj);
void spell_greater_heal_undead(int, P_char, char *, int, P_char, P_obj);
void spell_identify(int, P_char, char *, int, P_char, P_obj);


void spell_perm_increase_str(int, P_char, char *, int, P_char, P_obj);
void spell_perm_increase_agi(int, P_char, char *, int, P_char, P_obj);
void spell_perm_increase_dex(int, P_char, char *, int, P_char, P_obj);
void spell_perm_increase_con(int, P_char, char *, int, P_char, P_obj);
void spell_perm_increase_luck(int, P_char, char *, int, P_char, P_obj);
void spell_perm_increase_pow(int, P_char, char *, int, P_char, P_obj);
void spell_perm_increase_int(int, P_char, char *, int, P_char, P_obj);
void spell_perm_increase_wis(int, P_char, char *, int, P_char, P_obj);
void spell_perm_increase_cha(int, P_char, char *, int, P_char, P_obj);

void spell_reveal_true_name(int, P_char, char *, int, P_char, P_obj);
void spell_infravision(int, P_char, char *, int, P_char, P_obj);
void spell_invisibility(int, P_char, char *, int, P_char, P_obj);
void spell_improved_invisibility(int, P_char, char *, int, P_char, P_obj);
void cast_locate_object(int, P_char, char *, int, P_char, P_obj);
void spell_lore(int, P_char, char *, int, P_char, P_obj);
void spell_major_paralysis(int, P_char, char *, int, P_char, P_obj);
void spell_mass_invisibility(int, P_char, char *, int, P_char, P_obj);
void cast_minor_creation(int, P_char, char *, int, P_char, P_obj);
void spell_moonwell(int, P_char, char *, int, P_char, P_obj);
void cast_plane_shift(int, P_char, char *, int, P_char, P_obj);
void spell_poison(int, P_char, char *, int, P_char, P_obj);
void spell_preserve(int, P_char, char *, int, P_char, P_obj);
void spell_mass_preserve(int, P_char, char *, int, P_char, P_obj);
void spell_prot_from_undead(int, P_char, char *, int, P_char, P_obj);
void spell_prot_undead(int, P_char, char *, int, P_char, P_obj);
void spell_protection_from_living(int, P_char, char *, int, P_char, P_obj);
void spell_protection_from_animals(int, P_char, char *, int, P_char, P_obj);
void spell_animal_friendship(int, P_char, char *, int, P_char, P_obj);
void spell_protection_from_acid(int, P_char, char *, int, P_char, P_obj);
void spell_protection_from_cold(int, P_char, char *, int, P_char, P_obj);
void spell_protection_from_evil(int, P_char, char *, int, P_char, P_obj);
void spell_protection_from_fire(int, P_char, char *, int, P_char, P_obj);
void spell_protection_from_gas(int, P_char, char *, int, P_char, P_obj);
void spell_protection_from_good(int, P_char, char *, int, P_char, P_obj);
void spell_protection_from_lightning(int, P_char, char *, int, P_char, P_obj);
void spell_accel_healing(int, P_char, char *, int, P_char, P_obj);
void spell_relocate(int, P_char, char *, int, P_char, P_obj);
void spell_dark_compact(int, P_char, char *, int, P_char, P_obj);
void spell_remove_curse(int, P_char, char *, int, P_char, P_obj);
void spell_remove_poison(int, P_char, char *, int, P_char, P_obj);
void spell_resurrect(int, P_char, char *, int, P_char, P_obj);
void spell_lesser_resurrect(int, P_char, char *, int, P_char, P_obj);
void spell_sense_life(int, P_char, char *, int, P_char, P_obj);
void spell_sense_follower(int, P_char, char *, int, P_char, P_obj);
void spell_slow(int, P_char, char *, int, P_char, P_obj);
void spell_stone_skin(int, P_char, char *, int, P_char, P_obj);
void spell_ironwood(int, P_char, char *, int, P_char, P_obj);
void spell_strength(int, P_char, char *, int, P_char, P_obj);
void spell_summon(int, P_char, char *, int, P_char, P_obj);
void spell_group_teleport(int, P_char, char *, int, P_char, P_obj);
void spell_teleport(int, P_char, char *, int, P_char, P_obj);
void cast_teleimage(int, P_char, char *, int, P_char, P_obj);
void spell_grow_spike(int, P_char, char *, int, P_char, P_obj);
void spell_entangle(int, P_char, char *, int, P_char, P_obj);
void spell_vampiric_touch(int, P_char, char *, int, P_char, P_obj);
void spell_ventriloquate(int, P_char, char *, int, P_char, P_obj);
void spell_wizard_eye(int, P_char, char *, int, P_char, P_obj);
void spell_word_of_recall(int, P_char, char *, int, P_char, P_obj);
void spell_group_recall(int, P_char, char *, int, P_char, P_obj);
void spell_group_stone_skin(int, P_char, char *, int, P_char, P_obj);
void spell_group_haste(int, P_char, char *, int, P_char, P_obj);
void spell_miracle(int, P_char, char *, int, P_char, P_obj);
void cast_channel(int, P_char, char *, int, P_char, P_obj);
void spell_elemental_form(int, P_char, char *, int, P_char, P_obj);
void spell_wall_of_flames(int, P_char, char *, int, P_char, P_obj);
void spell_wall_of_ice(int, P_char, char *, int, P_char, P_obj);
void spell_wall_of_stone(int, P_char, char *, int, P_char, P_obj);
void spell_wall_of_iron(int, P_char, char *, int, P_char, P_obj);
void spell_wall_of_force(int, P_char, char *, int, P_char, P_obj);
void spell_wall_of_bones(int, P_char, char *, int, P_char, P_obj);
void spell_wall_of_fog(int, P_char, char *, int, P_char, P_obj);
void spell_wall_of_bones(int, P_char, char *, int, P_char, P_obj);
void spell_water_to_life(int, P_char, char *, int, P_char, P_obj);
void cast_lightning_curtain(int, P_char, char *, int, P_char, P_obj);
void spell_soulshield(int, P_char, char *, int, P_char, P_obj);
void spell_holy_sacrifice(int, P_char, char *, int, P_char, P_obj);
void spell_battle_ecstasy(int, P_char, char *, int, P_char, P_obj);
void spell_charm_animal(int, P_char, char *, int, P_char, P_obj);
void spell_mass_heal(int, P_char, char *, int, P_char, P_obj);
void spell_prayer(int, P_char, char *, int, P_char, P_obj);
void spell_true_seeing(int, P_char, char *, int, P_char, P_obj);
void spell_natures_blessing(int, P_char, char*, int, P_char, P_obj);
void spell_animal_growth(int, P_char, char *, int, P_char, P_obj);
void spell_enlarge(int, P_char, char *, int, P_char, P_obj);
void spell_reduce(int, P_char, char *, int, P_char, P_obj);
void spell_dazzle(int, P_char, char *, int, P_char, P_obj);
void spell_blur(int, P_char, char *, int, P_char, P_obj);
void cast_prismatic_cube(int, P_char, char *, int, P_char, P_obj);
void spell_judgement(int, P_char, char *, int, P_char, P_obj);
void spell_apocalypse(int, P_char, char *, int, P_char, P_obj);
void spell_sanctuary(int, P_char, char *, int, P_char, P_obj);
void spell_hellfire(int, P_char, char *, int, P_char, P_obj);
void spell_battletide(int, P_char, char *, int, P_char, P_obj);
void spell_starshell(int, P_char, char *, int, P_char, P_obj);
void spell_cloak_of_fear(int, P_char, char *, int, P_char, P_obj);
void spell_vampire(int, P_char, char *, int, P_char, P_obj);
void spell_raise_spectre(int, P_char, char *, int, P_char, P_obj);
void spell_raise_wraith(int, P_char, char *, int, P_char, P_obj);
void spell_raise_shadow(int, P_char, char *, int, P_char, P_obj);
void spell_raise_vampire(int, P_char, char *, int, P_char, P_obj);
void spell_raise_lich(int, P_char, char *, int, P_char, P_obj);
void spell_call_asura(int, P_char, char *, int, P_char, P_obj);
void spell_call_bralani(int, P_char, char *, int, P_char, P_obj);
void spell_call_deva(int, P_char, char *, int, P_char, P_obj);
void spell_call_knight(int, P_char, char *, int, P_char, P_obj);
void spell_call_liberator(int, P_char, char *, int, P_char, P_obj);
void spell_call_archon(int, P_char, char *, int, P_char, P_obj);

void spell_pass_without_trace(int, P_char, char *, int, P_char, P_obj);
void spell_spawn(int, P_char, char *, int, P_char, P_obj);
void cast_web(int, P_char, char *, int, P_char, P_obj);
void spawn_raise_undead(P_char, P_char, P_obj);
void mummify(P_char, P_char, P_obj);
void spell_negative_concussion_blast(int, P_char, char *, int, P_char, P_obj);
void spell_comet(int, P_char, char *, int, P_char, P_obj);
void spell_call_avatar(int, P_char, char *, int, P_char, P_obj);
void spell_create_greater_dracolich(int, P_char, char *, int, P_char, P_obj);
void spell_create_golem(int, P_char, char *, int, P_char, P_obj);
void create_golem(int level, P_char ch, P_char victim, P_obj obj, int which_type);
void spell_slashing_darkness(int, P_char, char *, int, P_char, P_obj);
void spell_heavens_aid(int, P_char, char, int, P_char, P_obj);
void spell_aid_of_the_heavens(int, P_char, char *, int, P_char, P_obj);
void spell_summon_ghasts(int, P_char, char *, int, P_char, P_obj);
void spell_ghastly_touch(int, P_char, char *, int, P_char, P_obj);
void event_aid_of_the_heavens(P_char, P_char, P_obj, void *);
void event_summon_ghasts(P_char, P_char, P_obj, void *);
void spell_undead_to_death(int, P_char, char *, int, P_char, P_obj);

/* ranger healing blade */
void spell_healing_blade(int, P_char, char *, int, P_char, P_obj);
void spell_windstrom_blessing(int, P_char, char *, int, P_char, P_obj);

/* specialization */
void spell_group_heal(int, P_char, char *, int, P_char, P_obj);
void cast_area_resurrect(int, P_char, char *, int, P_char, P_obj);
void spell_chaos_volley(int, P_char, char *, int, P_char, P_obj);
void spell_chaos_shield(int, P_char, char *, int, P_char, P_obj);
int parse_chaos_shield(P_char, P_char);
void spell_knock(int, P_char, char *, int, P_char, P_obj);


void cast_transmute_mud_rock(int, P_char, char *, int, P_char, P_obj);
void event_mud_rock(P_char, P_char, P_obj, void *);
void event_mud_rock_bye(P_char, P_char, P_obj, void *);
void cast_transmute_rock_mud(int, P_char, char *, int, P_char, P_obj);
void event_rock_mud(P_char, P_char, P_obj, void *);
void event_rock_mud_bye(P_char, P_char, P_obj, void *);
void cast_transmute_mud_water(int, P_char, char *, int, P_char, P_obj);
void event_mud_water(P_char, P_char, P_obj, void *);
void event_mud_water_bye(P_char, P_char, P_obj, void *);
void cast_transmute_water_mud(int, P_char, char *, int, P_char, P_obj);
void event_water_mud(P_char, P_char, P_obj, void *);
void event_water_mud_bye(P_char, P_char, P_obj, void *);
void cast_transmute_mud_rock(int, P_char, char *, int, P_char, P_obj);
void event_mud_rock(P_char, P_char, P_obj, void *);
void event_mud_rock_bye(P_char, P_char, P_obj, void *);
void cast_transmute_water_air(int, P_char, char *, int, P_char, P_obj);
void event_water_air(P_char, P_char, P_obj, void *);
void event_water_air_bye(P_char, P_char, P_obj, void *);
void cast_transmute_air_water(int, P_char, char *, int, P_char, P_obj);
void event_air_water(P_char, P_char, P_obj, void *);
void event_air_water_bye(P_char, P_char, P_obj, void *);
void cast_transmute_rock_lava(int, P_char, char *, int, P_char, P_obj);
void event_rock_lava(P_char, P_char, P_obj, void *);
void event_rock_lava_bye(P_char, P_char, P_obj, void *);
void cast_transmute_lava_rock(int, P_char, char *, int, P_char, P_obj);
void event_lava_rock(P_char, P_char, P_obj, void *);
void event_lava_rock_bye(P_char, P_char, P_obj, void *);
void cast_depressed_earth(int, P_char, char *, int, P_char, P_obj);
void event_depressed_earth(P_char, P_char, P_obj, void *);
void event_depressed_earth_bye(P_char, P_char, P_obj, void *);

/* druid specs */
void cast_grow(int, P_char, char *, int, P_char, P_obj);
void event_grow(P_char, P_char, P_obj, void *);
void event_grow_bye(P_char, P_char, P_obj, void *);
void cast_vines(int, P_char, char *, int, P_char, P_obj);
void event_spike_growth(P_char, P_char, P_obj, void *);
void cast_spike_growth(int, P_char, char *, int, P_char, P_obj);
void event_awaken_forest(P_char, P_char, P_obj, void *);
void event_spore_burst(P_char, P_char, P_obj, void *);
void cast_awaken_forest(int, P_char, char *, int, P_char, P_obj);
void cast_hurricane(int, P_char, char *, int, P_char, P_obj);
void cast_storm_shield(int, P_char, char *, int, P_char, P_obj);
void cast_bloodstone(int, P_char, char *, int, P_char, P_obj);

/* ethermancer spells */
void spell_vapor_armor(int, P_char, char *, int, P_char, P_obj);
void spell_faerie_sight(int, P_char, char *, int, P_char, P_obj);
void spell_cold_snap(int, P_char, char *, int, P_char, P_obj);
void spell_path_of_frost(int, P_char, char *, int, P_char, P_obj);
void spell_mass_fly(int, P_char, char *, int, P_char, P_obj);
void spell_wind_blade(int, P_char, char *, int, P_char, P_obj);
void spell_windwalk(int, P_char, char *, int, P_char, P_obj);
void spell_frost_beacon(int, P_char, char *, int, P_char, P_obj);
void spell_vapor_strike(int, P_char, char *, int, P_char, P_obj);
void spell_frost_bolt(int, P_char, char *, int, P_char, P_obj);
void spell_arctic_blast(int, P_char, char *, int, P_char, P_obj);
void spell_antimatter_collision(int, P_char, char *, int, P_char, P_obj);
void spell_ethereal_form(int, P_char, char *, int, P_char, P_obj);
void spell_conjure_air(int, P_char, char *, int, P_char, P_obj);
void spell_storm_empathy(int, P_char, char *, int, P_char, P_obj);
void spell_greater_ethereal_recharge(int level, P_char ch, char *arg, int type, P_char victim, P_obj tar_obj);
void spell_ethereal_recharge(int, P_char, char *, int, P_char, P_obj);
void spell_arcane_whirlwind(int, P_char, char *, int, P_char, P_obj);
void spell_forked_lightning(int, P_char, char *, int, P_char, P_obj);
void spell_purge(int, P_char, char *, int, P_char, P_obj);
void spell_induce_tupor(int, P_char, char *, int, P_char, P_obj);
void spell_tempest_terrain(int, P_char, char *, int, P_char, P_obj);
void event_tempest_terrain(P_char ch, P_char victim, P_obj obj, void *data);
void event_tempest_terrain_bye(P_char ch, P_char victim, P_obj obj, void *data);
void event_tupor_wake(P_char ch, P_char victim, P_obj obj, void *data);
void spell_cosmic_vacuum(int, P_char, char *, int, P_char, P_obj);
void spell_supernova(int, P_char, char *, int, P_char, P_obj);
void spell_ethereal_discharge(int, P_char, char *, int, P_char, P_obj);
void spell_planetary_alignment(int, P_char, char *, int, P_char, P_obj);
void spell_polar_vortex(int, P_char, char *, int, P_char, P_obj);
void spell_ethereal_travel(int, P_char, char *, int, P_char, P_obj);
void spell_cosmic_rift(int, P_char, char *, int, P_char, P_obj);
void spell_static_discharge(int, P_char, char *, int, P_char, P_obj);
void event_static_discharge(P_char, P_char, P_obj, void *);
void static_discharge(P_char, P_char);
void spell_mirage(int level, P_char ch, char *arg, int type, P_char victim, P_obj obj);
void event_mirage(P_char, P_char,P_obj, void *);
void spell_razor_wind(int, P_char, char *, int, P_char, P_obj);
void spell_single_razor_wind(int, P_char, char *, int, P_char, P_obj);
void event_razor_wind(P_char, P_char, P_obj, void *);
void spell_conjure_void_elemental(int, P_char, char *, int, P_char, P_obj);
void spell_conjure_ice_elemental(int, P_char, char *, int, P_char, P_obj);
void spell_iceflow_armor(int, P_char, char *, int, P_char, P_obj);
void spell_negative_feedback_barrier(int, P_char, char *, int, P_char, P_obj);
void spell_etheric_gust(int, P_char, char *, int, P_char, P_obj);
void spell_ice_spikes(int, P_char, char *, int, P_char, P_obj);
void spell_wall_of_air(int, P_char, char *, int, P_char, P_obj);

void event_change_yzar_race(P_char ch, P_char victim, P_obj obj, void *data);

/* spells.c */
void spell_single_doom_aoe(int, P_char, char *, int, P_char, P_obj);
void spell_curse_of_yzar(int, P_char, char *, int, P_char, P_obj);
void spell_rest(int, P_char, char *, int, P_char, P_obj);

/* ssl.c */
void ssl_read_cert(void);
gnutls_session_t ssl_new(int);
int ssl_negotiate(gnutls_session_t);
void ssl_close(gnutls_session_t);

/* sspells.c */
void cast_restore_item(int, P_char, char *, int, P_char, P_obj);

/* sillusionist.c */

int is_illusion_char(P_char ch);
int is_illusion_obj(P_obj obj);

void spell_phantom_armor(int, P_char, char*, int, P_char, P_obj);
void spell_shadow_monster(int, P_char, char*, int, P_char, P_obj);
void spell_insects(int, P_char, char*, int, P_char, P_obj);
void spell_illusionary_wall(int, P_char, char*, int, P_char, P_obj);
void spell_boulder(int, P_char, char*, int, P_char, P_obj);
void spell_shadow_travel(int, P_char, char*, int, P_char, P_obj);
void spell_stunning_visions(int, P_char, char*, int, P_char, P_obj);
void spell_reflection(int, P_char, char*, int, P_char, P_obj);
void spell_mask(int level, P_char ch, char *arg, int type, P_char victim, P_obj tar_obj);
void spell_watching_wall(int, P_char, char*, int, P_char, P_obj);
void spell_nightmare(int, P_char, char*, int, P_char, P_obj);
void spell_shadow_shield(int, P_char, char*, int, P_char, P_obj);
void spell_vanish(int, P_char, char*, int, P_char, P_obj);
void spell_hammer(int, P_char, char*, int, P_char, P_obj);
void spell_detect_illusion(int, P_char, char*, int, P_char, P_obj);
void spell_dream_travel(int, P_char, char*, int, P_char, P_obj);
void spell_clone_form(int, P_char, char*, int, P_char, P_obj);
void spell_imprison(int, P_char, char*, int, P_char, P_obj);
void spell_nonexistence(int, P_char, char*, int, P_char, P_obj);
void spell_dragon(int, P_char, char*, int, P_char, P_obj);
void spell_titan(int, P_char, char*, int, P_char, P_obj);
void spell_delirium(int, P_char, char*, int, P_char, P_obj);
void spell_flicker(int, P_char, char*, int, P_char, P_obj);
void spell_greater_flicker(int, P_char, char*, int, P_char, P_obj);
// illusionist specs - silluionist.c
void spell_obscuring_mist(int, P_char, char*, int, P_char, P_obj);
void spell_suppress_sound(int, P_char, char*, int, P_char, P_obj);
void spell_sound_suppression(int, P_char, char*, int, P_char, P_obj);
void spell_shadow_merge(int, P_char, char*, int, P_char, P_obj);
void cast_ardgral(int, P_char, char *, int, P_char, P_obj);
void spell_shadow_burst(int, P_char, char *, int, P_char, P_obj);
void spell_shadow_spawn(int, P_char, char*, int, P_char, P_obj);
void event_shadow_spawn(P_char, P_char, P_obj, void *);
void spell_asphyxiate(int, P_char, char*, int, P_char, P_obj);
void spell_natures_calling(int, P_char, char *, int, P_char, P_obj);
void spell_natures_call(int, P_char, char*, int, P_char, P_obj);
void event_natures_call(P_char, P_char, P_obj, void *);

/* Siege Engines */
void event_move_engine(P_char ch, P_char victim, P_obj obj, void *data);

/* track.c */

char *sickprocess(const char *);
int MaxTrackDist(P_char);
void track_move(P_char);
void add_track(P_char, int);
void do_track(P_char, char *, int);
void nuke_track(struct trackrecordtype *);
void show_tracks(P_char ch, int room);
void show_tracking_map(P_char);

/* trap.c */

void do_trapremove(P_char ch, char *argument, int cmd);
void do_trapstat(P_char ch, char *argument, int cmd);
void do_traplist(P_char ch, char *argument, int cmd);
void do_trapset(P_char ch, char *argument, int cmd);
bool checkmovetrap(P_char ch, int dir);
bool checkgetput(P_char ch, P_obj obj);
bool checkopen(P_char ch, P_obj obj);
void trapdamage(P_char ch, P_obj obj);

/* properties.c */
void initialize_properties();
void do_properties(P_char, char*, int);
float get_property(const char*, double);
int get_property(const char*, int);
float get_property(const char*, double, bool);
int get_property(const char*, int, bool);

/* nq.c */
void do_quest(P_char, char*, int);
int nq_action_check(P_char ch, P_char mob, char *phrase);
void nq_char_death(P_char ch, P_char victim);

/* utility.c */
ClassSkillInfo SKILL_DATA_ALL(P_char ch, int skill);
void debug( const char *format, ... );
int create_html();
int god_check(char *name);

string strip_ansi(const char *str);
int stripansi_2(const char *, char *);
void ansi_comp(char *);
int ansi_strlen(const char*);
int is_valid_ansi(char *mesg, bool can_set_blinking);
bool is_valid_ansi_with_msg(P_char ch, char *ansi_text, bool can_set_blinking);

char *striplinefeed(char *mesg);
int ScaleAreaDamage(P_char ch, int orig_dam);
int flag2idx(int);
bool can_char_multi_to_class(P_char, int);
int get_real_race(P_char ch);
int GET_CLASS(P_char, uint);
int GET_PRIME_CLASS(P_char, uint);
int GET_SECONDARY_CLASS(P_char, uint);
int GET_CLASS1(P_char, uint);
int GET_ALT_SIZE(P_char);
int GET_CHAR_SKILL_P(P_char, int);
char *get_class_string(P_char, char *);
void broadcast_to_arena(const char *, P_char, P_char, int);
void remove_plushit_bits(P_char mob);
void setCharPhysTypeInfo(P_char);
int is_introd(P_char, P_char);
void add_intro(P_char, P_char);
void purge_old_intros(P_char);
void boot_desc_data(void);
void generate_desc(P_char);
char * generate_shape(P_char);
char * generate_appear(P_char);
char * generate_modif(P_char);
int room_has_valid_exit(const int rnum);
int race_portal_check(P_char, P_char);
void ereglog(int level, const char *format,...);
const int char_in_list(const P_char);
const int is_char_in_room(const P_char, int);
bool racewar(P_char, P_char);
P_char char_in_room(int);
bool spell_can_affect_char(P_char, int);
bool FightingCheck(P_char, P_char, const char *);
bool SanityCheck(P_char, const char *);
bool StatSave(P_char, int, int);
bool are_together (P_char ch1, P_char ch2);
bool has_help (P_char);
bool is_aggr_to(P_char, P_char);
bool aggressive_to(P_char, P_char);
bool aggressive_to_basic(P_char, P_char);
char *PERS(P_char, P_char, int);
char *PERS(P_char, P_char, int, bool);
char *str_dup(const char *);
void str_free(char *);
char strleft(const char *, const char *);
char *deleteChar(char *, const unsigned long);
int send_incoming_msg(P_char ch);
int send_movement_noise(P_char ch);
int BOUNDED(int, int, int);
float BOUNDEDF(float, float, float);
bool is_hot_in_room(int);
int is_prime_plane(int);
/*
int MAX(int, int);
int MIN(int, int);*/
int NumAttackers(P_char);
int STAT_INDEX(int);
int STAT_INDEX2(int);
int STAT_INDEX_DAMAGE_PULSE(float);
int STAT_INDEX_SPELL_PULSE(float);
int SUB_MONEY(P_char, int, int);
int SUB_BALANCE(P_char, int, int);
bool ac_can_see(P_char, P_char, bool);
bool ac_can_see_obj(P_char sub, P_obj obj, int zrange = 0);
int get_vis_mode(P_char, int);
int coin_type(char *);
int dice(int, int);
int exist_in_equipment(P_char, int);
int IS_MORPH(P_char);
int can_exec_cmd(P_char, int);
int is_granted(P_char, int);
int move_cost(P_char, int);
int number(int, int);
int maproom_of_zone(int);
/* int str_cmp(const char *, const char *); */
int str_n_cmp(const char *, const char *);
int strn_cmp(const char *, const char *, uint);
int exitnumb_to_cmd(int);
int cmd_to_exitnumb(int);
struct time_info_data age(P_char);
struct time_info_data mud_time_passed(time_t, time_t);
struct time_info_data real_time_passed(time_t, time_t);
struct time_info_data real_time_countdown(time_t, time_t, int);
void ADD_MONEY(P_char, int);
void CAP(char *);
void DECAP(char *);
void InitGrantFastLookup(void);
void logit(const char *, const char *,...);
void sprint64bit(ulong *, const char **, char *);
void sprintbit(ulong, const char **, char *);
void sprinttype(int, const char **, char *);
void loginlog(int, const char*,...);
void statuslog(int, const char *,...);
void banlog(int, const char *,...);
void epiclog(int, const char *,...);
void strToLower(char *);
void wizlog(int level, const char *,...);
void debug(const char *,...);
void logexp(const char *,...);
int distance_from_shore(int);
int dir_from_keyword(char *);
int weight_notches_above_naked(P_char);
char char_in_snoopby_list(snoop_by_data *, P_char);
void rem_char_from_snoopby_list(snoop_by_data **, P_char);
P_char get_random_char_in_room(int, P_char, int);
void cast_as_area(P_char, int, int, char *);
void hummer(P_obj);
bool grouped(P_char, P_char);
int get_takedown_size(P_char);
bool char_falling(P_char);
P_char find_player_by_pid(int pid);
P_char find_player_by_name(const char *name);
void spawn_random_mapmob(void);
int decimal2binary(unsigned decimal, char* str);
bool is_natural_creature(P_char);
bool is_casting_aggr_spell(P_char);
bool match_pattern(const char *pat, const char *str);
bool is_pid_online( int pid, bool includeLD );
bool has_touch_stone( P_char ch );
P_desc get_descriptor_from_name( char *name );

/* statistcs.c */
void event_write_statistic(P_char ch, P_char victim, P_obj obj, void *data);
//void write_statistic(void);
void do_statistic(P_char, char *, int);


/* weather.c */

void event_astral_clock(P_char ch, P_char victim, P_obj obj, void *data);
//void astral_clock(void);
void init_astral_clock(void);
char get_season(int);
//void another_hour(void);
void event_another_hour(P_char ch, P_char victim, P_obj obj, void *data);
void blow_out_torches(void);
void calc_light_zone(int);
void event_weather_change(P_char ch, P_char victim, P_obj obj, void *data);
//void weather_change(void);
int in_weather_sector(int);

/* specs.*.c, dont want placed in other proto file */

bool is_char_pet (P_char, P_char);
int mount_rent_cost (P_char);

/* range.c */
void event_arrow_bleeding(P_char, P_char, P_obj, void*);
int archery_anatomy_strike(P_char, P_char, P_obj, struct damage_messages *messages, int);
void do_gather(P_char ch, char *argument, int cmd);
void do_throw(P_char, char *, int); /* TASFALEN */
void do_fire(P_char, char *, int); /* TASFALEN */
void do_load_weapon(P_char, char *, int); /* TASFALEN */
int range_scan(P_char, P_char, int, int); /* TASFALEN */
bool mob_can_range_att(P_char, P_char);   /* TASFALEN */
P_obj find_missile(P_char, P_obj, const char *);
P_obj find_throw(P_char, char *, int);
int number_throw(P_char, char *);
int check_wall(int, int);
int check_visible_wall(P_char, int);
P_obj get_wall_dir(P_char, int);
bool can_obj_damage(P_obj, P_char);
int is_slaying(P_obj, P_char, P_char);
void return_home(P_char, P_char, P_obj, void*);
void do_cover (P_char, char *, int);


/* sound.c */
void play_sound(const char *, P_char, int, int);
void sound_to_char(const char *, P_char);
void sound_to_room(const char *, int);
void sound_to_all(const char *);
void sound_to_zone(const char *, int);
void zone_noises(void);

/* genrand.c */
void init_genrand(unsigned long);
double genrand_real1(void);
double genrand_real2(void);
double genrand_real3(void);
unsigned long genrand_int32(void);

void do_specialize(P_char, char*, int);

/* salchemist.c */

void event_enchant(P_char ch, P_char victim, P_obj obj, void *data);
void do_encrust(P_char, char *, int);
void do_spellbind(P_char, char *, int);
void do_mix(P_char, char *, int);
void do_fix(P_char, char *, int);
void do_forge(P_char, char *, int);
P_obj get_bottle(P_char);
int spl2potion(int);
P_obj get_potion(P_char);
bool MobAlchemistGetPotions(P_char, int, int);
bool randomize_potion_non_damage(P_obj, int);
void do_enchant(P_char, char *, int);
P_obj check_furnace(P_char);
void do_smelt(P_char, char *, int);

        
/* mccp.c */
int compress_get_ratio(P_desc player);
int write_to_descriptor(P_desc, const char *);
int parse_telnet_options(P_desc, char *);
void advertise_mccp(P_desc desc);
int compress_start(P_desc, int);
int compress_end(P_desc, int);

/* poisons */
void poison_lifeleak(int, P_char, char *, int, P_char, struct affected_type*);
void poison_weakness(int, P_char, char *, int, P_char, struct affected_type*);
void poison_neurotoxin(int, P_char, char *, int, P_char, struct affected_type*);
void poison_heart_toxin(int, P_char, char *, int, P_char, struct affected_type*);
int poison_common_remove(P_char ch);

void unspecialize(P_char ch, P_obj obj);

/* proc libs */
void do_proclib(P_char ch, char *argument, int cmd);

/* global outpost.h define */
bool check_castle_walls(int, int);

/* gellz.c - Gellz special Procs atm */
void do_deaths_door(P_char ch, char *arg, int cmd);

// smoke.c
void do_smoke(P_char ch, char *arg, int cmd);

#endif /* _SOJ_PROTOTYPES_H_ */

