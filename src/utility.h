/*
 *  utility.h
 *  Duris
 *
 *  Created by Torgal on 1/29/10.
 *
 */

#ifndef _UTILITY_H_
#define _UTILITY_H_

#include "structs.h"

int GET_LVL_FOR_SKILL(P_char ch, int skill);
bool is_ansi_char(char collor_char);

void connect_rooms(int, int, int, int);
void connect_rooms(int, int, int);

void disconnect_exit(int v1, int dir);
void disconnect_rooms(int v1, int v2);

P_char get_char_online(char *name);

void logit(const char *, const char *,...);

int cmd_from_dir(int dir);
int direction_tag(P_char ch);

const char *condition_str(P_char ch);

string pad_ansi(const char *str, int length, bool trim_to_length = FALSE);
P_char get_player_from_name(char *name);
int get_player_pid_from_name(char *name);
char *get_player_name_from_pid(int pid);

bool sub_string( const char *, const char * );
bool sub_string_set( const char *, const char ** );

#endif // _UTILITY_H_
