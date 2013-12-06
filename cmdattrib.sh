#!/bin/bash

#Parses the file for attribs
parsefile ( )
{
  declare -i COUNT number
  declare -i COUNT1 number
  declare -i COUNT2 number
  declare -i FOUNDIT number
  FOUNDIT=0

  FILENAME=`grep -l "$FUNCTIONNAME" ./src/*.c`

  cat $FILENAME | 
  while read LINE;
  do
    if [[ $LINE = "$FUNCTIONNAME"* ]]; then
      # Count the damn brackets and do the math
      COUNT1=`grep -o "{" <<<"$LINE" | wc -l`
      COUNT2=`grep -o "}" <<<"$LINE" | wc -l`
      COUNT=$COUNT1-$COUNT2
      # FOUNDIT -> We're inside the function.
      FOUNDIT=1
    elif [[ $FOUNDIT = 1 ]]; then
      # Hunt for the GET_C_...( crap.
      grep -o "GET_C_...(.." <<<"$LINE" >> command_attributes.txt
      # Count the damn brackets and do the math
      COUNT1=`grep -o "{" <<<"$LINE" | wc -l`
      COUNT2=`grep -o "}" <<<"$LINE" | wc -l`
      COUNT+=$COUNT1-$COUNT2

      # if at end of function
      if [[ $COUNT = 0 ]]; then
        break
      fi
    fi
  done
  echo "~" >> command_attributes.txt
}

#remove the old command_att. file and recreate it.
rm -f command_attributes.txt
touch command_attributes.txt

echo "apply poison" >> command_attributes.txt
FUNCTIONNAME="void do_apply_poison"
parsefile

echo "appraise" >> command_attributes.txt
FUNCTIONNAME="void do_appraise"
parsefile

echo "armlock" >> command_attributes.txt
FUNCTIONNAME="void armlock_check"
parsefile

echo "awareness" >> command_attributes.txt
FUNCTIONNAME="void do_awareness"
parsefile

echo "backstab" >> command_attributes.txt
FUNCTIONNAME="bool backstab"
parsefile

echo "barrage" >> command_attributes.txt
FUNCTIONNAME="void event_barrage"
parsefile

echo "bash" >> command_attributes.txt
FUNCTIONNAME="void bash"
parsefile

echo "battle orders" >> command_attributes.txt
FUNCTIONNAME="void battle_orders"
parsefile

echo "bearhug" >> command_attributes.txt
FUNCTIONNAME="void event_bearhug"
parsefile

echo "berserk" >> command_attributes.txt
FUNCTIONNAME="void do_berserk"
parsefile

echo "blood scent" >> command_attributes.txt
FUNCTIONNAME="void do_blood_scent"
parsefile

echo "bodyslam" >> command_attributes.txt
FUNCTIONNAME="void bodyslam"
parsefile

echo "branch" >> command_attributes.txt
FUNCTIONNAME="void branch"
parsefile

echo "buck" >> command_attributes.txt
FUNCTIONNAME="void buck"
parsefile

echo "capture" >> command_attributes.txt
FUNCTIONNAME="void do_capture"
parsefile

echo "carve" >> command_attributes.txt
FUNCTIONNAME="void do_carve"
parsefile

echo "cast" >> command_attributes.txt
FUNCTIONNAME="void do_cast"
parsefile

echo "chi" >> command_attributes.txt
FUNCTIONNAME="void do_chi"
parsefile

echo "combination attack" >> command_attributes.txt
FUNCTIONNAME="void event_combination"
parsefile

echo "chant" >> command_attributes.txt
FUNCTIONNAME="void do_chant"
parsefile

echo "charge" >> command_attributes.txt
FUNCTIONNAME="void do_charge"
parsefile

echo "circle" >> command_attributes.txt
FUNCTIONNAME="bool circle"
parsefile

echo "conjure" >> command_attributes.txt
FUNCTIONNAME="void do_conjure"
parsefile

echo "craft" >> command_attributes.txt
FUNCTIONNAME="void do_craft"
parsefile

echo "dirttoss" >> command_attributes.txt
FUNCTIONNAME="void do_dirttoss"
parsefile

echo "disarm" >> command_attributes.txt
FUNCTIONNAME="void do_disarm"
parsefile

echo "disguise" >> command_attributes.txt
FUNCTIONNAME="void do_disguise"
parsefile

echo "drag" >> command_attributes.txt
FUNCTIONNAME="void do_drag"
parsefile

echo "dragon punch" >> command_attributes.txt
FUNCTIONNAME="void do_dragon_punch"
parsefile

echo "enhance" >> command_attributes.txt
FUNCTIONNAME="void do_enhance"
parsefile

echo "feign death" >> command_attributes.txt
FUNCTIONNAME="void do_feign_death"
parsefile

echo "fire" >> command_attributes.txt
FUNCTIONNAME="void do_fire"
parsefile

echo "flank" >> command_attributes.txt
FUNCTIONNAME="bool flank"
parsefile

echo "flee" >> command_attributes.txt
FUNCTIONNAME="void do_flee"
parsefile

echo "flurry of blows" >> command_attributes.txt
FUNCTIONNAME="void do_flurry_of_blows"
parsefile

echo "forage" >> command_attributes.txt
FUNCTIONNAME="void do_forage"
parsefile

echo "garrote" >> command_attributes.txt
FUNCTIONNAME="void do_garrote"
parsefile

echo "gaze" >> command_attributes.txt
FUNCTIONNAME="void gaze"
parsefile

echo "groundslam" >> command_attributes.txt
FUNCTIONNAME="void do_groundslam"
parsefile

echo "hamstring" >> command_attributes.txt
FUNCTIONNAME="void do_hamstring"
parsefile

echo "headbutt" >> command_attributes.txt
FUNCTIONNAME="void do_headbutt"
parsefile

echo "headlock" >> command_attributes.txt
FUNCTIONNAME="void event_headlock"
parsefile

echo "guard" >> command_attributes.txt
FUNCTIONNAME="P_char guard_check"
parsefile

echo "hide" >> command_attributes.txt
FUNCTIONNAME="void do_hide"
parsefile

echo "hit" >> command_attributes.txt
FUNCTIONNAME="void do_hit"
parsefile

echo "hitall" >> command_attributes.txt
FUNCTIONNAME="void do_hitall"
parsefile

echo "holy smite" >> command_attributes.txt
FUNCTIONNAME="void do_holy_smite"
parsefile

echo "infuriate" >> command_attributes.txt
FUNCTIONNAME="void do_infuriate"
parsefile

echo "jin touch" >> command_attributes.txt
FUNCTIONNAME="void chant_jin_touch"
parsefile

echo "kick" >> command_attributes.txt
FUNCTIONNAME="void kick"
parsefile

echo "lay hands" >> command_attributes.txt
FUNCTIONNAME="void do_layhand"
parsefile

echo "leglock" >> command_attributes.txt
FUNCTIONNAME="void event_leglock"
parsefile

echo "listen" >> command_attributes.txt
FUNCTIONNAME="void do_listen"
parsefile

echo "maul" >> command_attributes.txt
FUNCTIONNAME="void maul"
parsefile

echo "mine" >> command_attributes.txt
FUNCTIONNAME="void event_mine_check"
parsefile

echo "mix" >> command_attributes.txt
FUNCTIONNAME="void do_mix"
parsefile

echo "mug" >> command_attributes.txt
FUNCTIONNAME="void do_mug"
parsefile

echo "ogreroar" >> command_attributes.txt
FUNCTIONNAME="void do_ogre_roar"
parsefile

echo "parlay" >> command_attributes.txt
FUNCTIONNAME="void parlay"
parsefile

echo "play" >> command_attributes.txt
FUNCTIONNAME="int bard_calc_chance"
parsefile

echo "quaff" >> command_attributes.txt
FUNCTIONNAME="void do_quaff"
parsefile

echo "quivering palm" >> command_attributes.txt
FUNCTIONNAME="void chant_quivering_palm"
parsefile

echo "rage" >> command_attributes.txt
FUNCTIONNAME="void do_rage"
parsefile

echo "rampage" >> command_attributes.txt
FUNCTIONNAME="void do_rampage"
parsefile

echo "recite" >> command_attributes.txt
FUNCTIONNAME="void do_recite"
parsefile

echo "rescue" >> command_attributes.txt
FUNCTIONNAME="void rescue"
parsefile

echo "restrain" >> command_attributes.txt
FUNCTIONNAME="void restrain"
parsefile

echo "rearkick" >> command_attributes.txt
FUNCTIONNAME="void do_rearkick"
parsefile

echo "retreat" >> command_attributes.txt
FUNCTIONNAME="void do_retreat"
parsefile

echo "riff" >> command_attributes.txt
FUNCTIONNAME="void do_riff"
parsefile

echo "roundkick" >> command_attributes.txt
FUNCTIONNAME="int chance_roundkick"
parsefile

echo "rush" >> command_attributes.txt
FUNCTIONNAME="void rush"
parsefile

echo "search" >> command_attributes.txt
FUNCTIONNAME="void do_search"
parsefile

echo "shadow" >> command_attributes.txt
FUNCTIONNAME="void MoveShadower"
parsefile

echo "shadowstep" >> command_attributes.txt
FUNCTIONNAME="void do_shadowstep"
parsefile

echo "shieldpunch" >> command_attributes.txt
FUNCTIONNAME="void shieldpunch"
parsefile

echo "shriek" >> command_attributes.txt
FUNCTIONNAME="void do_shriek"
parsefile

echo "smith" >> command_attributes.txt
FUNCTIONNAME="void do_smith"
parsefile

echo "sneak" >> command_attributes.txt
FUNCTIONNAME="void do_sneak"
parsefile

echo "sneaky strike" >> command_attributes.txt
FUNCTIONNAME="void event_sneaky_strike"
parsefile

echo "springleap" >> command_attributes.txt
FUNCTIONNAME="void do_springleap"
parsefile

echo "stampede" >> command_attributes.txt
FUNCTIONNAME="void do_stampede"
parsefile

echo "steal" >> command_attributes.txt
FUNCTIONNAME="void do_steal"
parsefile

echo "newsteal" >> command_attributes.txt
FUNCTIONNAME="void do_newsteal"
parsefile

echo "subterfuge" >> command_attributes.txt
FUNCTIONNAME="void do_subterfuge"
parsefile

echo "sweeping thrust" >> command_attributes.txt
FUNCTIONNAME="void do_sweeping_thrust"
parsefile

echo "tackle" >> command_attributes.txt
FUNCTIONNAME="void do_tackle"
parsefile

echo "takedowns" >> command_attributes.txt
FUNCTIONNAME="int takedown_check"
parsefile

echo "throat crush" >> command_attributes.txt
FUNCTIONNAME="void do_throat_crush"
parsefile

echo "throw" >> command_attributes.txt
FUNCTIONNAME="void do_throw"
parsefile

echo "throwpotion" >> command_attributes.txt
FUNCTIONNAME="void do_throw_potion"
parsefile

echo "trample" >> command_attributes.txt
FUNCTIONNAME="void do_trample"
parsefile

echo "trap" >> command_attributes.txt
FUNCTIONNAME="void do_trap"
parsefile

echo "trip" >> command_attributes.txt
FUNCTIONNAME="void do_trip"
parsefile

echo "use" >> command_attributes.txt
FUNCTIONNAME="void do_use"
parsefile

echo "war cry" >> command_attributes.txt
FUNCTIONNAME="void do_war_cry"
parsefile

echo "webwrap" >> command_attributes.txt
FUNCTIONNAME="void webwrap"
parsefile

echo "whirlwind" >> command_attributes.txt
FUNCTIONNAME="void do_whirlwind"
parsefile

echo "will" >> command_attributes.txt
FUNCTIONNAME="void do_will"
parsefile

