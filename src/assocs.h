#ifndef _ASSOCS_H_
#define _ASSOCS_H_

/************************************************************************
* assocs.h - defines used for assocs.c                                  *
*                                                                       *
* a collection of useful defines for accessing association data         *
* done 1996 by Krov (Ingo Bojak) for Duris DikuMUD                      *
************************************************************************/

#include "structs.h"

/* DEFINES for assocs.c */

/* defines allowing access to player association data */ 
#define GET_A_NUM(ch) (ch)->specials.guild
#define GET_A_BITS(ch) (ch)->specials.guild_status

/* maximum/minimum numbers of associations */
#define MAX_ASC       999       /* maximal number of associations */
#define MAX_ENEMIES   10        /* maximal number of non-member enemies */
#define MIN_LEVEL     25        /* minimum level to join */
#define MAX_DISPLAY   200       /* max number of shown people on soc list */

/* note that the following is including ansi color chars, displayed
   strings are much shorter due to color */
#define MAX_STR_RANK     78        /* maximum length of rank name */
#define MAX_STR_ASC      78        /* maximum length of association name */
#define MAX_STR_TITLE    156       /* maximum length title=rank+asc name */

/* saves definitions */
#define ASC_DIR "Players/Assocs/"
#define ASC_SAVE_SIZE  16384

/* let's note waste memory if not necessary... 300 chars should be plenty */
#define MAX_STR_NORMAL  300

/* definition of association bits, as compact as possible to leave
   room for later extensions */
/* player bits */
#define A_SF1   1u         /* SF2,SF1        00:no thanks. 01:applicant   */
#define A_SF2   2u         /* society flags  10:member. 11:banned.        */
#define A_RK1   4u         /* RK3,RK2,RK1  000:enemy. 001:on parole.      */
#define A_RK2   8u         /* rank   010:normal. 011:senior. 100:officer. */
#define A_RK3   16u        /* bits   101:deputy. 110:leader. 111:god.     */
#define A_DEBT  32u        /* 0=no debts, 1=has debts                     */
#define A_STATICTITLE  64u /* change title on logon?                      */
/* association bits */
#define A_HIDESUBTITLE 16384u /* hide sub titles of guild            */
#define A_CHALL 32768u     /* does the association allow challenges? */
#define A_HIDETITLE 65536u /* do members get their titles reset?     */

#define A_SF_MASK   A_SF1+A_SF2
#define A_RK_MASK   A_RK3+A_RK2+A_RK1

/* this is used to protect player bits!
   UPDATE if more bits are used for player */
#define A_P_MASK (A_SF_MASK+A_RK_MASK+A_DEBT+A_STATICTITLE)
#define ASC_MASK ~A_P_MASK

/* get, test and set masked bits */
#define GET_M_BITS(asn,mask) ((asn) & (mask))
#define LT_M_BITS(asn,mask,bits) (((asn) & (mask)) < (bits))
#define LE_M_BITS(asn,mask,bits) (((asn) & (mask)) <= (bits))
#define IS_M_BITS(asn,mask,bits) (((asn) & (mask)) == (bits))
#define GT_M_BITS(asn,mask,bits) (((asn) & (mask)) > (bits))
#define GE_M_BITS(asn,mask,bits) (((asn) & (mask)) >= (bits))
#define SET_M_BITS(asn,mask,bits) ((asn) = ((asn) & ~(mask)) + (bits))

/* protect higher routines from knowing society flags */
#define IS_NO_THANKS(asn) (IS_M_BITS((asn),A_SF_MASK,0))
#define SET_NO_THANKS(asn) (SET_M_BITS((asn),A_SF_MASK,0))
#define IS_APPLICANT(asn) (IS_M_BITS((asn),A_SF_MASK,A_SF1))
#define SET_APPLICANT(asn) (SET_M_BITS((asn),A_SF_MASK,A_SF1))
#define IS_MEMBER(asn) (IS_M_BITS((asn),A_SF_MASK,A_SF2))
#define SET_MEMBER(asn) (SET_M_BITS((asn),A_SF_MASK,A_SF2))
#define IS_BANNED(asn) (IS_M_BITS((asn),A_SF_MASK,A_SF_MASK))
#define SET_BANNED(asn) (SET_M_BITS((asn),A_SF_MASK,A_SF_MASK))
#define IS_ASSOC_MEMBER(ch, asn) ( GET_A_NUM(ch) == asn && IS_MEMBER(GET_A_BITS(ch)) && GT_PAROLE(GET_A_BITS(ch)) )

/* protect higher routines from knowing rank bits */
#define IS_ENEMY(asn) (IS_M_BITS((asn),A_RK_MASK,0))
#define GT_ENEMY(asn) (GT_M_BITS((asn),A_RK_MASK,0))
#define SET_ENEMY(asn) (SET_M_BITS((asn),A_RK_MASK,0))
#define IS_PAROLE(asn) (IS_M_BITS((asn),A_RK_MASK,A_RK1))
#define GT_PAROLE(asn) (GT_M_BITS((asn),A_RK_MASK,A_RK1))
#define SET_PAROLE(asn) (SET_M_BITS((asn),A_RK_MASK,A_RK1))
#define LT_NORMAL(asn) (LT_M_BITS((asn),A_RK_MASK,A_RK2))
#define IS_NORMAL(asn) (IS_M_BITS((asn),A_RK_MASK,A_RK2))
#define GT_NORMAL(asn) (GT_M_BITS((asn),A_RK_MASK,A_RK2))
#define SET_NORMAL(asn) (SET_M_BITS((asn),A_RK_MASK,A_RK2))
#define IS_SENIOR(asn) (IS_M_BITS((asn),A_RK_MASK,A_RK2+A_RK1))
#define GT_SENIOR(asn) (GT_M_BITS((asn),A_RK_MASK,A_RK2+A_RK1))
#define SET_SENIOR(asn) (SET_M_BITS((asn),A_RK_MASK,A_RK2+A_RK1))
#define IS_OFFICER(asn) (IS_M_BITS((asn),A_RK_MASK,A_RK3))
#define GT_OFFICER(asn) (GT_M_BITS((asn),A_RK_MASK,A_RK3))
#define SET_OFFICER(asn) (SET_M_BITS((asn),A_RK_MASK,A_RK3))
#define IS_DEPUTY(asn) (IS_M_BITS((asn),A_RK_MASK,A_RK3+A_RK1))
#define GT_DEPUTY(asn) (GT_M_BITS((asn),A_RK_MASK,A_RK3+A_RK1))
#define SET_DEPUTY(asn) (SET_M_BITS((asn),A_RK_MASK,A_RK3+A_RK1))
#define IS_LEADER(asn) (IS_M_BITS((asn),A_RK_MASK,A_RK3+A_RK2))
#define GT_LEADER(asn) (GT_M_BITS((asn),A_RK_MASK,A_RK3+A_RK2))
#define SET_LEADER(asn) (SET_M_BITS((asn),A_RK_MASK,A_RK3+A_RK2))
#define IS_GOD(asn) (IS_M_BITS((asn),A_RK_MASK,A_RK_MASK))
#define SET_GOD(asn) (SET_M_BITS((asn),A_RK_MASK,A_RK_MASK))
#define IS_STT(asn) ((asn) & A_STATICTITLE)
#define IS_HIDDENTITLE(asn) ((asn) & A_HIDETITLE)
#define IS_HIDDENSUBTITLE(asn) ((asn) & A_HIDESUBTITLE)

/* get rank number 0-7 with enemy=0 and god=7 right now */
#define NR_RANK(asn) (GET_M_BITS((asn),A_RK_MASK) >> 2)

/* protect higher routine from knowing specific flags */
#define IS_DEBT(asn) ((asn) & A_DEBT)
#define SET_DEBT(asn) ((asn) |= A_DEBT)
#define REMOVE_DEBT(asn) ((asn) &= ~A_DEBT)
#define IS_CHALL(asn) ((asn) & A_CHALL)
#define SET_CHALL(asn) ((asn) |= A_CHALL)
#define REMOVE_CHALL(asn) ((asn) &= ~A_CHALL)
#define SET_HIDETITLE(asn) ((asn) |= A_HIDETITLE)
#define SET_HIDESUBTITLE(asn) ((asn) |= A_HIDESUBTITLE)

struct guild_frags
{
   int frags;
   char g_name[80];
   char g_topfragger[80];
   int top_frags;
   int members;
   int loaded;
};
//extern struct guild_frags guild_frags_data[];

int do_soc_ledger(P_char ch);
void insert_guild_transaction(int soc_id, char *buff);

int get_assoc_prestige(int assoc_id);
void set_assoc_prestige(int assoc_id, int prestige);
void add_assoc_prestige(int assoc_id, int prestige);
int get_assoc_cps(int assoc_id);
void add_assoc_cps(int assoc_id, int cps);
void set_assoc_cps(int assoc_id, int cps);
void add_assoc_overmax(int assoc_id, int overmax);
int get_assoc_overmax(int assoc_id);
void show_prestige_list(P_char ch);
void reload_assoc_table();
void prestige_update();
string get_assoc_name(int assoc_id);
void set_assoc_active(int assoc_id, bool active);
void check_assoc_prestige_epics(P_char ch, int epics, int epic_type);

int max_assoc_size(int asc_number);
bool is_clan(int asc_number);
bool is_guild(int asc_number);
bool is_kingdom(int asc_number);

#endif // _ASSOCS_H_
