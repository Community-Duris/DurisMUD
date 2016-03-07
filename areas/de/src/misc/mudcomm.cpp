//
//  File: mudcomm.cpp    originally part of durisEdit
//
//  Usage: big functions for retrieving command strings for all mud
//         commands
//

/*
 * Copyright (c) 1995-2007, Michael Glosenger
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * The name of Michael Glosenger may not be used to endorse or promote 
 *       products derived from this software without specific prior written 
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY MICHAEL GLOSENGER ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO 
 * EVENT SHALL MICHAEL GLOSENGER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; 
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "types.h"
#include "fh.h"
#include "mudcomm.h"

const mudCommand g_mudCommands[] =
{ { "", false },  // MUDCOMM_LOWEST is 1
  { "north", false },
  { "east", false },
  { "south", false },
  { "west", false },
  { "up", false },
  { "down", false },
  { "enter", false },
  { "exits", true },
  { "kiss", false },
  { "get", false },
  { "drink", false },
  { "eat", false },
  { "wear", false },
  { "wield", false },
  { "look", false },
  { "score", true },
  { "say", false },
  { "shout", false },
  { "tell", false },
  { "inventory", true },
  { "qui", true },
  { "bounce", false },
  { "smile", false },
  { "dance", false },
  { "kill", false },
  { "cackle", false },
  { "laugh", false },
  { "giggle", false },
  { "shake", false },
  { "puke", false },
  { "growl", false },
  { "scream", false },
  { "insult", false },
  { "comfort", false },
  { "nod", false },
  { "sigh", false },
  { "sulk", false },
  { "help", true },
  { "who", true },
  { "emote", true },
  { "echo", true },
  { "stand", false },
  { "sit", false },
  { "rest", false },
  { "sleep", false },
  { "wake", false },
  { "force", true },
  { "transfer", true },
  { "hug", false },
  { "snuggle", false },
  { "cuddle", false },
  { "nuzzle", false },
  { "cry", false },
  { "news", true },
  { "equipment", true },
  { "buy", true },
  { "sell", true },
  { "value", true },
  { "list", true },
  { "drop", false },
  { "goto", true },
  { "weather", false },
  { "read", false },
  { "pour", false },
  { "grab", false },
  { "remove", true },
  { "put", false },
  { "shutdow", true },
  { "save", true },
  { "hit", false },
  { "string", true },
  { "give", false },
  { "quit", true },
  { "stat", true },
  { "setskill", true },
  { "time", true },
  { "load", true },
  { "purge", true },
  { "shutdown", true },
  { "idea", true },
  { "typo", true },
  { "bug", true },
  { "whisper", false },
  { "cast", true },
  { "at", true },
  { "ask", false },
  { "order", false },
  { "sip", false },
  { "taste", false },
  { "snoop", true },
  { "follow", false },
  { "rent", true },
  { "offer", false },
  { "poke", false },
  { "advance", true },
  { "accuse", false },
  { "grin", false },
  { "bow", false },
  { "open", false },
  { "close", false },
  { "lock", false },
  { "unlock", false },
  { "leave", false },
  { "applaud", false },
  { "blush", false },
  { "burp", false },
  { "chuckle", false },
  { "clap", false },
  { "cough", false },
  { "curtsey", false },
  { "fart", false },
  { "flip", false },
  { "fondle", false },
  { "frown", false },
  { "gasp", false },
  { "glare", false },
  { "groan", false },
  { "grope", false },
  { "hiccup", false },
  { "lick", false },
  { "love", false },
  { "moan", false },
  { "nibble", false },
  { "pout", false },
  { "purr", false },
  { "ruffle", false },
  { "shiver", false },
  { "shrug", false },
  { "sing", false },
  { "slap", false },
  { "smirk", false },
  { "snap", false },
  { "sneeze", false },
  { "snicker", false },
  { "sniff", false },
  { "snore", false },
  { "spit", false },
  { "squeeze", false },
  { "stare", false },
  { "strut", false },
  { "thank", false },
  { "twiddle", false },
  { "wave", false },
  { "whistle", false },
  { "wiggle", false },
  { "wink", false },
  { "yawn", false },
  { "snowball", false },
  { "write", true },
  { "hold", false },
  { "flee", true },
  { "sneak", true },
  { "hide", true },
  { "backstab", false },
  { "pick", false },
  { "steal", false },
  { "bash", false },
  { "rescue", true },
  { "kick", false },
  { "french", false },
  { "comb", false },
  { "massage", false },
  { "tickle", false },
  { "practice", true },
  { "pat", false },
  { "examine", false },
  { "take", false },
  { "info", true },
  { "spells", false },
  { "practise", true },
  { "curse", false },
  { "use", false },
  { "where", true },
  { "levels", true },
  { "reroll", true },
  { "pray", false },
  { ":", true },
  { "beg", false },
  { "bleed", false },
  { "cringe", false },
  { "dream", false },
  { "fume", false },
  { "grovel", false },
  { "hop", false },
  { "nudge", false },
  { "peer", false },
  { "point", false },
  { "ponder", false },
  { "punch", false },
  { "snarl", false },
  { "spank", false },
  { "steam", false },
  { "tackle", false },
  { "taunt", false },
  { "think", false },
  { "whine", false },
  { "worship", false },
  { "yodel", false },
  { "toggle", true },
  { "wizlist", true },
  { "consider", true },
  { "group", true },
  { "restore", true },
  { "return", true },
  { "switch", true },
  { "quaff", false },
  { "recite", false },
  { "users", true },
  { "pose", true },
  { "silence", true },
  { "wizhelp", true },
  { "credits", true },
  { "disband", true },
  { "vis", true },
  { "pflags", true },
  { "poofin", true },
  { "wizmsg", true },
  { "display", true },
  { "echoa", true },
  { "demote", true },
  { "poofout", true },
  { "wimpy", true },
  { "balance", true },
  { "wizlock", true },
  { "deposit", true },
  { "withdraw", true },
  { "ignore", true },
  { "setattr", true },
  { "title", true },
  { "aggr", true },
  { "gsay", true },
  { "consent", true },
  { "setbit", true },
  { "hitall", true },
  { "trap", true },
  { "murder", true },
  { "#237", true },
  { "auction", true },
  { "channel", true },
  { "fill", false },
  { "gossip", true },
  { "nokill", true },
  { "page", true },
  { "commands", true },
  { "attributes", true },
  { "rules", true },
  { "track", true },
  { "listen", false },
  { "disarm", true },
  { "parry", true },
  { "delete", true },
  { "ban", true },
  { "allow", true },
  { "password", true },
  { "description", true },
  { "bribe", false },
  { "bonk", false },
  { "calm", false },
  { "rub", false },
  { "censor", false },
  { "choke", false },
  { "drool", false },
  { "flex", false },
  { "jump", false },
  { "lean", false },
  { "moon", false },
  { "ogle", false },
  { "pant", false },
  { "pinch", false },
  { "push", false },
  { "scare", false },
  { "scold", false },
  { "seduce", false },
  { "shove", false },
  { "shudder", false },
  { "shush", false },
  { "slobber", false },
  { "smell", false },
  { "sneer", false },
  { "spin", false },
  { "squirm", false },
  { "stomp", false },
  { "strangle", false },
  { "stretch", false },
  { "tap", false },
  { "tease", false },
  { "tiptoe", false },
  { "tweak", false },
  { "twirl", false },
  { "undress", false },
  { "whimper", false },
  { "exchange", false },
  { "release", false },
  { "search", false },
  { "fly", true },
  { "levitate", true },
  { "secret", true },
  { "lookup", true },
  { "report", true },
  { "split", true },
  { "world", true },
  { "junk", true },
  { "petition", true },
  { "do", true },
  { "'", true },
  { "caress", false },
  { "bury", true },
  { "donate", true },
  { "#309", true },
  { "#310", true },
  { "#311", true },
  { "#312", true },
  { "#313", true },
  { "yell", true },
  { "#315", true },
  { "#316", true },
  { "#317", true },
  { "#318", true },
  { "#319", true },
  { "touch", false },
  { "scratch", false },
  { "wince", false },
  { "toss", false },
  { "flame", false },
  { "arch", false },
  { "amaze", false },
  { "bathe", false },
  { "embrace", false },
  { "brb", false },
  { "ack", false },
  { "cheer", false },
  { "snort", false },
  { "eyebrow", false },
  { "bang", false },
  { "pillow", false },
  { "nap", false },
  { "nose", false },
  { "raise", false },
  { "hand", false },
  { "pull", false },
  { "tug", false },
  { "wet", false },
  { "mosh", false },
  { "wait", false },
  { "hi5", false },
  { "envy", false },
  { "flirt", false },
  { "bark", false },
  { "whap", false },
  { "roll", false },
  { "blink", false },
  { "duh", false },
  { "gag", false },
  { "grumble", false },
  { "dropkick", false },
  { "whatever", false },
  { "fool", false },
  { "noogie", false },
  { "melt", false },
  { "smoke", false },
  { "wheeze", false },
  { "bird", false },
  { "boggle", false },
  { "hiss", false },
  { "bite", false },  
    0 };

//
// getCommandStrn
//

const char *getCommandStrn(const uint commandValue, const bool verboseDisplay)
{
  if ((commandValue > MUDCOMM_HIGHEST) || (commandValue < MUDCOMM_LOWEST))
    return "command type out of range";

  if (!g_mudCommands[commandValue].verboseOnly || verboseDisplay)
    return g_mudCommands[commandValue].commandStrn;
  else
    return "";
}


//
// displayCommandList
//

void displayCommandList(const bool displayAll)
{
  size_t numbLines = 1;


  _outtext("\n\n");

  for (uint i = MUDCOMM_LOWEST; i <= MUDCOMM_HIGHEST; i++)
  {
    const char *cmdStrn = getCommandStrn(i, displayAll);

    if (cmdStrn[0])
    {
      char strn[256];
    
      sprintf(strn, "  %u - %s\n", i, cmdStrn);

      if (checkPause(strn, numbLines))
        return;
    }
  }

  _outtext("\n\n");
}


//
// displayCommandList
//

void displayCommandList(const char *searchStrn)
{
  uint i;
  size_t numbLines = 1;
  char strn[256];


  _outtext("\n\n");

  for (i = MUDCOMM_LOWEST; i <= MUDCOMM_HIGHEST; i++)
  {
    if (strstrnocase(getCommandStrn(i, true), searchStrn))
    {
      sprintf(strn, "  %u - %s\n", i, getCommandStrn(i, true));

      if (checkPause(strn, numbLines))
        return;
    }
  }

  _outtext("\n\n");
}
