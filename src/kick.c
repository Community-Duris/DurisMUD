#include <stdio.h>
#include <string.h>

#include "structs.h"
#include "comm.h"
#include "utils.h"
#include "prototypes.h"

const char *att_kick_kill_ch[] = {
  "Your kick caves $N's chest in, killing $M.",
  "Your kick destroys $N's arm and caves in one side of $S rib cage.",
  "Your kick smashes through $N's leg and into $S pelvis, killing $M.",
  "Your kick shatters $N's skull.",
  "Your kick at $N's snout shatters $S jaw, killing $M.",
  "You kick $N in the rump with such force that $E keels over dead.",
  "You kick $N in the belly, mangling several ribs and killing $M instantly.",
  "$N's scales cave in as your mighty kick kills $N.",
  "Your kick rips bark asunder and leaves fly everywhere, killing the $N.",
  "Bits of $N are sent flying as you kick $M to pieces.",
  "You punt $N across the room, $E lands in a heap of broken flesh.",
  "You kick $N in the groin; $E dies screaming an octave higher.",
  "",                          /* GHOST */
  "You knock $N's body, in several pieces, to the ground with your kick.",
  "Your kick splits $N to pieces, rotting flesh flies everywhere.",
  "Your kick topples $N over, killing $M.",
  "Your foot shatters cartilage, sending bits of $N everywhere.",
  "You launch a mighty kick at $N's gills, killing $M.",
  "Your kick at $N sends $M to the grave.",
  "Your fierce kick sends $N flying through the air in a lifeless arc of death."
  ""
};
const char *att_kick_kill_victim[] = {
  "$n crushes you beneath $s foot, killing you.",
  "$n destroys your arm and half your ribs.  You die.",
  "$n neatly splits your groin in two, you collapse and die instantly.",
  "$n splits your head in two, killing you instantly.",
  "$n forces your jaw into the lower part of your brain.",
  "$n kicks you from behind, snapping your spine and killing you.",
  "$n kicks your stomach and you into the great land beyond!!",
  "Your scales are no defense against $n's mighty kick.",
  "$n rips you apart with a massive kick, you die in a flutter of leaves.",
  "You are torn to little pieces as $n splits you with $s kick.",
  "$n's kick sends you flying, you die before you land.",
  "Puny little $n manages to land a killing blow to your groin, OUCH!",
  "",                          /* GHOST */
  "You hit the ground solidly as $n pulverizes you with a massive kick.",
  "$n's kick rips your rotten body into shreds, and your various pieces die.",
  "$n kicks you so hard, you fall over and die.",
  "$n shatters your exoskeleton, you die.",
  "$n kicks you in the gills!  You cannot breath..... you die!.",
  "$n sends you to the grave with a mighty kick.",
  "$n's mighty kick stops your heart cold."
  ""
};
const char *att_kick_kill_room[] = {
  "$n strikes $N in chest, shattering the ribs beneath $M.",
  "$n kicks $N in the side, destroying $S arm and ribs.",
  "$n nails $N in the groin, the pain killing $M.",
  "$n shatters $N's head, reducing $M to a twitching heap!",
  "$n blasts $N in the snout, destroying bones and causing death.",
  "$n kills $N with a massive kick to the rear.",
  "$n sends $N to the grave with a massive blow to the stomach!",
  "$n ignores $N's scales and kills $M with a mighty kick.",
  "$n sends bark and leaves flying as $e splits $N in two.",
  "$n blasts $N to pieces with a ferocious kick.",
  "$n sends $N flying, $E lands with a LOUD THUD, making no other noise.",
  "$N falls to the ground and dies clutching $S crotch due to $n's kick.",
  "",                          /* GHOST */
  "$N falls quickly to the ground as $n kicks $M to death.",
  "$n blasts $N's rotten body into pieces with a powerful kick.",
  "$n kicks $N so hard, $E falls over and dies.",
  "$n blasts $N's exoskeleton to little fragments.",
  "$n kicks $N in the gills, killing $M.",
  "$n sends $N to the grave with a mighty kick.",
  "$n's kick sends $N flying through the air in a lifeless heap."
  ""
};
const char *att_kick_miss_ch[] = {
  "$N steps back, and your kick misses $M.",
  "$N deftly blocks your kick with $S forearm.",
  "$N dodges, and you miss your kick at $S legs.",
  "$N ducks, and your foot flies a mile high.",
  "$N eyes close quickly as your foot flies by $S face.",
  "$N laughs at your feeble attempt to kick $M from behind.",
  "Your kick at $N's belly comes up short.",
  "$N hisses as your kick bounces off $S tough scales.",
  "You kick $N in the side, denting your foot.",
  "Your sloppy kick is easily avoided by $N.",
  "You misjudge $N's height and kick well above $S head.",
  "You stub your toe against $N's shin as you try to kick $M.",
  "Your kick passes through $N!!",      /* Ghost */
  "$N nimbly flitters away from your kick.",
  "$N sidesteps your kick and sneers at you.",
  "Your kick bounces off $N's leathery hide.",
  "Your kick bounces off $N's tough exoskeleton.",
  "$N deflects your kick with a fin.",
  "$N avoids your paltry attempt at a kick.",
  "$N floats deftly out of the way, avoiding your sweeping kick."
  ""
};
const char *att_kick_miss_victim[] = {
  "$n misses you with $s clumsy kick at your chest.",
  "You block $n's feeble kick with your arm.",
  "You dodge $n's feeble leg sweep.",
  "You duck under $n's lame kick.",
  "You step back and grin as $n misses your face with a kick.",
  "$n attempts a feeble kick from behind, which you neatly avoid.",
  "You laugh at $n's feeble attempt to kick you in the stomach.",
  "$n kicks you, but your scales are much too tough for that wimp.",
  "You laugh as $n dents $s foot on your bark.",
  "You easily avoid a sloppy kick from $n.",
  "$n's kick parts your hair but does little else.",
  "$n's light kick to your shin bearly gets your attention.",
  "$n passes through you with $s puny kick.",
  "You nimbly flitter away from $n's kick.",
  "You sneer as you sidestep $n's kick.",
  "$n's kick bounces off your tough hide.",
  "$n tries to kick you, but your too tough.",
  "$n tries to kick you, but you easily deflect $s blow with a fin.",
  "You avoid $n's feeble attempt to kick you.",
  "You float deftly out of the way, avoiding $n's feeble sweep."
  ""
};

const char *att_kick_miss_room[] = {
  "$n misses $N with a clumsy kick.",
  "$N blocks $n's kick with $S arm.",
  "$N easily dodges $n's feeble leg sweep.",
  "$N easily ducks under $n's lame kick.",
  "$N eyes close momentarily as $n's feeble kick to $S face misses.",
  "$n launches a kick at $N's behind, but fails miserably.",
  "$N avoids $n's attempt to kick $M in the stomach.",
  "$n tries to kick $N, but $s foot bounces off of $N's scales.",
  "$n hurts $s foot trying to kick $N.",
  "$N avoids a lame kick launched by $n.",
  "$n misses a kick at $N due to $S small size.",
  "$n misses a kick at $N's groin, stubbing $s toe in the process.",
  "$n's foot goes right through $N!!!!",
  "$N flitters away from $n's kick.",
  "$N sneers at $n while sidestepping $s kick.",
  "$N's tough hide deflects $n's kick.",
  "$n hurts $s foot on $N's tough exterior.",
  "$n tries to kick $N, but is thwarted by a fin.",
  "$N avoids $n's feeble kick.",
  "$N floats deftly out of the way, avoiding $n's sweeping kick."
  ""
};

const char *att_kick_hit_ch[] = {
  "Your kick crashes into $N's chest.",
  "Your kick hits $N in the side.",
  "You hit $N in the thigh with a hefty sweep.",
  "You hit $N in the face, sending $M reeling.",
  "You plant your foot firmly in $N's snout, smashing $M to one side.",
  "You nail $N from behind, sending $M reeling.",
  "You kick $N in the stomach, winding $M.",
  "You find a soft spot in $N's scales and launch a solid kick there.",
  "Your kick hits $N, sending small branches and leaves everywhere.",
  "Your kick contacts with $N, dislodging little pieces of $M.",
  "Your kick hits $N right in the stomach, rendering $M breathless.",
  "You stomp on $N's foot. After all, that's about all you can do to a giant.",
  "",                          /* GHOST */
  "Your kick sends $N reeling through the air.",
  "You kick $N and feel rotten bones crunch from the blow.",
  "You smash $N with a hefty roundhouse kick.",
  "You kick $N, cracking $S exoskeleton.",
  "Your mighty kick rearranges $N's scales.",
  "You leap off the ground and crash into $N with a powerful kick.",
  "Your solid kick sends $N reeling through the air."
  ""
};

const char *att_kick_hit_victim[] = {
  "$n's kick crashes into your chest.",
  "$n's kick hits you in your side.",
  "$n's sweep catches you in the side and you almost stumble.",
  "$n's kick hits you directly in the face.",
  "$n kicks you in the snout, smashing it up against your face.",
  "$n boots you in the rear, ouch!",
  "Your breath rushes from you as $n kicks you in the stomach.",
  "$n finds a soft spot on your scales and kicks you, ouch!",
  "$n kicks you hard, sending leaves flying everywhere!",
  "$n kicks you in the side, dislodging small parts of you.",
  "You suddenly see $n's foot in your chest.",
  "$n lands a kick hard on your foot making you jump around in pain.",
  "",                          /* GHOST */
  "$n kicks you, and you go reeling through the air.",
  "$n kicks you and your bones crumble.",
  "$n hits you in the flank with a hefty roundhouse kick.",
  "$n ruins some of your scales with a well placed kick.",
  "$n leaps off of the grand and crashes into you with $s kick.",
  "$n's solid kick finds flesh as $e sends $N flying through the air."
  ""
};

const char *att_kick_hit_room[] = {
  "$n hits $N with a mighty kick to $S chest.",
  "$n whacks $N in the side with a sound kick.",
  "$n almost sweeps $N off of $S feet with a well placed leg sweep.",
  "$N's eyes roll as $n plants a foot in $S face.",
  "$N's snout is smashed as $n relocates it with $s foot.",
  "$n hits $N with an impressive kick from behind.",
  "$N gasps as $n kicks $M in the stomach.",
  "$n finds a soft spot in $N's scales and launches a solid kick there.",
  "$n kicks $N.  Leaves fly everywhere!!",
  "$n hits $N with a mighty kick, $N loses parts of $Mself.",
  "$n kicks $N in the stomach, rendering $M breathless.",
  "$n kicks $N in the foot, causing $N to hop around in pain.",
  "",                          /* GHOST */
  "$n sends $N reeling through the air with a mighty kick.",
  "$n kicks $N causing parts of $N to cave in!",
  "$n kicks $N in the side with a hefty roundhouse kick.",
  "$n kicks $N, cracking exo-skelelton.",
  "$n kicks $N hard, sending scales flying!",
  "$n leaps up and nails $N with a mighty kick.",
  "$n's fierce kick sends $N reeling through the air in pain."
  ""
};

void kick_messages(P_char ch, P_char victim, bool hit,
                   struct damage_messages *messages)
{
  int      i;

  switch (GET_RACE(victim))
  {
  case RACE_HUMAN:
  case RACE_HUMANOID:
  case RACE_BARBARIAN:
  case RACE_DROW:
  case RACE_GREY:
  case RACE_HALFELF:
  case RACE_HALFORC:
  case RACE_ILLITHID:
  case RACE_THRIKREEN:
  case RACE_GITHYANKI:
  case RACE_MINOTAUR:
  case RACE_SHADE:
  case RACE_REVENANT:
  case RACE_OGRE:
  case RACE_SNOW_OGRE:
  case RACE_TROLL:
  case RACE_GOLEM:
  case RACE_PRIMATE:
  case RACE_ANGEL:
  case RACE_DEMON:
  case RACE_DEVIL:
  case RACE_VAMPIRE:
  case RACE_CENTAUR:
  case RACE_OROG:
  case RACE_GITHZERAI:
  case RACE_RAKSHASA:
  case RACE_WOODELF:
  case RACE_FIRBOLG:
  case RACE_KOBOLD:
  case RACE_KUOTOA:
  case RACE_AGATHINON:
  case RACE_ELADRIN:
    i = number(0, 3);
    break;
  case RACE_HERBIVORE:
  case RACE_CARNIVORE:
  case RACE_ANIMAL:
  case RACE_LYCANTH:
  case RACE_QUADRUPED:
    i = number(4, 6);
    break;
  case RACE_DRAGON:
  case RACE_DRAGONKIN:
  case RACE_REPTILE:
  case RACE_SNAKE:
  case RACE_PWORM:
    i = number(4, 7);
    break;
  case RACE_PLANT:
    i = 8;
    break;
  case RACE_PARASITE:
  case RACE_SLIME:
    i = 9;
    break;
  case RACE_MOUNTAIN:
  case RACE_DUERGAR:
  case RACE_HALFLING:
  case RACE_GNOME:
  case RACE_ORC:
  case RACE_GOBLIN:
  case RACE_FAERIE:
    i = 10;
    break;
  case RACE_GARGOYLE:
  case RACE_GIANT:
  case RACE_E_ELEMENTAL:
  case RACE_I_ELEMENTAL:
  case RACE_EFREET:
  case RACE_CONSTRUCT:
    if (!number(0, 1))
      i = 11;
    else
      i = 15;
    break;
  case RACE_GHOST:
  case RACE_WRAITH:
  case RACE_SPECTRE:
  case RACE_SHADOW:
  case RACE_DEVA:
  case RACE_ASURA:
  case RACE_PHANTOM:
  case RACE_WIGHT:
  case RACE_V_ELEMENTAL:
  case RACE_F_ELEMENTAL:
  case RACE_A_ELEMENTAL:
  case RACE_W_ELEMENTAL:
    i = 12;
    break;
  case RACE_FLYING_ANIMAL:
    i = 13;
    break;
  case RACE_UNDEAD:
  case RACE_PLICH:
  case RACE_PVAMPIRE:
  case RACE_PDKNIGHT:
  case RACE_PSBEAST:
  case RACE_ZOMBIE:
  case RACE_SKELETON:
  case RACE_DRACOLICH:
    i = 14;
    break;
  case RACE_INSECT:
  case RACE_ARACHNID:
  case RACE_DRIDER:
    i = 16;
    break;
  case RACE_AQUATIC_ANIMAL:
    i = 17;
    break;
  case RACE_BEHOLDER:
  case RACE_BEHOLDERKIN:
    i = 18;
    break;
  default:
    i = 19;
  };
  if (hit)
  {
    messages->attacker = (char *) att_kick_hit_ch[i];
    messages->victim = (char *) att_kick_hit_victim[i];
    messages->room = (char *) att_kick_hit_room[i];
    messages->death_attacker = (char *) att_kick_kill_ch[i];
    messages->death_victim = (char *) att_kick_kill_victim[i];
    messages->death_room = (char *) att_kick_kill_room[i];
  }
  else
  {
    messages->attacker = (char *) att_kick_miss_ch[i];
    messages->victim = (char *) att_kick_miss_victim[i];
    messages->room = (char *) att_kick_miss_room[i];
  }
}
