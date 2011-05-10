#ifndef _CTF_H_
#define _CTF_H_

#include "structs.h"
#include "boon.h"

#define CTF_NO_FLAG	0
#define CTF_PRIMARY	1
#define CTF_SECONDARY	2
#define CTF_BOON	3

#define CTF_MAX		3

#define CTF_ACT_CAPTURE	1
#define CTF_ACT_RETURN	2
#define CTF_ACT_DROP	3

// Quick references to good and evil flag ID's in the struct
#define CTF_FLAG_GOOD	1
#define CTF_FLAG_EVIL	2

// database references
#define CTF_TYPE_NONE		0
#define CTF_TYPE_CAPTURE	1
#define CTF_TYPE_RECLAIM	2
// can imp these later if needed, but would create a lot of db entries
#define CTF_TYPE_TAKE		3
#define CTF_TYPE_DROP		4

struct ctfData {
  int id;	// flag id
  int type;	// Primary flag, or secondary(zone) flags
  int racewar;	// Good/Evil for primary flgs, None for secondary flags
  int flag;	// flag rnum
  int room;	// real room to load object
  P_obj obj;	// link to flag
};

int init_ctf();
int load_ctf();
int ctf_flag_proc(P_obj, P_char, int, char*); 
void ctf_notify(const char*, int);
P_obj get_ctf_flag(int);
bool check_ctf_capture(P_char, P_obj);
void capture_flag(P_char, P_obj, int);
int add_ctf_entry(P_char, int, int);
void do_ctf(P_char, char*, int);
void reset_ctf(P_char);
bool drop_ctf_flag(P_char);
void ctf_populate_boons();
int ctf_use_boon(BoonData*);
int ctf_reload_flag(int);
P_char get_flag_carrier(int);
void ctf_delete_flag(int);
int ctf_get_random_room(int);
#endif
