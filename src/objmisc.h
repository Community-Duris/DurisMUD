/* materials stuff..  feel free to add the standard header if ya want */

#ifndef _OBJMISC_H_

#ifndef _DE_
#include "structs.h"
#endif

// Used in val0 of items of type ITEM_TOTEM - specifies what sphere shaman can cast with the totem
#define TOTEM_LESS_ANIM   1     // Lesser Animal
#define TOTEM_GR_ANIM     2     // Greater Animal
#define TOTEM_LESS_ELEM   4     // Elemental
#define TOTEM_GR_ELEM     8
#define TOTEM_LESS_SPIR  16     // Spirit
#define TOTEM_GR_SPIR    32

 /* obj->material */

#define MAT_UNDEFINED  0  /* lazy builders, no bonuses */
#define MAT_LOWEST     1
#define MAT_NONSUBSTANTIAL 1
#define MAT_FLESH      2
#define MAT_CLOTH      3
#define MAT_BARK       4
#define MAT_SOFTWOOD   5
#define MAT_HARDWOOD   6
#define MAT_SILICON    7
#define MAT_CRYSTAL    8
#define MAT_CERAMIC    9
#define MAT_BONE      10
#define MAT_STONE     11
#define MAT_HIDE      12
#define MAT_LEATHER   13
#define MAT_CURED_LEATHER 14
#define MAT_IRON      15
#define MAT_STEEL     16
#define MAT_BRASS     17
#define MAT_MITHRIL   18
#define MAT_ADAMANTIUM 19
#define MAT_BRONZE    20
#define MAT_COPPER    21
#define MAT_SILVER    22
#define MAT_ELECTRUM  23
#define MAT_GOLD      24
#define MAT_PLATINUM  25
#define MAT_GEM       26
#define MAT_DIAMOND   27
#define MAT_PAPER     28
#define MAT_PARCHMENT 29
#define MAT_LEAVES    30
#define MAT_RUBY      31
#define MAT_EMERALD   32
#define MAT_SAPPHIRE  33
#define MAT_IVORY     34
#define MAT_DRAGONSCALE 35
#define MAT_OBSIDIAN  36
#define MAT_GRANITE   37
#define MAT_MARBLE    38
#define MAT_LIMESTONE 39
#define MAT_LIQUID    40
#define MAT_BAMBOO    41
#define MAT_REEDS     42
#define MAT_HEMP      43
#define MAT_GLASSTEEL 44
#define MAT_EGGSHELL  45
#define MAT_CHITINOUS 46
#define MAT_REPTILESCALE 47
#define MAT_GENERICFOOD 48
#define MAT_RUBBER    49
#define MAT_FEATHER   50
#define MAT_WAX       51
#define MAT_PEARL     52
#define MAT_TIN       53

#define MAT_HIGHEST   53

#define IS_METAL(mat) ( mat == MAT_IRON || mat == MAT_STEEL || \
                        mat == MAT_COPPER || mat == MAT_BRASS || \
                        mat == MAT_ADAMANTIUM || mat == MAT_GOLD || \
                        mat == MAT_SILVER || mat == MAT_PLATINUM || \
                        mat == MAT_MITHRIL || mat == MAT_ELECTRUM || mat == MAT_TIN )
#define IS_STONE(mat) ( mat == MAT_GRANITE || mat == MAT_GEM || \
                        mat == MAT_EMERALD || mat == MAT_SAPPHIRE || \
                        mat == MAT_OBSIDIAN || mat == MAT_MARBLE || \
                        mat == MAT_LIMESTONE || mat == MAT_RUBY || \
                        mat == MAT_DIAMOND || mat == MAT_CRYSTAL || \
                        mat == MAT_STONE || mat == MAT_CERAMIC )
#define IS_LEATHER(mat) ( mat == MAT_HIDE || mat == MAT_REPTILESCALE || \
                          mat == MAT_CURED_LEATHER || mat == MAT_LEATHER || \
                          mat == MAT_DRAGONSCALE )
#define IS_WOODEN(mat) ( mat == MAT_BAMBOO || mat == MAT_SOFTWOOD || \
                         mat == MAT_HARDWOOD || mat == MAT_BARK )
#define IS_CLOTH(mat) ( mat == MAT_REEDS || mat == MAT_HEMP || \
                        mat == MAT_LEAVES || mat == MAT_PARCHMENT || \
                        mat == MAT_PAPER || mat == MAT_CLOTH || \
                        mat == MAT_FEATHER )
#define IS_PLASTIC(mat) ( mat == MAT_WAX || mat == MAT_RUBBER || \
                          mat == MAT_GENERICFOOD || mat == MAT_FLESH || \
                          mat == MAT_SILICON )
#define IS_RIGID(mat) ( IS_METAL(mat) || IS_STONE(mat) || IS_WOODEN(mat) || \
                        mat == MAT_IVORY || mat == MAT_PEARL || \
                        mat == MAT_CHITINOUS || mat == MAT_BONE )

#define NUMB_MATERIALS (MAT_HIGHEST + 1)

#define OBJCRAFT_LOWEST     0
#define OBJCRAFT_TERRIBLE   0
#define OBJCRAFT_E_POOR     1
#define OBJCRAFT_V_POOR     2
#define OBJCRAFT_F_POOR     3
#define OBJCRAFT_WELL_BELOW 4
#define OBJCRAFT_BELOW      5
#define OBJCRAFT_S_BELOW    6
#define OBJCRAFT_AVERAGE    7
#define OBJCRAFT_S_ABOVE    8
#define OBJCRAFT_ABOVE      9
#define OBJCRAFT_W_ABOVE   10
#define OBJCRAFT_EXCELLENT 11
#define OBJCRAFT_SKILLED   12
#define OBJCRAFT_V_SKILLED 13
#define OBJCRAFT_MASTER    14
#define OBJCRAFT_ONE_KIND  15
#define OBJCRAFT_HIGHEST   15

/* assorted armor info */

// primary armor locations when worn

#define ARMOR_WEAR_LOWEST       1
#define ARMOR_WEAR_FINGER       1
#define ARMOR_WEAR_NECK         2
#define ARMOR_WEAR_BODY         3
#define ARMOR_WEAR_HEAD         4
#define ARMOR_WEAR_LEGS         5
#define ARMOR_WEAR_FEET         6
#define ARMOR_WEAR_HANDS        7
#define ARMOR_WEAR_ARMS         8
#define ARMOR_WEAR_ABOUT        9
#define ARMOR_WEAR_WAIST       10
#define ARMOR_WEAR_WRIST       11
#define ARMOR_WEAR_EYES        12
#define ARMOR_WEAR_FACE        13
#define ARMOR_WEAR_EAR         14
#define ARMOR_WEAR_QUIVER      15
#define ARMOR_WEAR_BADGE       16
#define ARMOR_WEAR_BACK        17
#define ARMOR_WEAR_ATTACHBELT  18
#define ARMOR_WEAR_HORSEBODY   19
#define ARMOR_WEAR_TAIL        20
#define ARMOR_WEAR_NOSE        21
#define ARMOR_WEAR_HORNS       22
#define ARMOR_WEAR_IOUN        23
#define ARMOR_WEAR_HIGHEST     23

// bitvectors for where armor covers

// break up body, legs, and arms a fair amount since they're rather large
// areas compared to everything else - joints are covered separately ..
// will probably make armor that covers a joint hamper dex/agi/move a bit
// automatically based on material type and maybe weight (and of course
// which joint it is..)

#define NUMB_ARMOR_BODY_FLAGS    8
#define ARMOR_BODY_ALL           BIT_1
#define ARMOR_BODY_FRONT_UPPER   BIT_2
#define ARMOR_BODY_FRONT_LOWER   BIT_3
#define ARMOR_BODY_BACK_UPPER    BIT_4
#define ARMOR_BODY_BACK_LOWER    BIT_5
#define ARMOR_BODY_RIBS_UPPER    BIT_6
#define ARMOR_BODY_RIBS_LOWER    BIT_7
#define ARMOR_BODY_SHOULDERS     BIT_8

#define NUMB_ARMOR_LEGS_FLAGS    7
#define ARMOR_LEGS_ALL           BIT_1
#define ARMOR_LEGS_FRONT_UPPER   BIT_2  // above knees
#define ARMOR_LEGS_FRONT_KNEES   BIT_3
#define ARMOR_LEGS_FRONT_LOWER   BIT_4  // below knees
#define ARMOR_LEGS_BACK_UPPER    BIT_5
#define ARMOR_LEGS_BACK_KNEES    BIT_6
#define ARMOR_LEGS_BACK_LOWER    BIT_7

#define NUMB_ARMOR_ARMS_FLAGS    7
#define ARMOR_ARMS_ALL           BIT_1
#define ARMOR_ARMS_FRONT_UPPER   BIT_2  // above the elbow
#define ARMOR_ARMS_FRONT_ELBOW   BIT_3
#define ARMOR_ARMS_FRONT_LOWER   BIT_4  // below the elbow
#define ARMOR_ARMS_BACK_UPPER    BIT_5
#define ARMOR_ARMS_BACK_ELBOW    BIT_6
#define ARMOR_ARMS_BACK_LOWER    BIT_7

// horsebody stuff

#define NUMB_ARMOR_HORSE_FLAGS   8
#define ARMOR_HORSE_ALL          BIT_1
#define ARMOR_HORSE_FRONT_TOP    BIT_2  // from middle to head - let's say
                                        // saddles go here
#define ARMOR_HORSE_FRONT_SIDES  BIT_3
#define ARMOR_HORSE_FRONT_BOTTOM BIT_4
#define ARMOR_HORSE_BACK_TOP     BIT_5  // from middle to ass
#define ARMOR_HORSE_BACK_SIDES   BIT_6
#define ARMOR_HORSE_BACK_BOTTOM  BIT_7
#define ARMOR_HORSE_ASS_END      BIT_8  // the ass end of the horse, baby

// these are broken up quite a bit because what happens to you when hit
// on the head is heavily dependent on where exactly you are hit

#define NUMB_ARMOR_HEAD_FLAGS    9
#define ARMOR_HEAD_ALL           BIT_1
#define ARMOR_HEAD_FRONT_UPPER   BIT_2  // face area above bottom of nose
#define ARMOR_HEAD_SIDES_UPPER   BIT_3  // ears
#define ARMOR_HEAD_BACK_UPPER    BIT_4  // back of head, skull itself
#define ARMOR_HEAD_CROWN         BIT_5  // top of head - lights out, mommy
#define ARMOR_HEAD_FRONT_LOWER   BIT_6  // face area below nose
#define ARMOR_HEAD_SIDES_LOWER   BIT_7  // side of jaw
#define ARMOR_HEAD_BACK_LOWER    BIT_8  // back of head, where skull stops
                                        // and flesh of neck begins
#define ARMOR_HEAD_CHIN          BIT_9  // a dagger through the bottom of
                                        // the chin.  shrug.

// no need to really break these up, you get hit in the front of the neck and
// you're generally fucked - back of neck, maybe not as bad, unless it's a
// hard crushing blow or a REALLY hard slashing blow.

#define NUMB_ARMOR_NECK_FLAGS    3
#define ARMOR_NECK_ALL           BIT_1
#define ARMOR_NECK_FRONT         BIT_2
#define ARMOR_NECK_BACK          BIT_3

// doubt many people are gonna be taking foot shots, but ..

#define NUMB_ARMOR_FEET_FLAGS    5
#define ARMOR_FEET_ALL           BIT_1
#define ARMOR_FEET_TOP           BIT_2
#define ARMOR_FEET_BOTTOM        BIT_3
#define ARMOR_FEET_TOES          BIT_4  // this little piggy got cut off
#define ARMOR_FEET_BACK          BIT_5  // feel the pain of everyone..

// hands are pretty important but there's not much there, so not many
// 'slots'

#define NUMB_ARMOR_HANDS_FLAGS   4
#define ARMOR_HANDS_ALL          BIT_1
#define ARMOR_HANDS_FINGERS      BIT_2  // not all gloves have fingers
#define ARMOR_HANDS_HAND         BIT_3  // the 'main' area of the hand itself
#define ARMOR_HANDS_WRIST        BIT_4

// about-body stuff is generally assumed to cover the back, but not be
// another breastplate or heavy armor..  hmm.  this one is a bit ambiguous,
// so we'll see what all needs to be changed, if anything

// cloaks could technically also cover the arms, but ..  this is good
// enough

#define NUMB_ARMOR_ABOUT_FLAGS   6
#define ARMOR_ABOUT_ALL          BIT_1
#define ARMOR_ABOUT_BACK_UPPER   BIT_2
#define ARMOR_ABOUT_BACK_LOWER   BIT_3
#define ARMOR_ABOUT_RIBS_UPPER   BIT_4
#define ARMOR_ABOUT_RIBS_LOWER   BIT_5
#define ARMOR_ABOUT_SHOULDERS    BIT_6

// heh, the wonderful waist slot.  not much here to cover.  hell, I
// could probably assume all waist slot items are belts, but..

// really, waist items aren't gonna be worth jack shit..  I guess we
// could check if you hit the guy in the waist everytime he gets hit
// in the lower torso - but I'm not real sure what the 'waist' area is
// exactly - what, about 2"?  heh

// this is another candidate for items that don't protect but just imbue
// magical crap onto you

/*
#define NUMB_ARMOR_WAIST_FLAGS   3
#define ARMOR_WAIST_ALL          BIT_1
#define ARMOR_WAIST_FRONT        BIT_2
#define ARMOR_WAIST_BACK         BIT_3
*/

// wristwear includes bracers, so let's allow it to give some protection
// to the lower arms if they're not wimpy bracelets

#define NUMB_ARMOR_WRIST_FLAGS       5
#define ARMOR_WRIST_ALL              BIT_1
#define ARMOR_WRIST_LOWER_ARMS_FRONT BIT_2
#define ARMOR_WRIST_LOWER_ARMS_BACK  BIT_3
#define ARMOR_WRIST_FRONT            BIT_4
#define ARMOR_WRIST_BACK             BIT_5

// eyewear - yes, that's right, eyepatches that cover up yer damn eye are
// gonna reduce dexterity and agility and whatever else I can come up with..
// so we have crap for specific eyes and for transparent eyewear (glasses,
// magical patches that you can see through one way but not the other,
// whatever).  note that there is no 'ALL' flag for this since that wouldn't
// make any sense

#define NUMB_ARMOR_EYES_FLAGS         4
#define ARMOR_EYE_LEFT               BIT_1
#define ARMOR_EYE_RIGHT              BIT_2
#define ARMOR_EYE_LEFT_TRANSPARENT   BIT_3
#define ARMOR_EYE_RIGHT_TRANSPARENT  BIT_4

// facewear.. this is interesting, it's a bit redundant with headwear, but..
// let's just assume that facewear never guards the sides of the head or the
// eyes..  that works nicely

#define NUMB_ARMOR_FACE_FLAGS        3
#define ARMOR_FACE_ALL               BIT_1
#define ARMOR_FACE_UPPER             BIT_2  // from bottom of nose up
#define ARMOR_FACE_LOWER             BIT_3  // below nose

// earrings.  now this is really interesting.  I guess they could guard
// the side of the face and of course your ears.  but they're generally
// tiny..  why the hell are men running around with earrings in their
// ears, anyway?  whoever thought up these slots is batty

// how many earrings cover up your whole ear?  about none?  bleh..  this
// slot is hard to make fit in with the new combat system

// might just make earrings useless for protection and thusly only good
// for their magical properties.  and of course, to look sexy for all your
// other manly buddies

/*
#define NUMB_ARMOR_EARS_FLAGS       3
#define ARMOR_EARS_ALL              BIT_1
#define ARMOR_EARS_UPPER_FACE_SIDE  BIT_2
#define ARMOR_EARS_EARS             BIT_3
*/

// quivers don't provide any reasonable protection, don't bother counting them

// badges..  kinda like earrings.  I'd say the same thing..  they can imbue
// magical properties (+dex, perm haste, whatever) but are useless for
// protection


/* armor types */

#define ARMOR_NONE                  0
#define ARMOR_TYPE_LOWEST           1
#define ARMOR_LEATHER               1
#define ARMOR_STUDDED_LEATHER       2
#define ARMOR_PADDED_LEATHER        3
#define ARMOR_RING                  4
#define ARMOR_HIDE                  5
#define ARMOR_SCALE                 6
#define ARMOR_CHAIN                 7
#define ARMOR_SPLINT                8
#define ARMOR_BANDED                9
#define ARMOR_PLATE                10  // semi-bendable joints
#define ARMOR_FIELD_PLATE     11  // not so bendable
#define ARMOR_FULL_PLATE      12  // neigh near unbendable
#define ARMOR_TYPE_HIGHEST         12

// armor thickness (val3)

// these values aren't really cut-and-dry..  apply as you wish

#define ARMOR_THICKNESS_LOWEST           1
#define ARMOR_THICKNESS_VERY_THIN        1
#define ARMOR_THICKNESS_THIN             2
#define ARMOR_THICKNESS_AVERAGE          3
#define ARMOR_THICKNESS_THICK            4
#define ARMOR_THICKNESS_VERY_THICK       5
#define ARMOR_THICKNESS_HIGHEST          5

/* obj->status */

#define STAT_BRITTLE   1
#define STAT_HEATED    2
#define STAT_ELECTRIFIED 4

/* weapon stuff */

#define WEAPON_LOWEST       0
#define WEAPON_NONE         0
#define WEAPON_AXE          1  // axes - slashing
#define WEAPON_DAGGER       2  // daggers, knives - piercing, slashing (with -)
#define WEAPON_FLAIL        3  // flails - whip
#define WEAPON_HAMMER       4  // hammers - bludgeon
#define WEAPON_LONGSWORD    5  // long swords - slashing/piercing (with -)
#define WEAPON_MACE         6  // mace - bludgeon
#define WEAPON_SPIKED_MACE  7  // should be removed
#define WEAPON_POLEARM      8  // polearm - halberds, guisarmes, glaives - slashing
#define WEAPON_SHORTSWORD   9  // short swords - slashing/piercing
#define WEAPON_CLUB        10  // clubs - bludgeon
#define WEAPON_SPIKED_CLUB 11  // should be removed
#define WEAPON_STAFF       12  // staff - like club but longer, maybe 2-handed - bludgeon
#define WEAPON_2HANDSWORD  13  // should be removed
#define WEAPON_WHIP        14  // whips - whip
#define WEAPON_SPEAR       15  // long piercing weapons - pierce
#define WEAPON_LANCE       16  // lance - special handling probably
#define WEAPON_SICKLE      17  // sickle - slash
#define WEAPON_TRIDENT     18  // forks/rakes - slash
#define WEAPON_HORN        19  // pierce like dagger, should be removed
#define WEAPON_NUMCHUCKS   20  // numchucks - bludgeon
#define WEAPON_HIGHEST     20
#define NUMB_WEAPONS      (WEAPON_HIGHEST - WEAPON_LOWEST)

#define WEAPONTYPE_UNDEFINED  0
#define WEAPONTYPE_BLUDGEON   1
#define WEAPONTYPE_SLASH      2
#define WEAPONTYPE_PIERCE     3
#define WEAPONTYPE_WHIP       4

#define MISSILE_FIRST                   1
#define MISSILE_ARROW                   1
#define MISSILE_LIGHT_CBOW_QUARREL      2
#define MISSILE_HEAVY_CBOW_QUARREL      3
#define MISSILE_HAND_CBOW_QUARREL       4
#define MISSILE_SLING_BULLET            5
#define MISSILE_DART                    6
#define MISSILE_LAST                    6

#define IS_BACKSTABBER(obj) ((obj)->type == ITEM_WEAPON &&\
                             ((obj)->value[0] == WEAPON_DAGGER ||\
                              (obj)->value[0] == WEAPON_SHORTSWORD ||\
                              (obj)->value[0] == WEAPON_TRIDENT ||\
                              (obj)->value[0] == WEAPON_HORN))

#define IS_FLAYING(obj) ((obj)->type == ITEM_WEAPON &&\
                             ((obj)->value[0] == WEAPON_FLAIL ||\
                              (obj)->value[0] == WEAPON_WHIP))

#define IS_SWORD(obj) ((obj)->type == ITEM_WEAPON && \
                       ((obj)->value[0] == WEAPON_LONGSWORD ||\
                        (obj)->value[0] == WEAPON_SHORTSWORD ||\
                        (obj)->value[0] == WEAPON_2HANDSWORD))

#define IS_AXE(obj) ((obj)->type == ITEM_WEAPON && \
                       ((obj)->value[0] == WEAPON_AXE ))

#define IS_BLUDGEON(obj) ((obj)->type == ITEM_WEAPON && \
                          ((obj)->value[0] == WEAPON_HAMMER ||\
                           (obj)->value[0] == WEAPON_MACE ||\
                           (obj)->value[0] == WEAPON_CLUB ||\
                           (obj)->value[0] == WEAPON_NUMCHUCKS ||\
                         (obj)->value[0] == WEAPON_SPIKED_CLUB ||\
                         (obj)->value[0] == WEAPON_SPIKED_MACE ||\
                           (obj)->value[0] == WEAPON_STAFF))

#define IS_LANCE(obj) ((obj)->type == ITEM_WEAPON &&\
                       (obj)->value[0] == WEAPON_LANCE)

#define IS_REACH_WEAPON(obj) ((obj)->type == ITEM_WEAPON &&\
                             IS_SET((obj)->extra_flags, ITEM_TWOHANDS) &&\
                             ((obj)->value[0] == WEAPON_POLEARM ||\
                              (obj)->value[0] == WEAPON_SPEAR ||\
                              (obj)->value[0] == WEAPON_TRIDENT))

#define IS_DART(obj) ((obj)->type == ITEM_MISSILE &&\
                      (obj)->value[3] == MISSILE_DART)

#define IS_DAGGER(obj) ((obj)->type == ITEM_WEAPON && \
                       (obj)->value[0] == WEAPON_DAGGER)

#define IS_DIRK(obj) ((obj)->type == ITEM_WEAPON && \
                     !IS_SET((obj)->extra_flags, ITEM_TWOHANDS) && \
                     ((obj)->value[0] == WEAPON_DAGGER || \
                      (obj)->value[0] == WEAPON_SHORTSWORD))

#define CAN_HURT(ch, obj, mob) (!has_innate(mob, INNATE_WEAPON_IMMUNITY) ||\
      ((obj) && IS_SET((obj)->extra2_flags, ITEM2_MAGIC)) || (!(obj) && (GET_LEVEL(ch) >= 51)))

#define IS_SLAYING(obj, mob) ((IS_SET((obj)->extra2_flags, ITEM2_SLAY_GOOD) && IS_GOOD(mob)) ||\
                              (IS_SET((obj)->extra2_flags, ITEM2_SLAY_EVIL) && IS_EVIL(mob)) ||\
                              (IS_SET((obj)->extra2_flags, ITEM2_SLAY_UNDEAD) && IS_UNDEAD(mob)) ||\
                              (IS_SET((obj)->extra2_flags, ITEM2_SLAY_LIVING) && !IS_UNDEAD(mob)))
  
/* shield stuff */

// type (val0)

#define SHIELDTYPE_LOWEST   1
#define SHIELDTYPE_STRAPARM 1  // strapped to the arm - bucklers
#define SHIELDTYPE_HANDHELD 2  // held by hand
#define SHIELDTYPE_HIGHEST  2

// shape (val1)

#define SHIELDSHAPE_LOWEST   1
#define SHIELDSHAPE_CIRCULAR 1  // perfect circle
#define SHIELDSHAPE_SQUARE   2
#define SHIELDSHAPE_RECTVERT 3  // a rectangle aligned vertically
#define SHIELDSHAPE_RECTHORZ 4  // horizontally..  you never know
#define SHIELDSHAPE_OVALVERT 5  // vertical 'oval'
#define SHIELDSHAPE_OVALHORZ 6  // horizontal 'oval' - you never know
#define SHIELDSHAPE_TRIBIGUP 7  // triangle - wide side on top
#define SHIELDSHAPE_TRISMLUP 8  // triangle - narrow point on top

// dunno what they're called, but the type of shield that is square on
// top and rounded on the bottom should be added

#define SHIELDSHAPE_HIGHEST  8

// size (val2) - used to determine how well it can block (and how well it
//               encumbers your weapon use, perhaps)

#define SHIELDSIZE_LOWEST   1
#define SHIELDSIZE_TINY     1  // really small suckers
#define SHIELDSIZE_SMALL    2  // bucklers, small shields
#define SHIELDSIZE_MEDIUM   3  // normal shields
#define SHIELDSIZE_LARGE    4  // big shields
#define SHIELDSIZE_HUGE     5  // huge shields (might not need this)
#define SHIELDSIZE_HIGHEST  5

#define LISTOBJ_LONGDESC          BIT_1
#define LISTOBJ_SHORTDESC         BIT_2
#define LISTOBJ_ACTIONDESC        BIT_3
#define LISTOBJ_STATS             BIT_4

#ifndef _DE_
int obj_zone_id(P_obj o);
int obj_room_id(P_obj o);
#endif

#define _OBJMISC_H_
#endif
