#ifndef __EPIC_SKILLS_H__
#define __EPIC_SKILLS_H__

struct epic_reward {
  unsigned char type;
  int value;
  int min_points;
  int points_cost;
  int coins;
  unsigned int classes;
};

struct epic_teacher_skill {
  int vnum;
  int skill;
  int min;
  int max;
  int pre_requisite;
  int deny_skill;
  int pre_req_lvl;
};

int epic_teacher(P_char ch, P_char pl, int cmd, char *arg);
int devotion_spell_check(int);
void spell_resistance_check(P_char, P_char, void *);
void do_epic_skills(P_char ch, char *arg, int cmd);
void do_hone(P_char, char*, int); 
bool silent_spell_check(P_char ch);
void say_silent_spell(P_char ch, int spell);
int two_weapon_check(P_char ch);

// Might want to get rid of these? - Lohrr
#define epic_skillpoints(ch) (IS_NPC(ch) ? 0 : ch->only.pc->epic_skill_points)
#define epic_gain_skillpoints(ch, gain) (IS_NPC(ch) ? 0 : ch->only.pc->epic_skill_points \
   = MAX(0, ch->only.pc->epic_skill_points + gain))

#define EPIC_IMP_VNUM         54
#define EPIC_RAVEN_VNUM       55
#define EPIC_OWL_VNUM         56
#define EPIC_CAT_VNUM         57
#define EPIC_BAT_VNUM         58
#define EPIC_IGUANA_VNUM      59

#endif // __EPIC_SKILLS_H__
