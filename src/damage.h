#ifndef _DAMAGE_H_
#define _DAMAGE_H_

#define DAM_NONEDEAD    0
#define DAM_VICTDEAD    1
#define DAM_CHARDEAD    -1
#define DAM_BOTHDEAD    -2

#define DAMMSG_HIT_EFFECT BIT_1
#define DAMMSG_EFFECT_HIT BIT_2
#define DAMMSG_EFFECT     BIT_3
#define DAMMSG_KICK       BIT_4
#define DAMMSG_TERSE      BIT_5
#define DAMMSG_HIT        BIT_6

#define DF_SANC                  0
#define DF_NPCTOPC               1
#define DF_TROLLSKIN             2
#define DF_BARKSKIN              3
#define DF_SOULMELEE             4
#define DF_SOULSPELL             5
#define DF_PROTLIVING            6
#define DF_PROTANIMAL            7
#define DF_PROTECTION            8
#define DF_ELSHIELDRED           9
#define DF_IRONWILL             10
#define DF_ELAFFINITY           11
#define DF_COLDWRITHE           12
#define DF_BARKFIRE             13
#define DF_ELEMENTALIST         14
#define DF_ELSHIELDINC          15
#define DF_PHANTFORM            16
#define DF_VULNCOLD             17
#define DF_VULNFIRE             18
#define DF_ELSHIELDDAM          19
#define DF_NEGSHIELD            20
#define DF_SOULSHIELDDAM        21
#define DF_MONKVAMP             22
#define DF_TOUCHVAMP            23
#define DF_TRANCEVAMP           24
#define DF_UNDEADVAMP           25
#define DF_BTXVAMP              26
#define DF_NPCVAMP              27
#define DF_HFIREVAMP            28
#define DF_HOLYSACVAMP          29
#define DF_WEAPON_DICE          30
#define DF_WETFIRE              31
#define DF_IRONWOOD             32
#define DF_BATTLETIDEVAMP       33
#define DF_SLSHIELDINCREASE     34
#define DF_BERSERKMELEE         35
#define DF_BERSERKSPELL         36
#define DF_CHAOSSHIELD          37
#define DF_DIVINEFORCE          38
#define DF_BERSERKRAGE          39
#define DF_SWASHBUCKLER_DEFENSE 40
#define DF_BERSERKEREXTRA       41
#define DF_RAGED                42
#define DF_SWASHBUCKLER_OFFENSE 43
#define DF_ENERGY_CONTAINMENT   44
#define DF_PROTECTION_TROLL     45
#define DF_ELSHIELDRED_TROLL    46
#define DF_NEG_SHIELD_SPELL     47
#define DF_GUARDIANS_BULWARK    48
#define DF_TIGERPALM            49
#define DF_DAMROLL_MOD          50
#define DF_MELEEMASTERY         51
#define DF_DRACOLICHVAMP        52
#define LAST_DF                 52

#define SPLDAM_GENERIC   1
#define SPLDAM_FIRE      2
#define SPLDAM_COLD      3
#define SPLDAM_LIGHTNING 4
#define SPLDAM_GAS       5
#define SPLDAM_ACID      6
#define SPLDAM_NEGATIVE  7
#define SPLDAM_HOLY      8
#define SPLDAM_PSI       9
#define SPLDAM_SPIRIT   10
#define SPLDAM_SOUND    11
#define SPLDAM_EARTH    12
#define LAST_SPLDAM_TYPE 12

#define ELEMENTAL_DAM(type) ((type >= SPLDAM_FIRE && type <= SPLDAM_ACID) || (type == SPLDAM_EARTH))

/* all flags below share the same vector */
#define PHSDAM_TOUCH      BIT_1
#define PHSDAM_NOREDUCE   BIT_2
#define PHSDAM_NOSHIELDS  BIT_3
#define PHSDAM_HELLFIRE   BIT_4
#define PHSDAM_NOPOSITION BIT_5
#define PHSDAM_NOENGAGE   BIT_6
#define PHSDAM_BATTLETIDE BIT_7
#define PHSDAM_ARROW      BIT_8

#define SPLDAM_NOSHRUG     BIT_10
#define SPLDAM_BREATH      BIT_11
#define SPLDAM_MINORGLOBE  BIT_12
#define SPLDAM_SPIRITWARD  BIT_13
#define SPLDAM_GLOBE       BIT_14
#define SPLDAM_GRSPIRIT    BIT_15
#define SPLDAM_NODEFLECT   BIT_16
#define SPLDAM_NOVAMP      BIT_17
#define SPLDAM_SPELL       BIT_18

#define RAWDAM_TRANCEVAMP    BIT_20
#define RAWDAM_BTXVAMP       BIT_21
#define RAWDAM_HOLYSAC       BIT_22
#define RAWDAM_SANCTUARY     BIT_23
#define RAWDAM_IMPRISON      BIT_24
#define RAWDAM_VINES         BIT_25
#define RAWDAM_NOKILL        BIT_26
#define RAWDAM_NOEXP         BIT_27
#define RAWDAM_SHRTVMP       BIT_28
#define RAWDAM_DEFAULT (RAWDAM_TRANCEVAMP | RAWDAM_BTXVAMP | RAWDAM_SHRTVMP | \
  RAWDAM_HOLYSAC | RAWDAM_SANCTUARY | RAWDAM_IMPRISON | RAWDAM_VINES)

#define SPLDAM_ALLGLOBES   SPLDAM_MINORGLOBE | SPLDAM_GLOBE |\
                           SPLDAM_SPIRITWARD | SPLDAM_GRSPIRIT
  

#define MSG_HIT                     0
#define MSG_BLUDGEON                1
#define MSG_PIERCE                  2
#define MSG_SLASH                   3
#define MSG_WHIP                    4
#define MSG_CLAW                    5
#define MSG_BITE                    6
#define MSG_STING                   7
#define MSG_CRUSH                   8
#define MSG_MAUL                    9
#define MSG_THRASH                 10
#define MSG_TOUCH                  11

#endif

