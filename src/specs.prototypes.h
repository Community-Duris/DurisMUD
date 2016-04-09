/* constants below are used to recognize wall type -
        they are set in val[3] of wall object */
#define WALL_OF_FLAMES   0
#define WALL_OF_ICE 1
#define LIGHTNING_CURTAIN 2
#define WALL_OF_FOG 3
#define PRISMATIC_WALL 4
#define WEB 5
#define LIFE_WARD 6
#define ILLUSIONARY_WALL 7
#define WALL_OF_FORCE 8
#define WALL_OF_STONE 9
#define WALL_OF_IRON 10
#define WATCHING_WALL 11
#define WALL_OUTPOST 12
#define WALL_OF_BONES 13

int player_council_room(int, P_char, int, char *);
int cage_room1(int, P_char, int, char *);
int cage_room2(int, P_char, int, char *);

int dildo_test(P_obj obj, P_char ch, int cmd, char *arg);
int newbie_portal(P_obj obj, P_char ch, int cmd, char *arg);

int block_up(P_char ch, P_char pl, int cmd, char *arg);
int golem_noflee(P_char ch, P_char pl, int cmd, char *arg);

/* these are all stuff added/modified by raxxel or dakta enjoy */
/* if something crashes due to these, its umm Jera's fault I swear */

/* Necro Spec Pets */
int necro_specpet_flesh(P_char ch, P_char pl, int cmd, char *arg);
int necro_specpet_blood(P_char ch, P_char pl, int cmd, char *arg);
int necro_specpet_bone(P_char ch, P_char pl, int cmd, char *arg);

/* Conj Spec Pets */
int conj_specpet_xorn(P_char ch, P_char pl, int cmd, char *arg);
int conj_specpet_golem(P_char ch, P_char pl, int cmd, char *arg);
int conj_specpet_djinni(P_char ch, P_char pl, int cmd, char *arg);
int conj_specpet_slyph(P_char ch, P_char pl, int cmd, char *arg);
int conj_specpet_undine(P_char ch, P_char pl, int cmd, char *arg);
int conj_specpet_triton(P_char ch, P_char pl, int cmd, char *arg);
int conj_specpet_salamander(P_char ch, P_char pl, int cmd, char *arg);
int conj_specpet_serpent(P_char ch, P_char pl, int cmd, char *arg);


/* misc other */
// int mithril_dagger(P_obj obj, P_char ch, int cmd, char *argument);
int rax_red_dagger(P_obj, P_char, int, char *);
int totem_of_mastery(P_obj obj, P_char ch, int cmd, char *arg);
int god_bp(P_obj obj, P_char ch, int cmd, char *arg);
int out_of_god_bp(P_obj obj, P_char ch, int cmd, char *arg);
int ring_of_regeneration(P_obj obj, P_char ch, int cmd, char *arg);
int proc_whirlwinds(P_obj obj, P_char ch, int cmd, char *arg);
int glowing_necklace(P_obj obj, P_char ch, int cmd, char *arg);
int staff_shadow_summoning(P_obj obj, P_char ch, int cmd, char *arg);
int rod_of_magic(P_obj obj, P_char ch, int cmd, char *arg);
int nightcrawler_dagger(P_obj obj, P_char ch, int cmd, char *arg);
int lyrical_instrument_of_time(P_obj obj, P_char ch, int cmd, char *argument);
int khaziddea_blade(P_obj obj, P_char ch, int cmd, char *arg);
int mentality_mace(P_obj obj, P_char ch, int cmd, char *arg);
int resurrect_totem(P_obj obj, P_char ch, int cmd, char *arg);

/* Ailvio procs */
int burbul_map_obj(P_obj obj, P_char ch, int cmd, char *arg);
int chyron_search_obj(P_obj obj, P_char ch, int cmd, char *arg);
int bandage_mob(P_char, P_char, int, char*);
int bandage_reward_mob(P_char, P_char, int, char*);
/* Avernus procs */
int sinister_tactics_staff(P_obj obj, P_char ch, int cmd, char *arg);
int shard_frozen_styx_water(P_obj obj, P_char ch, int cmd, char *arg);


/* Shabo Procs */
int shaboath_alternation_tower(int room, P_char ch, int cmd, char *argument);
int shaboath_enchantment_tower(int room, P_char ch, int cmd, char *argument);
int shaboath_necromancy_tower(int room, P_char ch, int cmd, char *argument);


int pesky_imp_chest(P_obj obj, P_char ch, int cmd, char *arg);
int mox_totem(P_obj obj, P_char ch, int cmd, char *argument);
int monitor_trident(P_obj obj, P_char ch, int cmd, char *arg);
int flayed_mind_mask(P_obj obj, P_char ch, int cmd, char *argument);
int stalker_cloak(P_obj obj, P_char ch, int cmd, char *argument);
int finslayer_air(P_obj obj, P_char ch, int cmd, char *argument);
int cold_hammer(P_obj obj, P_char ch, int cmd, char *argument);
int aboleth_pendant(P_obj obj, P_char ch, int cmd, char *argument);
int undeadholy_weapon(P_obj obj, P_char ch, int cmd, char *arg);
int shabo_derro_savant(P_char ch, P_char pl, int cmd, char *arg);

int shabo_petre(P_char ch, P_char pl, int cmd, char *arg);
int shabo_evilpetre(P_char ch, P_char pl, int cmd, char *arg);
int shabo_butler(P_char ch, P_char pl, int cmd, char *arg);
int shabo_caran(P_char ch, P_char tch, int cmd, char *arg);
int shabo_palle(P_char ch, P_char vict, int cmd, char *arg);

int tower_summoning(P_obj obj, P_char ch, int cmd, char *arg);
int shabo_trap_north(P_obj obj, P_char ch, int cmd, char *arg);
int shabo_trap_north_two(P_obj obj, P_char ch, int cmd, char *arg);
int shabo_trap_up_two(P_obj obj, P_char ch, int cmd, char *arg);
int shabo_trap_up(P_obj obj, P_char ch, int cmd, char *arg);
int shabo_trap_south(P_obj obj, P_char ch, int cmd, char *arg);
int shabo_trap_south_two(P_obj obj, P_char ch, int cmd, char *arg);
int shabo_trap_down(P_obj obj, P_char ch, int cmd, char *arg);

int spec_mob(P_char ch, P_char pl, int cmd, char *arg);

/* lohrr stuff */
int sphinx_prefect_crown(P_obj obj, P_char ch, int cmd, char *arg);
int ballista(P_obj obj, P_char ch, int cmd, char *arg);
int battering_ram(P_obj obj, P_char ch, int cmd, char *arg);
int catapult(P_obj obj, P_char ch, int cmd, char *arg);
int proc_lohrr(P_obj obj, P_char ch, int cmd, char *arg);
int proc_soldon_hat(P_obj obj, P_char ch, int cmd, char *arg);
int castlewall(P_obj obj, P_char ch, int cmd, char *arg);

/* harpy hometown */
int harpy_gatekeeper(P_char ch, P_char pl, int cmd, char *arg);
int gargoyle_master(P_char ch, P_char pl, int cmd, char *arg);
int harpy_good(P_char ch, P_char pl, int cmd, char *arg);
int harpy_evil(P_char ch, P_char pl, int cmd, char *arg);


/* RaxQuest(tm) Stuff */
int long_john_silver_shout(P_char ch, P_char tch, int cmd, char *arg);
int circlet_of_light(P_obj obj, P_char ch, int cmd, char *arg);
int ljs_sword(P_obj obj, P_char ch, int cmd, char *arg);
int wuss_sword(P_obj obj, P_char ch, int cmd, char *arg);
int undead_parrot(P_char ch, P_char pl, int cmd, char *arg);
int head_guard_sword(P_obj obj, P_char ch, int cmd, char *arg);
int alch_rod(P_obj obj, P_char ch, int cmd, char *arg);
int dragon_skull_helm(P_obj obj, P_char ch, int cmd, char *arg);
int priest_rudder(P_obj obj, P_char ch, int cmd, char *argument);
int ljs_armor(P_obj obj, P_char ch, int cmd, char *arg);
int undead_dragon_east(P_char ch, P_char pl, int cmd, char *arg);
int alch_bag(P_obj obj, P_char ch, int cmd, char *arg);
int pirate_talk(P_char ch, P_char pl, int cmd, char *arg);
int pirate_female_talk(P_char ch, P_char pl, int cmd, char *arg);
int pirate_cabinboy_talk(P_char ch, P_char pl, int cmd, char *arg);
/* end RaxQuest(tm) Stuff */
int annoying_mob(P_char ch, P_char pl, int cmd, char *arg);
int cow_talk(P_char ch, P_char tch, int cmd, char *arg);
int cookie_monster(P_char ch, P_char pl, int cmd, char *arg);
int imprison_armor(P_obj obj, P_char ch, int cmd, char *arg);
int lightning_armor(P_obj obj, P_char ch, int cmd, char *arg);


int magic_mouth(P_obj, P_char, int, char *);
int guildwindow(P_obj, P_char, int, char *);
int guildhome(P_obj, P_char, int, char *);

/* specs.clwcvrn.c */
int clwcvrn_crys_die(P_char, P_char, int, char *);
int clwcvrn_golem_shatter(P_char, P_char, int, char *);
int clwcvrn_protect(P_char, P_char, int, char *);

/* specs.morgs.c */
int morgs_protect(P_char, P_char, int, char *);

/* specs.mist.c */
int mist_protect(P_char, P_char, int, char *);

/* temple zone */
int temple_illyn(P_char, P_char, int, char *);

/* specs.halfcut.c */
int halfcut_defenders(P_char, P_char, int, char *);
int crossbow_ambusher(P_char, P_char, int, char *);
int blowgunner(P_char, P_char, int, char *);

/* specs.snikzone.c */
int frost_elb_dagger(P_obj, P_char, int, char *);
int dagger_submission(P_obj, P_char, int, char *);
int demon_chick(P_char, P_char, int, char *);
int wristthrow_and_gore(P_char, P_char, int, char *);

 /* specs.zalrix.c */
int drowcrusher(P_obj, P_char, int, char *);
int dragonarmor(P_obj, P_char, int, char *);
int squelcher(P_obj, P_char, int, char *);

/* specs.grove.c */
int shimmering_longsword(P_obj, P_char, int, char *);
int troll_slave(P_char ch, P_char pl, int cmd, char *arg);
int stray_dog(P_char ch, P_char pl, int cmd, char *arg);
int hardworking_fisherman(P_char ch, P_char pl, int cmd, char *arg);
int orcish_jailkeeper(P_char ch, P_char pl, int cmd, char *arg);
int orcish_woman(P_char ch, P_char pl, int cmd, char *arg);
int topless_prostitute(P_char ch, P_char pl, int cmd, char *arg);
int sex_crazed_prostitute(P_char ch, P_char pl, int cmd, char *arg);
int well_built_prostitute(P_char ch, P_char pl, int cmd, char *arg);
int sleezy_prostitute(P_char ch, P_char pl, int cmd, char *arg);
int Vella_slut(P_char ch, P_char pl, int cmd, char *arg);
int Padh_bouncer(P_char ch, P_char pl, int cmd, char *arg);
int Vem_rouge(P_char ch, P_char pl, int cmd, char *arg);
int tired_young_man(P_char ch, P_char pl, int cmd, char *arg);

/* specs spec */
int spec_proc(P_char ch, P_char pl, int cmd, char *arg);

/* specs.hoa.c */
int trap_razor_hooks(P_obj, P_char, int, char *);
int trap_tower1_para(P_obj, P_char, int, char *);
int trap_tower2_sleep(P_obj, P_char, int, char *);
int illesarus(P_obj, P_char, int, char *);
int morkoth_mother(P_char, P_char, int, char *);
int akckx(P_char, P_char, int, char *);
int human_girl(P_char, P_char, int, char *);
int hoa_plat(P_obj, P_char, int, char *);
int hoa_death(P_char, P_char, int, char *);
int hoa_sin(P_char, P_char, int, char *);

/* specs.vecna.c */
int vecna_bubble_room(int, P_char, int, char *);
int vecna_deathaltar(P_obj, P_char, int, char *);
int vecna_black_mass(P_char, P_char, int, char *);
int vecnas_fight_proc(P_char, P_char, int, char *);
int chressan_shout(P_char, P_char, int, char *);
int vecna_deathportal(P_obj, P_char, int, char *);
int vecna_ghosthands(P_obj, P_char, int, char *);
int vecna_torturerroom(P_obj, P_char, int, char *);
int vecna_gorge(P_obj, P_char, int, char *);
int vecna_stonemist(P_obj, P_char, int, char *);
int vecna_mob_rebirth(P_char, P_char, int, char *);
int vecna_pestilence(P_obj, P_char, int, char *);
int vecna_minifist(P_obj, P_char, int, char *);
int vecna_dispel(P_obj, P_char, int, char *);
int vecna_boneaxe(P_obj, P_char, int, char *);
int vecna_staffoaken(P_obj, P_char, int, char *);
int vecna_krindor_main(P_obj, P_char, int, char *);
void reset_krindor(P_obj);
int krindor_rogue(P_obj, P_char, int, char *);
int krindor_bard(P_obj, P_char, int, char *);
int krindor_psionicist(P_obj, P_char, int, char *);
int krindor_monk(P_obj, P_char, int, char *);
int krindor_illusionist(P_obj, P_char, int, char *);
int vecna_death_mask(P_obj, P_char, int, char *);
int mob_vecna_procs(P_obj, P_char, int, char *); // Many procs on one item.

/* specs.highway.c */

void event_smoke_to_fire(P_char ch, P_char victim, P_obj obj, void *data);
void event_web_to_smoke(P_char ch, P_char victim, P_obj obj, void *data);
int wand_of_wonder(P_obj, P_char, int, char *);
int hewards_mystical_organ(P_obj, P_char, int, char *);
int amethyst_orb(P_obj, P_char, int, char *);
int lobos_jacket(P_obj, P_char, int, char *);
int soul_render(P_obj, P_char, int, char *);
int breale_townsfolk(P_char, P_char, int, char *);
int kearonor_hide(P_obj, P_char, int, char *);
int mir_spider(P_char, P_char, int, char *);
int mir_fire(P_obj, P_char, int, char *);
int blackness_sword(P_obj, P_char, int, char *);
int soulrender(P_obj, P_char, int, char *);
int white_wyrm_shout(P_char, P_char, int, char *);
int blue_wyrm_shout(P_char, P_char, int, char *);
int red_wyrm_shout(P_char, P_char, int, char *);
int amphisbean(P_char, P_char, int, char *);

/* specs.dragonnia.c */

int baby_dragon(P_char, P_char, int, char *);
int demodragon(P_char, P_char, int, char *);
int dragon_guard(P_char, P_char, int, char *);
int dragons_of_dragonnia(P_char, P_char, int, char *);
int room_of_sanctum(int, P_char, int, char *);
int statue(P_char, P_char, int, char *);
void call_b_acid(P_char, P_char, int);
void call_b_fire(P_char, P_char, int);
void call_b_frost(P_char, P_char, int);
void call_b_gas(P_char, P_char, int);
void call_b_lig(P_char, P_char, int);
void call_protection(P_char, P_char);
void call_protector(P_char, P_char, int, int);
void call_solve_sanctuary(P_char, P_char);
void do_mobdisarm(P_char, char *, int);
void give_proper_abilities(P_char);
void give_proper_stat(P_char);
void make_remains(P_char);

/* specs.mobile.c */

int bahamut(P_char, P_char, int, char *);
int goodie_guardian(P_char, P_char, int, char *);
int claw_cavern_drow_mage(P_char, P_char, int, char *);
int rentacleric(P_char, P_char, int, char *);
int agthrodos(P_char, P_char, int, char *);
int albert(P_char, P_char, int, char *);
int automaton_unblock(P_char, P_char, int, char *);
int barbarian_spiritist(P_char, P_char, int, char *);
int barmaid(P_char, P_char, int, char *);
int beavis(P_char, P_char, int, char *);
int billthecat(P_char, P_char, int, char *);
int blob(P_char, P_char, int, char *);
int boulder_pusher(P_char, P_char, int, char *);
int braddistock(P_char, P_char, int, char *);
int brass_dragon(P_char, P_char, int, char *);
int bridge_troll(P_char, P_char, int, char *);
int bs_boss(P_char, P_char, int, char *);

int bs_citizen(P_char, P_char, int, char *);
int bs_barons_mistress(P_char ch, P_char tch, int cmd, char *arg);
int bs_comwoman(P_char, P_char, int, char *);
int bs_brat(P_char, P_char, int, char *);
int bs_holyman(P_char, P_char, int, char *);
int bs_merchant(P_char, P_char, int, char *);
int bs_wino(P_char, P_char, int, char *);
int bs_watcher(P_char, P_char, int, char *);
int bs_guard(P_char, P_char, int, char *);
int bs_squire(P_char, P_char, int, char *);
int bs_peddler(P_char, P_char, int, char *);
int bs_critter(P_char, P_char, int, char *);
int bs_timid(P_char, P_char, int, char *);
int bs_shady(P_char, P_char, int, char *);
int bs_sinister(P_char, P_char, int, char *);
int bs_menacing(P_char, P_char, int, char *);
int bs_executioner(P_char, P_char, int, char *);
int bs_baron(P_char, P_char, int, char *);
int bs_sparrow(P_char, P_char, int, char *);
int bs_squirrel(P_char, P_char, int, char *);
int bs_crow(P_char, P_char, int, char *);
int bs_mountainman(P_char, P_char, int, char *);
int bs_salesman(P_char, P_char, int, char *);
int bs_nomad(P_char, P_char, int, char *);
int bs_insane(P_char, P_char, int, char *);
int bs_homeless(P_char, P_char, int, char *);
int bs_servant(P_char, P_char, int, char *);
int bs_wolf(P_char, P_char, int, char *);
int bs_gnoll(P_char, P_char, int, char *);
int bs_griffon(P_char, P_char, int, char *);
int bs_boar(P_char, P_char, int, char *);
int bs_cub(P_char, P_char, int, char *);
int bs_fierce(P_char, P_char, int, char *);
int bs_stirge(P_char, P_char, int, char *);
int bs_(P_char, P_char, int, char *);

int butthead(P_char, P_char, int, char *);
int cc_female_ffolk(P_char, P_char, int, char *);
int cc_fisherffolk(P_char, P_char, int, char *);
int cc_warehouse_foreman(P_char, P_char, int, char *);
int cc_warehouse_man(P_char, P_char, int, char *);
int chicken(P_char, P_char, int, char *);
int christine(P_char, P_char, int, char *);
int good_city_guard(P_char, P_char, int, char *);
int cityguard(P_char, P_char, int, char *);
int clyde(P_char, P_char, int, char *);
int confess_figure(P_char, P_char, int, char *);
int cookie(P_char, P_char, int, char *);
int crystal_golem_die(P_char, P_char, int, char *);
int devour(P_char, P_char, int, char *);
int dryad(P_char, P_char, int, char *);
int eligoth_rift_spawn(P_char, P_char, int, char *);
int toe_chamber_switch(P_obj, P_char, int, char *);
int farmer(P_char, P_char, int, char *);
int gate_guard(P_char, P_char, int, char *);
int ghore_paradise(P_char, P_char, int, char *);
int guardian(P_char, P_char, int, char *);
int guild_guard(P_char, P_char, int, char *);
int guru_anapest(P_char, P_char, int, char *);
int hippogriff_die(P_char, P_char, int, char *);
int hunt_cat(P_char, P_char, int, char *);
int ice_artist(P_char, P_char, int, char *);
int ice_bodyguards(P_char, P_char, int, char *);
int ice_cleaning_crew(P_char, P_char, int, char *);
int ice_commander(P_char, P_char, int, char *);
int ice_garden_attendant(P_char, P_char, int, char *);
int ice_impatient_guest(P_char, P_char, int, char *);
int ice_malice(P_char, P_char, int, char *);
int ice_masha(P_char, P_char, int, char *);
int ice_masonary_crew(P_char, P_char, int, char *);
int ice_priest(P_char, P_char, int, char *);
int ice_privates(P_char, P_char, int, char *);
int ice_privates2(P_char, P_char, int, char *);
int ice_raucous_guest(P_char, P_char, int, char *);
int ice_snooty_wife(P_char, P_char, int, char *);
int ice_tar(P_char, P_char, int, char *);
int ice_tubby_merchant(P_char, P_char, int, char *);
int ice_viscount(P_char, P_char, int, char *);
int ice_wolf(P_char, P_char, int, char *);
int warden_shout(P_char, P_char, int, char *);
int imix_shout(P_char, P_char, int, char *);
int menzellon_shout(P_char, P_char, int, char *);
int ogremoch_shout(P_char, P_char, int, char *);
int olhydra_shout(P_char, P_char, int, char *);
int yancbin_shout(P_char, P_char, int, char *);

int demogorgon_shout(P_char, P_char, int, char *);
int strychnesch_shout(P_char, P_char, int, char *);
int morgoor_shout(P_char, P_char, int, char *);
int jabulanth_shout(P_char, P_char, int, char *);
int redpal_shout(P_char, P_char, int, char *);
int cyvrand_shout(P_char, P_char, int, char *);
int overseer_shout(P_char, P_char, int, char *);
int imageproc(P_char, P_char, int, char *);
int earth_treant(P_char, P_char, int, char *);
int yeenoghu(P_char, P_char, int, char *);
int demogorgon(P_char, P_char, int, char *);
int astral_succubus(P_char, P_char, int, char *);


int janitor(P_char, P_char, int, char *);
int jester(P_char, P_char, int, char *);
int neg_pocket(P_char, P_char, int, char *);
int jotun_balor(P_char, P_char, int, char *);
int jotun_mimer(P_char, P_char, int, char *);
int jotun_thrym(P_char, P_char, int, char *);
int jotun_utgard_loki(P_char, P_char, int, char *);
int kobold_priest(P_char, P_char, int, char *);
int loviatar(P_obj, P_char, int, char *);
int mage_anapest(P_char, P_char, int, char *);
int magic_user(P_char, P_char, int, char *);
int mailed_fist_guardian(P_char, P_char, int, char *);
int menden_figurine_die(P_char, P_char, int, char *);
int menden_fisherman(P_char, P_char, int, char *);
int menden_inv_serv_die(P_char, P_char, int, char *);
int menden_magus(P_char, P_char, int, char *);
int money_changer(P_char, P_char, int, char *);
int mystra(P_obj, P_char, int, char *);
int mystra_dragon(P_char, P_char, int, char *);
int navagator(P_char, P_char, int, char *);
int neophyte(P_char, P_char, int, char *);
int nw_agatha(P_char, P_char, int, char *);
int nw_ammaster(P_char, P_char, int, char *);
int nw_ansal(P_char, P_char, int, char *);
int nw_brock(P_char, P_char, int, char *);
int nw_builder(P_char, P_char, int, char *);
int nw_carpen(P_char, P_char, int, char *);
int nw_chicken(P_char, P_char, int, char *);
int nw_chief(P_char, P_char, int, char *);
int nw_cow(P_char, P_char, int, char *);
int nw_cutter(P_char, P_char, int, char *);
int nw_diamaster(P_char, P_char, int, char *);
int nw_elfhealer(P_char, P_char, int, char *);
int nw_emmaster(P_char, P_char, int, char *);
int nw_farmer(P_char, P_char, int, char *);
int nw_foreman(P_char, P_char, int, char *);
int nw_golem(P_char, P_char, int, char *);
int nw_hafbreed(P_char, P_char, int, char *);
int nw_human(P_char, P_char, int, char *);
int nw_logger(P_char, P_char, int, char *);
int nw_malchor(P_char, P_char, int, char *);
int nw_merthol(P_char, P_char, int, char *);
int nw_mirroid(P_char, P_char, int, char *);
int nw_owl(P_char, P_char, int, char *);
int nw_pig(P_char, P_char, int, char *);
int nw_rubmaster(P_char, P_char, int, char *);
int nw_sapmaster(P_char, P_char, int, char *);
int nw_vitnor(P_char, P_char, int, char *);
int nw_woodelf(P_char, P_char, int, char *);
int obsid_cit_death_knight(P_char, P_char, int, char *);
int obsid_cit_satar_ghulan(P_char, P_char, int, char *);
int offensive(P_char, P_char, int, char *);
int phalanx(P_char, P_char, int, char *);
int plant_attacks_blindness(P_char, P_char, int, char *);
int plant_attacks_paralysis(P_char, P_char, int, char *);
int plant_attacks_poison(P_char, P_char, int, char *);
int poison(P_char, P_char, int, char *);
int puff(P_char, P_char, int, char *);
int raoul(P_char, P_char, int, char *);
int realms_master_shout(P_char, P_char, int, char *);
int sales_spec(P_char, P_char, int, char *);
int seas_coral_golem(P_char, P_char, int, char *);
int shadow_demon(P_char, P_char, int, char *);
int shadow_demon_of_torm(P_char, P_char, int, char *);
int shady_man(P_char, P_char, int, char *);
int kimordril_shout(P_char, P_char, int, char*);
int silver_lady_shout(P_char, P_char, int, char*);
int sister_knight(P_char, P_char, int, char *);
int skeleton(P_char, P_char, int, char *);
int animated_skeleton(P_char, P_char, int, char *);
int snowbeast(P_char, P_char, int, char *);
int snowvulture(P_char, P_char, int, char *);
int spiny(P_char, P_char, int, char *);
int spore_ball(P_char, P_char, int, char *);
int stone_crumble(P_char, P_char, int, char *);
int stone_golem(P_char, P_char, int, char *);
P_char summon_creature(int, P_char, int, int, const char *, const char *);
int tako_demon(P_char, P_char, int, char *);
int taxman(P_char, P_char, int, char *);
int tentacle(P_char, P_char, int, char *);
int tentacler_death(P_char, P_char, int, char *);
int charon(P_char, P_char, int, char *);
int thief(P_char, P_char, int, char *);
int tiaka_ghoul(P_char, P_char, int, char *);
int ticket_taker(P_char, P_char, int, char *);
int waiter(P_char, P_char, int, char *);
int warhorse(P_char ch, P_char pl, int cmd, char *arg);
int water_elemental(P_char ch, P_char pl, int cmd, char *arg);
int xexos(P_char, P_char, int, char *);
int archer(P_char, P_char, int, char *);
int citizenship(P_char, P_char, int, char *);
int justice_clerk(P_char, P_char, int, char *);
void nw_block_exit(int, int, int);
void nw_reset_maze(int);
int assoc_golem (P_char, P_char, int, char *);
int house_guard (P_char, P_char, int, char *);
int patrol_leader(P_char, P_char, int, char *);
int patrol_leader_road(P_char, P_char, int, char *);
int transp_tow_acerlade(P_char, P_char, int, char *);
int recharm_ch(P_char, P_char, bool, char *);
int underdark_track(P_char, P_char, int, char *);
int undeadcont_track(P_char, P_char, int, char *);
int fooquest_mob(P_char, P_char, int, char *);
int fooquest_boss(P_char, P_char, int, char *);

int io_assistant(P_char ch, P_char pl, int cmd, char *arg);

int monk_remort(P_char ch, P_char pl, int cmd, char *arg);

// Alatorin
int smelter(P_char ch, P_char pl, int cmd, char *arg);

// Torg
int timoro_die(P_char, P_char, int, char *);

// newbie zone Paladin
int newbie_guard_north (P_char, P_char, int, char *);
int newbie_guard_west (P_char, P_char, int, char *);
int newbie_guard_east (P_char, P_char, int, char *);
int newbie_guard_south (P_char, P_char, int, char *);
//tharn map justice
int outpost_captain(P_char, P_char, int, char *);


int world_quest(P_char, P_char, int, char *);

int newbie_paladin(P_char, P_char, int, char *);
int newbie_quest(P_char, P_char, int, char *);
// CELESTIA PROCS
int Malevolence(P_char, P_char, int, char *);
int Malevolence_vapor(P_char, P_char, int, char *);
int celestia_pulsar(P_char, P_char, int, char *);

 // Nyneth
int construct(P_char, P_char, int, char *);
int nyneth(P_char, P_char, int, char *);
int living_stone(P_char, P_char, int, char *);

int greater_living_stone(P_char, P_char, int, char *);
int shadow_monster(P_char, P_char, int, char *);
int insects(P_char, P_char, int, char *);
int illus_dragon(P_char, P_char, int, char *);
int illus_titan(P_char, P_char, int, char *);
int necro_dracolich(P_char, P_char, int, char *);
int elemental_swarm_fire(P_char, P_char, int, char *);
int elemental_swarm_earth(P_char, P_char, int, char *);
int elemental_swarm_air(P_char, P_char, int, char *);
int elemental_swarm_water(P_char, P_char, int, char *);


/* specs.ioun.c */
int ioun_sustenance(P_obj, P_char, int, char *);
int ioun_testicle(P_obj, P_char, int, char *);
int ioun_warp(P_obj, P_char, int, char *);

/* specs.lohrr.c */
int leviathan(P_char, P_char, int, char *);


/* specs.object.c */


/* START 56 ZONE */
int transparent_blade(P_obj, P_char, int, char *);
int serpent_of_miracles(P_obj, P_char, int, char *);
int Einjar(P_obj, P_char, int, char *);
// END 56 ZONE

int artifact_monolith(P_obj, P_char, int, char *);
int disarm_pick_gloves (P_obj, P_char, int, char *);
int master_set(P_obj, P_char, int, char *);
// used inr andomobj.c
int random_eq_proc(P_obj, P_char, int, char *);
int set_proc(P_obj, P_char, int, char *);
int encrusted_eq_proc(P_obj, P_char, int, char *);
int thrusted_eq_proc(P_obj, P_char, int, char *);

int thrusted_eq_proc(P_obj, P_char, int, char *);
int parchment_forge(P_obj, P_char, int, char *);
int learn_recipe(P_obj, P_char, int, char *);

// 51 potion chest
int treasure_chest(P_obj, P_char, int, char *);

int relic_proc(P_obj, P_char, int, char *);
/* 4 horseman */
int brainripper(P_obj, P_char, int, char *);

/* Nyneth */
int hammer_titans(P_obj, P_char, int, char *);
int stormbringer(P_obj, P_char, int, char *);

//Thanks giving eq.
int generic_riposte_proc(P_obj, P_char, int, char *);
int generic_parry_proc(P_obj, P_char, int, char *);
int tripboots(P_obj, P_char, int, char *);
int blindbadge(P_obj, P_char, int, char *);
int fumblegaunts(P_obj, P_char, int, char *);
int confusionsword(P_obj, P_char, int, char *);
int guild_badge(P_obj, P_char, int, char *);
int gc_portal(P_obj, P_char, int, char *);
int ec_portal(P_obj, P_char, int, char *);
int druid_spring(P_obj, P_char, int, char *);
int blighter_pond(P_obj, P_char, int, char *);
int druid_sabre(P_obj, P_char, int, char *);
int death_proc(P_obj, P_char, int, char *);
int olympus_portal(P_obj, P_char, int, char *);
int stat_pool_str(P_obj, P_char, int, char *);
int stat_pool_dex(P_obj, P_char, int, char *);
int stat_pool_agi(P_obj, P_char, int, char *);
int stat_pool_con(P_obj, P_char, int, char *);
int stat_pool_wis(P_obj, P_char, int, char *);
int stat_pool_int(P_obj, P_char, int, char *);
int stat_pool_pow(P_obj, P_char, int, char *);
int stat_pool_cha(P_obj, P_char, int, char *);
int stat_pool_luc(P_obj, P_char, int, char *);
int skill_beacon(P_obj, P_char, int, char *);
int epic_stone(P_obj, P_char, int, char *);
int period_book(P_obj, P_char, int, char *);
int spell_pool(P_obj, P_char, int, char *);
int mace_of_sea(P_obj, P_char, int, char *);
int platemail_of_defense(P_obj, P_char, int, char *);
int serpent_blade(P_obj, P_char, int, char *);
int lich_spine(P_obj, P_char, int, char *);
int revenant_helm(P_obj, P_char, int, char *);
int dragonlord_plate(P_obj, P_char, int, char *);
int sunblade(P_obj, P_char, int, char *);
int kvasir_dagger(P_obj, P_char, int, char *);
int dragonslayer(P_obj, P_char, int, char *);
int mankiller(P_obj, P_char, int, char *);
int fun_dagger(P_obj, P_char, int, char *);
int cutting_dagger(P_obj, P_char, int, char *);
int mist_claymore(P_obj, P_char, int, char *);
int bloodfeast(P_obj, P_char, int, char *);
int blue_sword_armor(P_obj, P_char, int, char *);
int jet_black_maul(P_obj, P_char, int, char *);

int xmas_cap(P_obj, P_char, int, char *);
int bel_sword(P_obj obj, P_char ch, int cmd, char *arg);
int zarthos_vampire_slayer(P_obj obj, P_char ch, int cmd, char *arg);
int critical_attack_proc(P_obj obj, P_char ch, int cmd, char *arg);
int orb_of_destruction(P_obj, P_char, int, char *);
int sanguine(P_obj, P_char, int, char *);
int neg_orb(P_obj, P_char, int, char *);
int artifact_biofeedback(P_obj, P_char, int, char *);
int artifact_stone(P_obj, P_char, int, char *);
int thanksgiving_wings(P_obj, P_char, int, char *);
int moonstone(P_obj, P_char, int, char *);
int artifact_shadow_shield(P_obj, P_char, int, char *);
int charon_ship(P_obj, P_char, int, char *);
int living_necroplasm(P_obj, P_char, int, char *);
int vapor(P_obj, P_char, int, char *);
int vigor_mask(P_obj, P_char, int, char *);
int church_door(P_obj, P_char, int, char *);
int splinter(P_obj, P_char, int, char *);
int demo_scimitar(P_obj, P_char, int, char *);
int dranum_mask(P_obj, P_char, int, char *);
int artifact_hide(P_obj, P_char, int, char *);
int pathfinder(P_obj, P_char, int, char *);
int miners_helmet(P_obj, P_char, int, char *);
int guild_chest(P_obj, P_char, int, char *);
int illithid_sack(P_obj, P_char, int, char *);
int artifact_invisible(P_obj, P_char, int, char *);
int burn_touch_obj(P_obj, P_char, int, char *);
int transp_tow_misty_gloves(P_obj, P_char, int, char *);
int zarbon_shaper(P_obj, P_char, int, char *);
int rod_of_zarbon(P_obj, P_char, int, char *);
int illithid_teleport_veil(P_obj, P_char, int, char *);
int check_trap_trigger(P_char, int);
int trap_timer(P_obj, P_char, int, char *);
int jailtally(P_obj obj, P_char, int, char *);
int flaming_mace_ruzdo(P_obj, P_char, int, char *);
int automaton_lever(P_obj, P_char, int, char *);
int banana(P_obj, P_char, int, char *);
int chess_board(P_obj, P_char, int, char *);
int creeping_doom(P_obj, P_char, int, char *);
int crystal_spike(P_obj, P_char, int, char *);
int cursed_mirror(P_obj, P_char, int, char *);
int die_roller(P_obj, P_char, int, char *);
int floating_pool(P_obj, P_char, int, char *);
int teleporting_pool(P_obj, P_char, int, char *);
int teleporting_map_pool(P_obj, P_char, int, char *);
int fw_ruby_monocle(P_obj, P_char, int, char *);
int flying_citadel(P_obj, P_char, int, char *);
int holy_weapon(P_obj, P_char, int, char *);
int item_switch(P_obj, P_char, int, char *);
int labelas(P_obj, P_char, int, char *);
int lathander(P_obj, P_char, int, char *);
int llyms_altar(P_obj, P_char, int, char *);
int menden_figurine(P_obj, P_char, int, char *);
int obj_imprison(P_obj obj, P_char ch, int cmd, char *arg);
int orcus_wand(P_obj obj, P_char ch, int cmd, char *arg);
int portal_door(P_obj, P_char, int, char *);
int portal_wormhole(P_obj, P_char, int, char *);
int portal_etherportal(P_obj obj, P_char ch, int cmd, char *arg);
int ring_elemental_control(P_obj, P_char, int, char *);
int skeleton_key(P_obj, P_char, int, char *);
int slot_machine(P_obj, P_char, int, char *);
int staff_of_oghma(P_obj, P_char, int, char *);
int thought_beacon(P_obj, P_char, int, char *);
int torms_chessboard(P_obj, P_char, int, char *);
int trustee_artifact(P_obj, P_char, int, char *);
int tyr_sword(P_obj, P_char, int, char *);
int wall_generic(P_obj, P_char, int, char *);
int wall_generic1(P_obj, P_char, int, char *);
int huntsman_ward(P_obj, P_char, int, char *);
int ice_block(P_obj, P_char, int, char *);
int frost_beacon(P_obj, P_char, int, char *);
int io_proc_item(P_obj, P_char, int, char *);
int changelog(P_obj, P_char, int, char *);
int sword_named_magik(P_obj, P_char, int, char *);
int yuan_ti_stone(P_obj, P_char, int, char *);
int trans_tower_sword(P_obj, P_char, int, char *);
int trans_tower_shadow_globe(P_obj, P_char, int, char *);
int golem_chunk(P_obj, P_char, int, char *);
int good_evil_sword(P_obj, P_char, int, char *);
int earthquake_gauntlet(P_obj, P_char, int, char *);
int blind_boots(P_obj, P_char, int, char *);
int lifereaver(P_obj, P_char, int, char *);
int flaming_axe_of_azer(P_obj, P_char, int, char *);
int mrinlor_whip(P_obj, P_char, int, char *);
int ogre_warlords_sword(P_obj, P_char, int, char *);
int khildarak_warhammer(P_obj, P_char, int, char *);
int mace_dragondeath(P_obj, P_char, int, char *);
int lucky_weapon(P_obj, P_char, int, char *);
int glades_dagger(P_obj, P_char, int, char *);
int doom_blade_Proc(P_obj, P_char, int, char *);
int righteous_blade(P_obj, P_char, int, char *);
int flame_blade(P_obj, P_char, int, char *);

int refreshing_fountain(P_obj, P_char, int, char *);
int magical_fountain(P_obj, P_char, int, char *);

int random_tomb(P_obj, P_char, int, char *);
int random_slab(P_obj, P_char, int, char *);
int random_glass(P_obj, P_char, int, char *);


/* TIKI MADMAN PROCS */

int madman_mangler(P_obj, P_char, int, char *);
int madman_shield(P_obj, P_char, int, char *);

/* specs.realm.c */

int cricket(P_char, P_char, int, char *);
int faerie(P_char, P_char, int, char *);
int finn(P_char, P_char, int, char *);
int tree_spirit(P_char, P_char, int, char *);
int vaprak_claw(P_obj, P_char, int, char *);

/* specs.room.c */

int berserker_proc_room(int room, P_char, int cmd, char *arg);
int multiclass_proc(int room, P_char, int cmd, char *arg);
int inn (int, P_char, int, char *);
int undead_inn (int, P_char, int, char *);
int GlyphOfWarding(int, P_char, int, char *);
int GithyankiCave(int, P_char, int, char *);
int TiamatThrone(int, P_char, int, char *);
int akh_elamshin(int, P_char, int, char *);
int automaton_trapdoor(int, P_char, int, char *);
int dump(int, P_char, int, char *);
int feed_lock(int room, P_char, int, char *);
int fw_warning_room(int, P_char, int, char *);
int keyless_unlock(int, P_char, int, char *);
int kings_hall(int room, P_char, int, char *);
int pet_shops(int, P_char, int, char *);

int patrol_shops(int, P_char, int, char *);

int pray_for_items(int, P_char, int, char *);
int duergar_guild(int, P_char, int, char *);

int squid_arena(int, P_char, int, char *);
int mortal_heaven(int, P_char, int, char *);
/* specs.tharnadia.c */

int tharn_tall_merchant(P_char, P_char, int, char *);
int tharn_beach_guard(P_char, P_char, int, char *);
int tharn_male_commoner(P_char, P_char, int, char *);
int tharn_female_commoner(P_char, P_char, int, char *);
int tharn_human_merchant(P_char, P_char, int, char *);
int tharn_lighthouse_attendent(P_char, P_char, int, char *);
int tharn_crier_one(P_char, P_char, int, char *);
int tharn_shady_mercenary(P_char, P_char, int, char *);
int tharn_shady_youth(P_char, P_char, int, char *);
int tharn_jailor(P_char, P_char, int, char *);
int tharn_old_man(P_char, P_char, int, char *);

/* specs.twintowers.c */

int forest_animals(P_char ch, P_char pl, int cmd, char *arg);
int forest_corpse(P_obj obj, P_char ch, int cmd, char *args);
int gardener_block(int room, P_char ch, int cmd, char *args);

/* specs.undermountain.c */

int blade_of_paladins(P_obj, P_char, int, char *);
int iron_flindbar(P_obj, P_char, int, char *);
int fade_drusus(P_obj, P_char, int, char *);
int magebane_falchion(P_obj, P_char, int, char *);
int lightning_sword(P_obj, P_char, int, char *);
int woundhealer_scimitar(P_obj, P_char, int, char *);
int flame_of_north_sword(P_obj, P_char, int, char *);
int generic_drow_eq(P_obj, P_char, int, char *);
int elfdawn_sword(P_obj, P_char, int, char *);
int undead_trident(P_obj, P_char, int, char *);
int martelo_mstar(P_obj, P_char, int, char *);
int um_durnan(P_char, P_char, int, char *);
int um_mhaere(P_char, P_char, int, char *);
int um_regular(P_char, P_char, int, char *);
int um_gambler(P_char, P_char, int, char *);
int um_tamsil(P_char, P_char, int, char *);
int um_kevlar(P_char, P_char, int, char *);
int um_thorn(P_char, P_char, int, char *);
int um_korelar(P_char, P_char, int, char *);
int um_essra(P_char, P_char, int, char *);
int um_mezzoloth(P_char, P_char, int, char *);
int um_goblin_leader(P_char, P_char, int, char *);
int flying_dagger(P_char, P_char, int, char *);
int ochre_jelly(P_char, P_char, int, char *);
int animated_sword(P_char, P_char, int, char *);
int helmed_horror(P_char, P_char, int, char *);
int malodine_one(P_char, P_char, int, char *);
int malodine_two(P_char, P_char, int, char *);
int malodine_three(P_char, P_char, int, char *);
int black_pudding(P_char, P_char, int, char *);
int flame_of_north(P_obj, P_char, int, char *);

/* specs.underworld.c */

int avernus(P_obj, P_char, int, char *);
int bulette(P_char, P_char, int, char *);
int doombringer(P_obj, P_char, int, char *);
int orb_of_the_sea(P_obj, P_char, int, char *);
int mace(P_obj, P_char, int, char *);
int dranum_jurtrem(P_char, P_char, int, char *);
int nexus(P_obj, P_char, int, char *);
int elfgate(P_obj, P_char, int, char *);
int flamberge(P_obj, P_char, int, char *);
int githpc_special_weap(P_obj, P_char, int, char *);
int githyanki(P_obj, P_char, int, char *);
int hammer(P_obj, P_char, int, char *);
int barb(P_obj, P_char, int, char *);
int lightning(P_obj, P_char, int, char *);
int magic_pool(P_obj, P_char, int, char *);
int magic_map_pool(P_obj, P_char, int, char *);
int random_map_room();
int nightbringer(P_obj, P_char, int, char *);
int dispator(P_obj, P_char, int, char *);
int dragonkind(P_obj, P_char, int, char *);
int piercer(P_char, P_char, int, char *);
int purple_worm(P_char, P_char, int, char *);
int tiamat(P_char, P_char, int, char *);
int torment(P_obj, P_char, int, char *);
int unholy_avenger_bloodlust(P_obj, P_char, int, char *);
int tiamat_stinger(P_obj, P_char, int, char *);
int gfstone(P_obj, P_char, int, char *);
int tendrils(P_obj, P_char, int, char *);
int elvenkind_cloak(P_obj, P_char, int, char*);
int deflect_ioun(P_obj, P_char, int, char*);
int epic_teacher(P_char, P_char, int, char *);
int learn_tradeskill(P_char, P_char, int, char *);
int epic_store(P_char, P_char, int, char *);
int pvp_store(P_char, P_char, int, char *);
int epic_familiar(P_char, P_char, int, char *);
int smith(P_char, P_char, int, char *);

/* specs.jot.c */

int ai_mob_proc(P_char, P_char, int, char *);
int random_mob_proc(P_char, P_char, int, char *);
int random_quest_mob_proc(P_char, P_char, int, char *);

/* specs.jot.c */

int frostbite(P_obj, P_char, int, char *);
int faith(P_obj, P_char, int, char *);
int mistweave(P_obj, P_char, int, char *);
int leather_vest(P_obj obj, P_char ch, int cmd, char *arg);
int deva_cloak(P_obj obj, P_char ch, int cmd, char *arg);
int icicle_cloak(P_obj obj, P_char ch, int cmd, char *arg);
int valkyrie(P_obj, P_char, int, char *);
int ogrebane(P_obj, P_char, int, char *);
int giantbane(P_obj, P_char, int, char *);
int dwarfslayer(P_obj, P_char, int, char *);
int betrayal(P_obj, P_char, int, char *);
int mindbreaker(P_obj, P_char, int, char *);
int holy_mace(P_obj, P_char, int, char *);
int staff_of_blue_flames(P_obj, P_char, int, char *);
int staff_of_power(P_obj, P_char, int, char *);
int reliance_pegasus(P_obj, P_char, int, char *);

/* specs.verzanan.c */

#ifdef CONFIG_JAIL
int verzanan_witness(P_char, P_char, int, char *);
#endif
int artillery_one(P_char, P_char, int, char *);
int assassin_one(P_char, P_char, int, char *);
int baker_one(P_char, P_char, int, char *);
int baker_two(P_char, P_char, int, char *);
int bouncer_four(P_char, P_char, int, char *);
int bouncer_one(P_char, P_char, int, char *);
int bouncer_three(P_char, P_char, int, char *);
int bouncer_two(P_char, P_char, int, char *);
int brigand_one(P_char, P_char, int, char *);
int casino_four(P_char, P_char, int, char *);
int casino_one(P_char, P_char, int, char *);
int casino_three(P_char, P_char, int, char *);
int casino_two(P_char, P_char, int, char *);
int cat_one(P_char, P_char, int, char *);
int cell_drunk(P_char, P_char, int, char *);
int cleric_one(P_char, P_char, int, char *);
int clock_tower(P_obj, P_char, int, char *);
int commoner_five(P_char, P_char, int, char *);
int commoner_four(P_char, P_char, int, char *);
int commoner_one(P_char, P_char, int, char *);
int commoner_six(P_char, P_char, int, char *);
int commoner_three(P_char, P_char, int, char *);
int commoner_two(P_char, P_char, int, char *);
int crier_one(P_char, P_char, int, char *);
int dog_one(P_char, P_char, int, char *);
int dog_two(P_char, P_char, int, char *);
int drunk_one(P_char, P_char, int, char *);
int drunk_three(P_char, P_char, int, char *);
int drunk_two(P_char, P_char, int, char *);
int farmer_one(P_char, P_char, int, char *);
int fisherman_one(P_char, P_char, int, char *);
int fisherman_two(P_char, P_char, int, char *);
int gesen(P_obj obj, P_char, int, char *);
int guard_one(P_char, P_char, int, char *);
int guard_two(P_char, P_char, int, char *);
int guild_guard_eight(P_char, P_char, int, char *);
int guild_guard_eleven(P_char, P_char, int, char *);
int guild_guard_five(P_char, P_char, int, char *);
int guild_guard_four(P_char, P_char, int, char *);
int guild_guard_nine(P_char, P_char, int, char *);
int guild_guard_one(P_char, P_char, int, char *);
int guild_guard_seven(P_char, P_char, int, char *);
int guild_guard_six(P_char, P_char, int, char *);
int guild_guard_ten(P_char, P_char, int, char *);
int guild_guard_thirteen(P_char, P_char, int, char *);
int guild_guard_three(P_char, P_char, int, char *);
int guild_guard_twelve(P_char, P_char, int, char *);
int guild_guard_two(P_char, P_char, int, char *);
int guild_protection(P_char, P_char);
int guildmaster_eight(P_char, P_char, int, char *);
int guildmaster_eleven(P_char, P_char, int, char *);
int guildmaster_five(P_char, P_char, int, char *);
int guildmaster_four(P_char, P_char, int, char *);
int guildmaster_nine(P_char, P_char, int, char *);
int guildmaster_one(P_char, P_char, int, char *);
int guildmaster_seven(P_char, P_char, int, char *);
int guildmaster_six(P_char, P_char, int, char *);
int guildmaster_ten(P_char, P_char, int, char *);
int guildmaster_three(P_char, P_char, int, char *);
int guildmaster_twelve(P_char, P_char, int, char *);
int guildmaster_two(P_char, P_char, int, char *);
int homeless_one(P_char, P_char, int, char *);
int homeless_two(P_char, P_char, int, char *);
int lighthouse_one(P_char, P_char, int, char *);
int lighthouse_two(P_char, P_char, int, char *);
int lloth(P_obj, P_char, int, char *);
int lloth_avatar(P_obj, P_char, int, char *);
int mage_one(P_char, P_char, int, char *);
int mercenary_one(P_char, P_char, int, char *);
int mercenary_three(P_char, P_char, int, char *);
int mercenary_two(P_char, P_char, int, char *);
int merchant_one(P_char, P_char, int, char *);
int merchant_two(P_char, P_char, int, char *);
int naval_four(P_char, P_char, int, char *);
int naval_one(P_char, P_char, int, char *);
int naval_three(P_char, P_char, int, char *);
int naval_two(P_char, P_char, int, char *);
int park_five(P_char, P_char, int, char *);
int park_four(P_char, P_char, int, char *);
int park_one(P_char, P_char, int, char *);
int park_six(P_char, P_char, int, char *);
int park_three(P_char, P_char, int, char *);
int park_two(P_char, P_char, int, char *);
int piergeiron(P_char, P_char, int, char *);
int piergeiron_guard(P_char, P_char, int, char *);
int prostitute_one(P_char, P_char, int, char *);
int rambo(P_obj obj, P_char, int, char *);
int rogue_one(P_char, P_char, int, char *);
int sailor_one(P_char, P_char, int, char *);
int seabird_one(P_char, P_char, int, char *);
int seabird_two(P_char, P_char, int, char *);
int seaman_one(P_char, P_char, int, char *);
int secret_door(P_obj, P_char, int, char *);
int selune_five(P_char, P_char, int, char *);
int selune_four(P_char, P_char, int, char *);
int selune_one(P_char, P_char, int, char *);
int selune_six(P_char, P_char, int, char *);
int selune_three(P_char, P_char, int, char *);
int selune_two(P_char, P_char, int, char *);
int shopper_one(P_char, P_char, int, char *);
int shopper_two(P_char, P_char, int, char *);
int tailor_one(P_char, P_char, int, char *);
int varon(P_obj obj, P_char, int, char *);
int wanderer(P_char, P_char, int, char *);
int warrior_one(P_char, P_char, int, char *);
int verzanan_guard_one(P_char, P_char, int, char *);
int verzanan_guard_three(P_char, P_char, int, char *);
int verzanan_guard_two(P_char, P_char, int, char *);
int verzanan_guild_eight(int, P_char, int, char *);
int verzanan_guild_eleven(int, P_char, int, char *);
int verzanan_guild_five(int, P_char, int, char *);
int verzanan_guild_four(int, P_char, int, char *);
int verzanan_guild_nine(int, P_char, int, char *);
int verzanan_guild_one(int, P_char, int, char *);
int verzanan_guild_seven(int, P_char, int, char *);
int verzanan_guild_six(int, P_char, int, char *);
int verzanan_guild_ten(int, P_char, int, char *);
int verzanan_guild_three(int, P_char, int, char *);
int verzanan_guild_twelve(int, P_char, int, char *);
int verzanan_guild_two(int, P_char, int, char *);
int verzanan_portal(P_obj, P_char, int, char *);
int wrestler_one(P_char, P_char, int, char *);
int young_druid_one(P_char, P_char, int, char *);
int young_mercenary_one(P_char, P_char, int, char *);
int young_monk_one(P_char, P_char, int, char *);
int young_necro_one(P_char, P_char, int, char *);
int young_paladin_one(P_char, P_char, int, char *);
int youth_one(P_char, P_char, int, char *);
int youth_two(P_char, P_char, int, char *);
void verzanan_city_noises(void);

// ships
int ship_panel_proc(P_obj, P_char, int, char *);
int ship_obj_proc(P_obj, P_char, int, char *);
int crew_shop_proc(int,P_char,int,char *);
int erzul_proc(P_char, P_char, int, char *);
int moonstone_fragment(P_obj obj, P_char ch, int cmd, char *argument);


// sea kingdom stuff
int SeaKingdom_Tsunami(P_obj, P_char, int, char *);

// jind ticketmaster stuff
int jindo_ticket_master(P_char, P_char, int, char *);


int witch_doctor(P_char, P_char, int, char *);
int llyren(P_char, P_char, int, char *);

// sevenoaks stuff
int sevenoaks_longsword(P_obj, P_char, int, char*);

//airp2
int dagger_of_wind(P_obj, P_char, int, char*);
int staff_of_air_conjuration(P_obj, P_char, int, char *);

//newbie zone stuf
int life_of_stream(P_obj obj, P_char ch, int cmd, char *arg);
int newbie_sign1(P_obj obj, P_char ch, int cmd, char *arg);
int newbie_sign2(P_obj obj, P_char ch, int cmd, char *arg);

//Duris Tournament
int arenaobj_proc(P_obj, P_char, int, char *);


// Ako Stuff

int ako_hypersquirrel(P_char ch, P_char pl, int cmd, char *arg);
int ako_songbird(P_char ch, P_char pl, int cmd, char *arg);
int ako_vulture(P_char ch, P_char pl, int cmd, char *arg);
int ako_wildmare(P_char ch, P_char pl, int cmd, char *arg);
int ako_cow(P_char ch, P_char pl, int cmd, char *arg);


// Mossi Modification:   DECAY
int blood_stains(P_obj ch, P_char pl, int cmd, char *arg);
int tracks(P_obj ch, P_char pl, int cmd, char *arg);
int ice_shattered_bits(P_obj ch, P_char pl, int cmd, char *arg);


// storag lockers
int storage_locker_room_hook(int room, P_char ch, int cmd, char *arg);
int storage_locker_obj_hook(P_obj obj, P_char ch, int cmd, char *argument);

// god toys
int vareena_statue(P_obj obj, P_char ch, int cmd, char *argument);
int unspec_altar(P_obj obj, P_char ch, int cmd, char *argument);

int generic_obj_proc(P_obj obj, P_char ch, int cmd, char *argument);

// rename by mob
int mob_do_rename_hook(P_char npc, P_char ch, int cmd, char *arg);

int clear_epic_task_spec(P_char npc, P_char ch, int cmd, char *arg);

// generic undead death howl--Cool!
int undead_howl(P_char, P_char, int, char *);

/* specs.winterhaven.c */
int wh_janitor(P_char, P_char, int, char *);
int wh_guard(P_char, P_char, int, char *);

// specs.lucrot.c
int lucrot_mindstone(P_obj obj, P_char ch, int cmd, char *arg);

// specs.venthix.c
int roulette_pistol(P_obj obj, P_char ch, int cmd, char *arg);
int orb_of_deception(P_obj obj, P_char ch, int cmd, char *arg);
void event_super_cannon(P_char ch, P_char vict, P_obj obj, void *data);
int super_cannon(P_obj obj, P_char ch, int cmd, char *arg);
void halloween_mine_proc(P_char ch);
int zombies_game(P_obj obj, P_char ch, int cmd, char *arg);
int zg_count_zombies(P_obj obj);
