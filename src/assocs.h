/************************************************************************
* assocs.h - defines used for assocs.c                                  *
*                                                                       *
* a collection of useful defines for accessing association data         *
* done 1996 by Krov (Ingo Bojak) for Duris DikuMUD                      *
* Overhauled to use OOP by Lohrr 2016                                   *
************************************************************************/

#ifndef _ASSOCS_H_
#define _ASSOCS_H_

#include <stdio.h>
#include "structs.h"

/* DEFINES for assocs.c */

/* defines allowing access to player association data */ 
#define GET_ASSOC(ch) ((ch)->specials.guild)
#define GET_ASSOC_ID(ch) (GET_ASSOC(ch) ? GET_ASSOC(ch)->get_id() : 0)
#define GET_A_BITS(ch) ((ch)->specials.guild_status)

/* maximum/minimum numbers of associations */
#define MAX_ASC           999       /* maximal number of associations */
#define ASC_MAX_ENEMIES    10       /* maximal number of non-member enemies */
#define ASC_MIN_LEVEL      25       /* minimum level to join */
#define ASC_MAX_DISPLAY   200       /* max number of shown people on soc list */

/* Note that the following is including ansi color chars; displayed
 * strings are much shorter due to color.
 */
#define ASC_MAX_STR_RANK        81       // maximum length of rank name + '\0'
#define ASC_MAX_STR             81       // maximum length of association name + '\0'
#define ASC_MAX_STR_TITLE      162       // maximum length title = rank + ' ' + asc_name + '\0'
#define ASC_MAX_STR_TITLE_ANSI  80       // maximum printed characters in title.

/* saves definitions */
#define ASC_DIR "Players/Assocs/"
#define ASC_SAVE_SIZE  16384

/* let's note waste memory if not necessary... 300 chars should be plenty */
#define MAX_STR_NORMAL  300

/* definition of association bits, as compact as possible to leave
   room for later extensions */
/* player bits */
#define A_SF1           BIT_1 /* SF2,SF1        00:no thanks. 01:applicant   */
#define A_SF2           BIT_2 /* society flags  10:member. 11:banned.        */
#define A_RK1           BIT_3 /* RK3,RK2,RK1  000:enemy. 001:on parole.      */
#define A_RK2           BIT_4 /* rank   010:normal. 011:senior. 100:officer. */
#define A_RK3           BIT_5 /* bits   101:deputy. 110:leader. 111:god.     */
#define A_DEBT          BIT_6 /* 0=no debts, 1=has debts                     */
#define A_STATICTITLE   BIT_7 /* change title on logon?                      */

#define ASC_NUM_RANKS 8
#define A_RANK_BITS(bits) (bits & ( A_RK3 | A_RK2 | A_RK1 ))
#define A_GET_RANK(ch)    (A_RANK_BITS( GET_A_BITS(ch) ))
#define A_GOD     (A_RK3 | A_RK2 | A_RK1)
#define A_LEADER  (A_RK3 | A_RK2)
#define A_DEPUTY  (A_RK3 | A_RK1)
#define A_OFFICER (A_RK3)
#define A_SENIOR  (A_RK2 | A_RK1)
#define A_NORMAL  (A_RK2)
#define A_PAROLE  (A_RK1)
#define A_ENEMY   (0)

/* association bits */
#define A_HIDESUBTITLE 16384u /* hide sub titles of guild            */
#define A_CHALL        32768u /* does the association allow challenges? */
#define A_HIDETITLE    65536u /* do members get their titles reset?     */

#define A_SF_MASK    (A_SF1 | A_SF2)
#define A_RK_MASK    (A_RK3 | A_RK2 | A_RK1)
#define A_SF_BITS(bits) (bits & ( A_SF1 | A_SF2 ))

/* this is used to protect player bits! UPDATE if more bits are used for player */
#define A_P_MASK (A_SF_MASK | A_RK_MASK | A_DEBT | A_STATICTITLE)
#define ASC_MASK (~A_P_MASK)

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
#define IS_ASSOC_MEMBER(ch, guild) ( GET_ASSOC(ch) == guild && IS_MEMBER(GET_A_BITS(ch)) && GT_PAROLE(GET_A_BITS(ch)) )

/* protect higher routines from knowing rank bits */
#define IS_ENEMY(asn)     (IS_M_BITS((asn),  A_RK_MASK, A_ENEMY))
#define GT_ENEMY(asn)     (GT_M_BITS((asn),  A_RK_MASK, A_ENEMY))
#define SET_ENEMY(asn)    (SET_M_BITS((asn), A_RK_MASK, A_ENEMY))
#define IS_PAROLE(asn)    (IS_M_BITS((asn),  A_RK_MASK, A_PAROLE))
#define GT_PAROLE(asn)    (GT_M_BITS((asn),  A_RK_MASK, A_PAROLE))
#define SET_PAROLE(asn)   (SET_M_BITS((asn), A_RK_MASK, A_PAROLE))
#define LT_NORMAL(asn)    (LT_M_BITS((asn),  A_RK_MASK, A_NORMAL))
#define IS_NORMAL(asn)    (IS_M_BITS((asn),  A_RK_MASK, A_NORMAL))
#define GT_NORMAL(asn)    (GT_M_BITS((asn),  A_RK_MASK, A_NORMAL))
#define SET_NORMAL(asn)   (SET_M_BITS((asn), A_RK_MASK, A_NORMAL))
#define IS_SENIOR(asn)    (IS_M_BITS((asn),  A_RK_MASK, A_SENIOR))
#define GT_SENIOR(asn)    (GT_M_BITS((asn),  A_RK_MASK, A_SENIOR))
#define SET_SENIOR(asn)   (SET_M_BITS((asn), A_RK_MASK, A_SENIOR))
#define IS_OFFICER(asn)   (IS_M_BITS((asn),  A_RK_MASK, A_OFFICER))
#define GT_OFFICER(asn)   (GT_M_BITS((asn),  A_RK_MASK, A_OFFICER))
#define SET_OFFICER(asn)  (SET_M_BITS((asn), A_RK_MASK, A_OFFICER))
#define IS_DEPUTY(asn)    (IS_M_BITS((asn),  A_RK_MASK, A_DEPUTY))
#define GT_DEPUTY(asn)    (GT_M_BITS((asn),  A_RK_MASK, A_DEPUTY))
#define SET_DEPUTY(asn)   (SET_M_BITS((asn), A_RK_MASK, A_DEPUTY))
#define IS_LEADER(asn)    (IS_M_BITS((asn),  A_RK_MASK, A_LEADER))
#define GT_LEADER(asn)    (GT_M_BITS((asn),  A_RK_MASK, A_LEADER))
#define SET_LEADER(asn)   (SET_M_BITS((asn), A_RK_MASK, A_LEADER))
#define IS_GOD(asn)       (IS_M_BITS((asn),  A_RK_MASK, A_RK_MASK))
#define SET_GOD(asn)      (SET_M_BITS((asn), A_RK_MASK, A_RK_MASK))
#define IS_STT(asn)       (IS_SET((asn), A_STATICTITLE))
#define SET_STT(asn)      (SET_BIT((asn), A_STATICTITLE))
#define REMOVE_STT(asn)   (REMOVE_BIT((asn), A_STATICTITLE))
#define IS_HIDDENTITLE(asn) ((asn) & A_HIDETITLE)
#define IS_HIDDENSUBTITLE(asn) ((asn) & A_HIDESUBTITLE)

/* get rank number 0-7 with enemy=0 and god=7 right now */
#define NR_RANK(asn) (GET_M_BITS((asn),A_RK_MASK) >> 2)
#define GET_RK_BITS(bits) (GET_M_BITS((bits), A_RK_MASK))

/* protect higher routine from knowing specific flags */
#define IS_DEBT(asn) ((asn) & A_DEBT)
#define HAS_FINE( ch )  ( IS_DEBT(GET_A_BITS( ch )) )
#define SET_DEBT(asn) (SET_BIT(asn, A_DEBT))
#define REMOVE_DEBT(asn) (REMOVE_BIT(asn, A_DEBT))
#define IS_CHALL(asn) ((asn) & A_CHALL)
#define SET_CHALL(asn) ((asn) |= A_CHALL)
#define REMOVE_CHALL(asn) ((asn) &= ~A_CHALL)
#define SET_HIDETITLE(asn) ((asn) |= A_HIDETITLE)
#define SET_HIDESUBTITLE(asn) ((asn) |= A_HIDESUBTITLE)

#define GSTAT_OFFLINE   0
#define GSTAT_LINKDEAD  1
#define GSTAT_ONLINE    2

// Contained by the Guild class.
struct guild_frags
{
  long frags;
  long top_frags;
  char topfragger[MAX_NAME_LENGTH + 1];
};

typedef struct guild_member *P_member;
struct guild_member
{
  char name[MAX_NAME_LENGTH + 1];
  unsigned int bits;
  unsigned int debt; // In copper
  ushort online_status;
  P_member next;
};

// Used to create frag guild.
struct guild_frag_info
{
    char guild_name[MAX_INPUT_LENGTH];
    long tot_frags;
    // + 1 for the string terminator.
    char top_fragger[MAX_NAME_LENGTH + 1];
    long top_frags;
    int  num_members;
};

typedef struct Guild * P_Guild;

class Guild
{
  public:
    unsigned int get_id( ) { if( this == NULL ) return 0; else return id_number; }
    unsigned int get_racewar() { return racewar; }
    char *get_top_fragger() { return frags.topfragger; }
    long get_top_frags() { return frags.top_frags; }
    unsigned int get_num_members() { return member_count; }
    unsigned int get_max_members();

    string get_name() { return string(name); }
    void set_name( char *new_name ) { sprintf( name, "%s", new_name ); save(); }
    void set_racewar( int new_racewar ) { if( (new_racewar > 0) && (new_racewar <= MAX_RACEWAR) ) racewar = new_racewar; save(); }

    unsigned long get_prestige( ) { return prestige; }
    void add_prestige( int prest ) { prestige += prest; save(); }
    void set_prestige( int prest ) { prestige = prest; save(); }
    void add_points_from_epics( P_char ch, int epics, int epic_type );

    unsigned long get_construction( ) { return construction; }
    void add_construction( int cps ) { construction += cps; save(); }
    void set_construction( int cps ) { construction = cps; save(); }

    void set_bits( unsigned int new_bits ) { bits = new_bits; save(); }

    unsigned int get_overmax( ) { return overmax; }
    void add_overmax( unsigned int _overmax ) { overmax += _overmax; save(); }
    unsigned int max_size( );

    long get_frags( ) { return frags.frags; }
    void add_frags( P_char ch, long new_frags );
    void set_frags( long new_frags ) { frags.frags = new_frags; save(); }
    void frag_remove( P_char member );

    void apply(P_char member, P_char applicant );
    bool add_member( P_char ch, int rank );
    int update( );
    void update_member( P_char ch );
    void update_bits( P_char ch );
    void secede( P_char ch );

    bool is_allied_with( P_Guild ally);
    bool is_enemy( P_char enemy );
    P_Alliance get_alliance( );
    P_Guild next() { return next_guild; }
    void set_next( P_Guild nGuild ) { next_guild = nGuild; }
    void display( P_char ch );

    void ledger( P_char ch, char *args );
    bool is_clan( );
    bool is_guild( );
    bool is_kingdom( );
    void kick( P_char victim );
    void kick( P_char kicker, char *char_name );
    bool sub_money( int p, int g, int s, int c );

    void challenge( P_char member, P_char victim );
    void deposit( P_char member, int p, int g, int s, int c );
    void enroll( P_char member, P_char victim);
    void fine( P_char member, char *target, int p, int g, int s, int c );
    void home( P_char member );
    void ostracize( P_char member, char *target );
    void punish( P_char member, P_char victim );
    void rank( P_char member, P_char victim, char *rank_change );
    void title( P_char member, P_char victim, char *new_title );
    void withdraw( P_char member, int p, int g, int s, int c );

    void name_title( P_char member, char *title_info );

    void save( );

    static void initialize();
    Guild( char *_name, unsigned int _racewar, unsigned int _id_number, unsigned long _prestige,
      unsigned long _construction, unsigned long _money, unsigned int _bits );
    Guild( );
    ~Guild( );

  protected:
    static bool load_guild( int guild_num );
    void remove_member_from_list( P_char ch );
    void write_transaction_to_ledger( char *name, char *trans_type, char *coin_str );
    void title_trim( char *raw_title, char *good_title );
    int max_assoc_size();
    void default_title( P_char ch );
    void update_online_members();

    char name[ASC_MAX_STR];
    char titles[ASC_NUM_RANKS][ASC_MAX_STR_RANK];
    unsigned int  racewar;
    unsigned int  id_number;
    unsigned long prestige;
    unsigned long construction;
    unsigned int  platinum, gold, silver, copper;
    unsigned int  member_count;
    unsigned int  bits;
    unsigned int  overmax;
    struct guild_frags frags;
    P_member members;
    P_Guild next_guild;
};

void sql_update_assoc_table( );
void prestige_update();
void show_prestige_list( P_char ch );
void show_guild_frags( P_char ch );
P_Guild get_guild_from_id( int id_num );
bool found_asc(P_char god, P_char leader, char *bits, char *asc_name);

void do_supervise(P_char, char *, int);
void do_asclist(P_char, char *, int);
void do_society(P_char, char *, int);
void do_gmotd(P_char, char *, int);

#endif // _ASSOCS_H_
