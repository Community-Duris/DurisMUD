/*
 * copyover.c - true copyover (hotboot) support
 * keeps player connections alive across process replacement
 */

#include "prototypes.h"
#include "structs.h"
#include "comm.h"
#include "db.h"
#include "utils.h"
#include "copyover.h"
#include <errno.h>
#include <fcntl.h>
#include <gnutls/gnutls.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include "defines.h"
#include "files.h"
#include "gmcp.h"
#include "mm.h"
#include "ships/ships.h"
#include "ttype.h"
#include "websocket.h"

extern const int         top_of_world;
extern int               top_of_zone_table;
extern struct zone_data *zone_table;
extern P_room            world;
extern P_desc            descriptor_list;
extern P_char            character_list;
extern P_index           mob_index;
extern P_index           obj_index;
extern P_obj             object_list;
extern int               RUNNING_PORT;
extern struct mm_ds     *dead_mob_pool;
extern struct mm_ds     *dead_pconly_pool;
extern struct mm_ds     *dead_desc_pool;
extern int               _copyover;
extern int               used_descs;

extern void nonblock(int s);

extern int  restoreCharOnly(P_char ch, char *name);
extern void clear_char(P_char ch);
extern void setCharPhysTypeInfo(P_char ch);

static int copyover_in_progress = 0;

int is_copyover_boot(void) { return copyover_in_progress; }

void copyover_prepare_socket(int fd)
{
	int flags = fcntl(fd, F_GETFD);
	if (flags >= 0)
	{
		fcntl(fd, F_SETFD, flags & ~FD_CLOEXEC);
	}
}

static int write_desc_entry(FILE *fp, P_desc d)
{
	struct copyover_desc entry;
	P_char               ch = d->character;
	struct follow_type  *f;

	memset(&entry, 0, sizeof(entry));
	entry.fd = d->descriptor;

	if (ch && GET_NAME(ch))
	{
		strncpy(entry.player_name, GET_NAME(ch), sizeof(entry.player_name) - 1);
		entry.player_name[sizeof(entry.player_name) - 1] = '\0';
		entry.room                                       = ch->in_room;

		// save combat state
		if (ch->specials.fighting)
		{
			P_char target = ch->specials.fighting;
			if (IS_NPC(target))
			{
				entry.fighting_type = 1;
				entry.fighting_id   = GET_IDNUM(target);
			}
			else
			{
				entry.fighting_type = 2;
				if (GET_NAME(target))
				{
					strncpy(entry.fighting_name, GET_NAME(target), sizeof(entry.fighting_name) - 1);
					entry.fighting_name[sizeof(entry.fighting_name) - 1] = '\0';
				}
			}
		}

		// save pet/follower vnums and hp
		entry.num_pets = 0;
		for (f = ch->followers; f && entry.num_pets < 10; f = f->next)
		{
			if (IS_NPC(f->follower) && f->follower->in_room == ch->in_room)
			{
				int idx                = entry.num_pets++;
				entry.pet_vnums[idx]   = mob_index[GET_RNUM(f->follower)].virtual_number;
				entry.pet_hit[idx]     = GET_HIT(f->follower);
				entry.pet_max_hit[idx] = GET_MAX_HIT(f->follower);
			}
		}
	}
	strncpy(entry.host, d->host, sizeof(entry.host) - 1);
	entry.host[sizeof(entry.host) - 1] = '\0';
	strncpy(entry.host2, d->host2, sizeof(entry.host2) - 1);
	entry.host2[sizeof(entry.host2) - 1] = '\0';
	entry.term_type                      = d->term_type;
	entry.gmcp_enabled                   = d->gmcp_enabled;
	entry.out_compress                   = d->out_compress;
	entry.mtts_flags                     = d->mtts_flags;
	entry.charset_detected               = 0; // removed
	strncpy(entry.ttype_client, d->client_name, sizeof(entry.ttype_client) - 1);
	entry.ttype_client[sizeof(entry.ttype_client) - 1] = '\0';
	entry.ttype_terminal[0] = '\0'; // removed

	return fwrite(&entry, sizeof(entry), 1, fp) == 1;
}

static int write_mob_entry(FILE *fp, P_char mob)
{
	struct copyover_mob entry;

	memset(&entry, 0, sizeof(entry));
	entry.vnum         = mob_index[GET_RNUM(mob)].virtual_number;
	entry.idnum        = GET_IDNUM(mob);
	entry.room         = world[mob->in_room].number; // save vnum not rnum
	entry.hit          = GET_HIT(mob);
	entry.max_hit      = GET_MAX_HIT(mob);
	entry.mana         = GET_MANA(mob);
	entry.max_mana     = GET_MAX_MANA(mob);
	entry.vitality     = GET_VITALITY(mob);
	entry.max_vitality = GET_MAX_VITALITY(mob);
	entry.position     = GET_POS(mob);

	if (mob->specials.fighting)
	{
		P_char target = mob->specials.fighting;
		if (!target)
		{
			// stale pointer, skip
		}
		else if (IS_NPC(target))
		{
			entry.fighting_type = 2;
			entry.fighting_id   = GET_IDNUM(target);
		}
		else
		{
			entry.fighting_type = 1;
			if (GET_NAME(target))
			{
				strncpy(entry.fighting_name, GET_NAME(target), sizeof(entry.fighting_name) - 1);
				entry.fighting_name[sizeof(entry.fighting_name) - 1] = '\0';
			}
		}
	}

	struct affected_type *af;
	entry.num_affects = 0;
	for (af = mob->affected; af; af = af->next)
	{
		entry.num_affects++;
	}

	// save equipment
	for (int w = 0; w < MAX_WEAR; w++)
	{
		if (mob->equipment[w])
			entry.equipment_vnums[w] = OBJ_VNUM(mob->equipment[w]);
		else
			entry.equipment_vnums[w] = -1;
	}

	// count carried items
	entry.num_carrying = 0;
	for (P_obj obj = mob->carrying; obj; obj = obj->next_content)
		entry.num_carrying++;

	entry.gold = GET_GOLD(mob);

	return fwrite(&entry, sizeof(entry), 1, fp) == 1;
}

static int write_mob_affects(FILE *fp, P_char mob)
{
	struct copyover_affect entry;
	struct affected_type  *af;

	for (af = mob->affected; af; af = af->next)
	{
		memset(&entry, 0, sizeof(entry));
		entry.type                   = af->type;
		entry.wear_off_message_index = af->wear_off_message_index;
		entry.duration               = af->duration;
		entry.flags                  = af->flags;
		entry.modifier               = af->modifier;
		entry.location               = af->location;
		entry.loc2                   = af->loc2;
		entry.level                  = af->level;
		entry.bitvector              = af->bitvector;
		entry.bitvector2             = af->bitvector2;
		entry.bitvector3             = af->bitvector3;
		entry.bitvector4             = af->bitvector4;
		entry.bitvector5             = af->bitvector5;

		if (fwrite(&entry, sizeof(entry), 1, fp) != 1)
		{
			return 0;
		}
	}
	return 1;
}

static int write_mob_inventory(FILE *fp, P_char mob)
{
	copyover_carried_item entry;
	P_obj                 obj;

	for (obj = mob->carrying; obj; obj = obj->next_content)
	{
		entry.vnum = OBJ_VNUM(obj);
		if (fwrite(&entry, sizeof(entry), 1, fp) != 1)
		{
			return 0;
		}
	}
	return 1;
}

static int write_room_door(FILE *fp, int room_rnum, int dir)
{
	struct copyover_room entry;

	memset(&entry, 0, sizeof(entry));
	entry.vnum  = world[room_rnum].number;
	entry.dir   = dir;
	entry.state = world[room_rnum].dir_option[dir]->exit_info;

	return fwrite(&entry, sizeof(entry), 1, fp) == 1;
}

static int write_obj_entry(FILE *fp, P_obj obj)
{
	struct copyover_obj         entry;
	P_obj                       content;
	struct copyover_obj_content cont_entry;

	memset(&entry, 0, sizeof(entry));
	entry.vnum = OBJ_VNUM(obj);
	entry.room = world[obj->loc.room].number; // save vnum not rnum
	entry.type = obj->type;
	memcpy(entry.value, obj->value, sizeof(entry.value));
	memcpy(entry.timer, obj->timer, sizeof(entry.timer));

	if (obj->name)
	{
		strncpy(entry.name, obj->name, sizeof(entry.name) - 1);
		entry.name[sizeof(entry.name) - 1] = '\0';
	}
	if (obj->short_description)
	{
		strncpy(entry.short_desc, obj->short_description, sizeof(entry.short_desc) - 1);
		entry.short_desc[sizeof(entry.short_desc) - 1] = '\0';
	}
	if (obj->description)
	{
		strncpy(entry.description, obj->description, sizeof(entry.description) - 1);
		entry.description[sizeof(entry.description) - 1] = '\0';
	}

	// count contents
	entry.num_contents = 0;
	for (content = obj->contains; content; content = content->next_content)
	{
		entry.num_contents++;
	}

	if (fwrite(&entry, sizeof(entry), 1, fp) != 1)
		return 0;

	// write contents
	for (content = obj->contains; content; content = content->next_content)
	{
		cont_entry.vnum = OBJ_VNUM(content);
		if (fwrite(&cont_entry, sizeof(cont_entry), 1, fp) != 1)
			return 0;
	}

	return 1;
}

// raw write to socket fd
static void raw_write_to_fd(int fd, const char *msg)
{
	if (fd < 0)
		return;
	write(fd, msg, strlen(msg));
}

// notify websocket users before disconnect
static void notify_ws_copyover(P_desc d)
{
	if (!d->websocket || d->descriptor < 0)
		return;

	// send json message so web client knows whats happening
	const char *msg = "{\"type\":\"system\",\"data\":{\"status\":\"copyover\",\"message\":\"Server updating, please wait...\"}}";

	// wrap in websocket frame
	websocket_send_text(d, msg);
}

// notify ssl users before disconnect
static void notify_ssl_copyover(P_desc d)
{
	if (!d->sslses || d->descriptor < 0)
		return;

	const char *msg = "\r\n*** Copyover in progress. Reconnecting... ***\r\n";
	// use raw write since we're about to close
	gnutls_record_send(d->sslses, msg, strlen(msg));
}

// count how many of each thing we need to save
static void count_copyover_items(int *num_descs, int *num_mobs, int *num_objs, int *num_rooms)
{
	P_desc d;
	P_char ch;
	P_obj  obj;
	int    room, dir;

	*num_descs = 0;
	*num_mobs  = 0;
	*num_objs  = 0;
	*num_rooms = 0;

	// count valid descriptors (telnet only, playing state)
	for (d = descriptor_list; d; d = d->next)
	{
		if (d->descriptor > 0 && d->connected == CON_PLAYING && d->character && !d->websocket && !d->sslses)
		{
			(*num_descs)++;
		}
	}

	// count living mobs (skip pc pets - saved per-descriptor)
	for (ch = character_list; ch; ch = ch->next)
	{
		if (IS_NPC(ch) && ch->in_room >= 0 && !IS_PC_PET(ch))
		{
			(*num_mobs)++;
		}
	}

	// count objects on ground, but skip ship stuff - already loaded
	// also skip objects in ship rooms (dynamic vnums 60000-64999)
	for (obj = object_list; obj; obj = obj->next)
	{
		if (OBJ_ROOM(obj))
		{
			int vnum = OBJ_VNUM(obj);
			if (vnum == VOBJ_PANEL || vnum == VOBJ_ALL_SHIPS || vnum == VOBJ_CARGO_CRATE)
				continue;
			if (IS_SHIP_ROOM(obj->loc.room))
				continue;
			(*num_objs)++;
		}
	}

	// count doors
	for (room = 0; room <= top_of_world; room++)
	{
		for (dir = 0; dir < NUM_EXITS; dir++)
		{
			if (world[room].dir_option[dir] && IS_SET(world[room].dir_option[dir]->exit_info, EX_ISDOOR))
			{
				(*num_rooms)++;
			}
		}
	}
}

void copyover_save(int mother_desc, int mother_desc_ssl, int ws_desc)
{
	FILE                  *fp;
	struct copyover_header header;
	P_desc                 d, d_next;
	P_char                 ch;
	P_obj                  obj;
	int                    room, dir;
	int                    num_descs, num_mobs, num_objs, num_rooms;
	char                   exec_buf[256];

	logit(LOG_STATUS, "copyover: saving world state...");
	logit(LOG_STATUS, "copyover: world=%p top_of_world=%d", (void *)world, top_of_world);

	// first pass - save all players, disconnect websocket/ssl
	for (d = descriptor_list; d; d = d_next)
	{
		d_next = d->next;

		logit(LOG_STATUS, "copyover: desc fd=%d state=%d char=%p ws=%d ssl=%d", d->descriptor, d->connected, (void *)d->character, d->websocket, d->sslses ? 1 : 0);

		if (d->descriptor < 0 || d->connected != CON_PLAYING || !d->character)
		{
			continue;
		}

		logit(LOG_STATUS, "copyover: saving %s with RENT_CRASH", GET_NAME(d->character));
		do_save_silent(d->character, RENT_CRASH);

		if (d->websocket)
		{
			logit(LOG_STATUS, "copyover: %s is websocket, disconnecting", GET_NAME(d->character));
			notify_ws_copyover(d);
			close(d->descriptor);
			d->descriptor = -1;
		}
		else if (d->sslses)
		{
			logit(LOG_STATUS, "copyover: %s is ssl, disconnecting", GET_NAME(d->character));
			notify_ssl_copyover(d);
			gnutls_bye(d->sslses, GNUTLS_SHUT_WR);
			close(d->descriptor);
			d->descriptor = -1;
		}
		else
		{
			// end mccp compression so client stops expecting compressed data
			if (d->out_compress)
			{
				compress_end(d, 1);
			}
			logit(LOG_STATUS, "copyover: %s is telnet fd=%d, preserving", GET_NAME(d->character), d->descriptor);
		}
	}

	// count items to save
	count_copyover_items(&num_descs, &num_mobs, &num_objs, &num_rooms);

	fp = fopen(COPYOVER_FILE, "wb");
	if (!fp)
	{
		logit(LOG_STATUS, "copyover: cant open %s for writing", COPYOVER_FILE);
		return;
	}

	// write header
	memset(&header, 0, sizeof(header));
	memcpy(header.magic, COPYOVER_MAGIC, 4);
	header.version         = COPYOVER_VERSION;
	header.timestamp       = time(NULL);
	header.num_descriptors = num_descs;
	header.num_mobs        = num_mobs;
	header.num_objects     = num_objs;
	header.num_rooms       = num_rooms;
	header.num_combat      = 0;

	if (fwrite(&header, sizeof(header), 1, fp) != 1 || fwrite(&mother_desc, sizeof(int), 1, fp) != 1 || fwrite(&mother_desc_ssl, sizeof(int), 1, fp) != 1 || fwrite(&ws_desc, sizeof(int), 1, fp) != 1)
	{
		logit(LOG_STATUS, "copyover: failed to write header/sockets");
		fclose(fp);
		return;
	}

	// write descriptors
	for (d = descriptor_list; d; d = d->next)
	{
		if (d->descriptor > 0 && d->connected == CON_PLAYING && d->character && !d->websocket && !d->sslses)
		{

			// keep socket open across exec
			copyover_prepare_socket(d->descriptor);
			write_desc_entry(fp, d);

			// let player know
			raw_write_to_fd(d->descriptor, "\r\n*** Copyover in progress... ***\r\n");
		}
	}

	// write mobs (skip pc pets - saved per-descriptor)
	for (ch = character_list; ch; ch = ch->next)
	{
		if (IS_NPC(ch) && ch->in_room >= 0 && !IS_PC_PET(ch))
		{
			write_mob_entry(fp, ch);
			write_mob_affects(fp, ch);
			write_mob_inventory(fp, ch);
		}
	}

	// write objects on ground, skip ship stuff - already loaded
	// also skip objects in ship rooms (dynamic vnums 60000-64999)
	for (obj = object_list; obj; obj = obj->next)
	{
		if (OBJ_ROOM(obj))
		{
			int vnum = OBJ_VNUM(obj);
			if (vnum == VOBJ_PANEL || vnum == VOBJ_ALL_SHIPS || vnum == VOBJ_CARGO_CRATE)
				continue;
			if (IS_SHIP_ROOM(obj->loc.room))
				continue;
			write_obj_entry(fp, obj);
		}
	}

	// write door states
	for (room = 0; room <= top_of_world; room++)
	{
		for (dir = 0; dir < NUM_EXITS; dir++)
		{
			if (world[room].dir_option[dir] && IS_SET(world[room].dir_option[dir]->exit_info, EX_ISDOOR))
			{
				write_room_door(fp, room, dir);
			}
		}
	}

	fclose(fp);

	logit(LOG_STATUS, "copyover: saved %d descs, %d mobs, %d objs, %d doors", num_descs, num_mobs, num_objs, num_rooms);

	// prepare listener sockets
	copyover_prepare_socket(mother_desc);
	copyover_prepare_socket(mother_desc_ssl);
	if (ws_desc > 0)
		copyover_prepare_socket(ws_desc);

	// exec new binary
	snprintf(exec_buf, sizeof(exec_buf), "%d", RUNNING_PORT);

	// copy new binary if exists
	if (access("src/dms_new", X_OK) == 0)
	{
		logit(LOG_STATUS, "copyover: copying src/dms_new to ./dms");
		rename("./dms", "./dms.old");
		rename("src/dms_new", "./dms");
	}

	logit(LOG_STATUS, "copyover: executing new binary...");

	execl("./dms", "dms", "-C", exec_buf, (char *)NULL);

	// if we get here, exec failed
	logit(LOG_STATUS, "copyover: execl failed: %s", strerror(errno));

	// try to tell players
	for (d = descriptor_list; d; d = d->next)
	{
		if (d->descriptor > 0)
		{
			raw_write_to_fd(d->descriptor, "\r\n*** Copyover FAILED! ***\r\n");
		}
	}
}

// find_player_by_name already declared in prototypes.h

// load a player character for copyover recovery
static P_char copyover_load_player(const char *name, P_desc d)
{
	P_char player;
	int    status;

	player = (P_char)mm_get(dead_mob_pool);
	if (!player)
		return NULL;

	clear_char(player);

	if (!dead_pconly_pool)
		dead_pconly_pool = mm_create("PC_ONLY", sizeof(struct pc_only_data), offsetof(struct pc_only_data, switched), mm_find_best_chunk(sizeof(struct pc_only_data), 10, 25));

	player->only.pc = (struct pc_only_data *)mm_get(dead_pconly_pool);

	player->desc = d;
	setCharPhysTypeInfo(player);

	status = restoreCharOnly(player, (char *)name);
	if (status < 0)
	{
		logit(LOG_STATUS, "copyover: failed to restore %s (status %d)", name, status);
		return NULL;
	}

	return player;
}

void copyover_recover(int *mother_desc, int *mother_desc_ssl, int *ws_desc)
{
	FILE                  *fp;
	struct copyover_header header;
	struct copyover_desc   desc_entry;
	struct copyover_room   room_entry;
	P_desc                 d;
	P_char                 ch;
	int                    i, rnum, save_room;

	copyover_in_progress = 1;

	fp = fopen(COPYOVER_FILE, "rb");
	if (!fp)
	{
		logit(LOG_STATUS, "copyover_recover: no %s found, normal boot", COPYOVER_FILE);
		copyover_in_progress = 0;
		return;
	}

	// read and verify header
	if (fread(&header, sizeof(header), 1, fp) != 1 || memcmp(header.magic, COPYOVER_MAGIC, 4) != 0 || header.version != COPYOVER_VERSION)
	{
		logit(LOG_STATUS, "copyover_recover: invalid header or version mismatch");
		fclose(fp);
		unlink(COPYOVER_FILE);
		copyover_in_progress = 0;
		return;
	}

	logit(LOG_STATUS, "copyover_recover: restoring %d descs, %d mobs, %d doors", header.num_descriptors, header.num_mobs, header.num_rooms);

	// read listener sockets
	if (fread(mother_desc, sizeof(int), 1, fp) != 1 || fread(mother_desc_ssl, sizeof(int), 1, fp) != 1 || fread(ws_desc, sizeof(int), 1, fp) != 1)
	{
		logit(LOG_STATUS, "copyover_recover: failed to read listener sockets");
		fclose(fp);
		unlink(COPYOVER_FILE);
		copyover_in_progress = 0;
		return;
	}

	// restore descriptors
	for (i = 0; i < header.num_descriptors; i++)
	{
		if (fread(&desc_entry, sizeof(desc_entry), 1, fp) != 1)
		{
			logit(LOG_STATUS, "copyover_recover: failed reading desc %d", i);
			break;
		}

		d = (P_desc)mm_get(dead_desc_pool);
		if (!d)
			continue;
		memset(d, 0, sizeof(struct descriptor_data));

		d->descriptor = desc_entry.fd;

		// check if fd is still valid socket
		int       sock_type;
		socklen_t optlen = sizeof(sock_type);
		if (getsockopt(d->descriptor, SOL_SOCKET, SO_TYPE, &sock_type, &optlen) < 0)
		{
			logit(LOG_STATUS, "copyover: fd=%d is NOT a valid socket! errno=%d", d->descriptor, errno);
			continue;
		}
		logit(LOG_STATUS, "copyover: fd=%d is valid socket type=%d", d->descriptor, sock_type);

		nonblock(d->descriptor);
		int opt = 1;
		setsockopt(d->descriptor, SOL_TCP, TCP_NODELAY, &opt, sizeof(opt));

		strncpy(d->host, desc_entry.host, sizeof(d->host) - 1);
		d->host[sizeof(d->host) - 1] = '\0';
		strncpy(d->host2, desc_entry.host2, sizeof(d->host2) - 1);
		d->host2[sizeof(d->host2) - 1] = '\0';
		d->term_type                   = desc_entry.term_type;
		d->gmcp_enabled                = desc_entry.gmcp_enabled;
		d->mtts_flags                  = desc_entry.mtts_flags;
		strlcpy(d->client_name, desc_entry.ttype_client, sizeof(d->client_name));
		d->ttype_state                                   = TTYPE_COMPLETE; // already negotiated before copyover
		d->wait                                          = 1;
		d->prompt_mode                                   = FALSE;
		d->connected                                     = -1; // temp state until player loads
		used_descs++;
		check_cp437(d);

		// load character
		if (desc_entry.player_name[0])
		{
			ch = copyover_load_player(desc_entry.player_name, d);
			if (ch)
			{
				d->character = ch;
				ch->desc     = d;
				d->connected = CON_PLAYING;

#ifdef USE_ACCOUNT
				// restore account for preserved telnet connections
				d->account = allocate_account();
				if (d->account)
				{
					d->account->acct_name = str_dup(desc_entry.player_name);
					if (read_account(d->account) == -1)
					{
						d->account = free_account(d->account);
					}
				}
#endif

				// make them alive
				SET_POS(ch, POS_STANDING + STAT_NORMAL);

				// add to character_list first
				ch->next       = character_list;
				character_list = ch;

				// use room from copyover data, not pfile
				save_room = desc_entry.room;
				if (save_room < 0 || save_room > top_of_world)
				{
					save_room = 0;
				}
				ch->in_room = NOWHERE;
				char_to_room(ch, save_room, FALSE);

				reset_char(ch);
				int items_result = restoreItemsOnly(ch, 0);
				logit(LOG_STATUS, "copyover: restoreItemsOnly for %s returned %d", GET_NAME(ch), items_result);

				// restore pets/followers with hp
				for (int p = 0; p < desc_entry.num_pets && p < 10; p++)
				{
					int pet_vnum = desc_entry.pet_vnums[p];
					if (pet_vnum > 0)
					{
						int pet_rnum = real_mobile(pet_vnum);
						if (pet_rnum >= 0)
						{
							P_char pet = read_mobile(pet_rnum, REAL);
							if (pet)
							{
								char_to_room(pet, save_room, FALSE);
								setup_pet(pet, ch, -1, PET_NOAGGRO);
								add_follower(pet, ch);
								// restore hp from before copyover
								GET_HIT(pet)     = desc_entry.pet_hit[p];
								GET_MAX_HIT(pet) = desc_entry.pet_max_hit[p];
								logit(LOG_STATUS, "copyover: restored pet %s for %s (%d/%d hp)", GET_NAME(pet), GET_NAME(ch), GET_HIT(pet), GET_MAX_HIT(pet));
							}
						}
					}
				}

				// stash fighting info for later restoration
				ch->specials.copyover_fighting_type = desc_entry.fighting_type;
				ch->specials.copyover_fighting_id   = desc_entry.fighting_id;
				if (desc_entry.fighting_name[0])
				{
					strncpy(ch->specials.copyover_fighting_name, desc_entry.fighting_name, sizeof(ch->specials.copyover_fighting_name) - 1);
					ch->specials.copyover_fighting_name[sizeof(ch->specials.copyover_fighting_name) - 1] = '\0';
				}

				raw_write_to_fd(d->descriptor, "\r\n*** Copyover complete! ***\r\n");

				logit(LOG_STATUS, "copyover: restored %s fd=%d room=%d fighting=%d", desc_entry.player_name, d->descriptor, save_room, desc_entry.fighting_type);
			}
			else
			{
				logit(LOG_STATUS, "copyover: failed to load %s", desc_entry.player_name);
				close(desc_entry.fd);
				mm_release(dead_desc_pool, d);
				used_descs--;
				continue;
			}
		}

		// add to descriptor list
		d->next         = descriptor_list;
		descriptor_list = d;
	}

	// restore mobs directly from saved data (zones were not reset)
	for (i = 0; i < header.num_mobs; i++)
	{
		struct copyover_mob    mob_entry;
		struct copyover_affect aff_entries[64];
		copyover_carried_item  inv_entries[256];
		int                    num_affs, num_inv;

		if (fread(&mob_entry, sizeof(mob_entry), 1, fp) != 1)
			break;

		// read affects into temp array
		num_affs = mob_entry.num_affects;
		if (num_affs > 64)
			num_affs = 64;
		for (int a = 0; a < mob_entry.num_affects; a++)
		{
			struct copyover_affect aff_entry;
			if (fread(&aff_entry, sizeof(aff_entry), 1, fp) != 1)
				break;
			if (a < 64)
				aff_entries[a] = aff_entry;
		}

		// read carried items into temp array
		num_inv = mob_entry.num_carrying;
		if (num_inv > 256)
			num_inv = 256;
		for (int c = 0; c < mob_entry.num_carrying; c++)
		{
			copyover_carried_item inv_entry;
			if (fread(&inv_entry, sizeof(inv_entry), 1, fp) != 1)
				break;
			if (c < 256)
				inv_entries[c] = inv_entry;
		}

		int mob_rnum = real_mobile(mob_entry.vnum);
		if (mob_rnum < 0)
			continue;

		rnum = real_room(mob_entry.room);
		if (rnum < 0 || rnum > top_of_world)
			continue;

		// spawn mob from vnum
		P_char mob = read_mobile(mob_rnum, REAL);
		if (!mob)
		{
			logit(LOG_STATUS, "copyover: read_mobile failed for vnum %d at mob %d", mob_entry.vnum, i);
			continue;
		}
		if (!mob->only.npc)
		{
			logit(LOG_STATUS, "copyover: mob has null only.npc for vnum %d at mob %d", mob_entry.vnum, i);
			continue;
		}

		// restore idnum first

		GET_IDNUM(mob) = mob_entry.idnum;

		// place in saved room
		char_to_room(mob, rnum, FALSE);

		// restore saved state
		GET_HIT(mob)          = mob_entry.hit;
		GET_MAX_HIT(mob)      = mob_entry.max_hit;
		GET_MANA(mob)         = mob_entry.mana;
		GET_MAX_MANA(mob)     = mob_entry.max_mana;
		GET_VITALITY(mob)     = mob_entry.vitality;
		GET_MAX_VITALITY(mob) = mob_entry.max_vitality;
		// force standing to avoid weird states like falling
		SET_POS(mob, POS_STANDING + STAT_NORMAL);

		// restore gold
		GET_GOLD(mob) = mob_entry.gold;

		// restore affects
		for (int a = 0; a < num_affs; a++)
		{
			struct affected_type af;
			memset(&af, 0, sizeof(af));
			af.type                   = aff_entries[a].type;
			af.wear_off_message_index = aff_entries[a].wear_off_message_index;
			af.duration               = aff_entries[a].duration;
			af.flags                  = aff_entries[a].flags;
			af.modifier               = aff_entries[a].modifier;
			af.location               = aff_entries[a].location;
			af.loc2                   = aff_entries[a].loc2;
			af.level                  = aff_entries[a].level;
			af.bitvector              = aff_entries[a].bitvector;
			af.bitvector2             = aff_entries[a].bitvector2;
			af.bitvector3             = aff_entries[a].bitvector3;
			af.bitvector4             = aff_entries[a].bitvector4;
			af.bitvector5             = aff_entries[a].bitvector5;
			affect_to_char(mob, &af);
		}

		// restore equipment
		for (int w = 0; w < MAX_WEAR; w++)
		{
			if (mob_entry.equipment_vnums[w] > 0)
			{
				P_obj obj = read_object(mob_entry.equipment_vnums[w], VIRTUAL);
				if (obj)
				{
					equip_char(mob, obj, w, 0);
				}
			}
		}

		// restore carried items
		for (int c = 0; c < num_inv; c++)
		{
			if (inv_entries[c].vnum > 0)
			{
				P_obj obj = read_object(inv_entries[c].vnum, VIRTUAL);
				if (obj)
				{
					obj_to_char(obj, mob);
				}
			}
		}

		// stash fighting info for later
		if (mob_entry.fighting_type)
		{
			mob->specials.copyover_fighting_type = mob_entry.fighting_type;
			mob->specials.copyover_fighting_id   = mob_entry.fighting_id;
			if (mob_entry.fighting_name[0])
			{
				strncpy(mob->specials.copyover_fighting_name, mob_entry.fighting_name, sizeof(mob->specials.copyover_fighting_name) - 1);
				mob->specials.copyover_fighting_name[sizeof(mob->specials.copyover_fighting_name) - 1] = '\0';
			}
		}
	}

	// restore objects on ground (including corpses)
	for (i = 0; i < header.num_objects; i++)
	{
		struct copyover_obj obj_entry;
		if (fread(&obj_entry, sizeof(obj_entry), 1, fp) != 1)
			break;

		rnum = real_room(obj_entry.room);
		if (rnum < 0 || rnum > top_of_world)
		{
			// skip contents too
			for (int c = 0; c < obj_entry.num_contents; c++)
			{
				struct copyover_obj_content dummy;
				fread(&dummy, sizeof(dummy), 1, fp);
			}
			continue;
		}

		// skip objects in ship rooms - ships are recreated fresh by initialize_ships()
		if (IS_SHIP_ROOM(rnum))
		{
			for (int c = 0; c < obj_entry.num_contents; c++)
			{
				struct copyover_obj_content dummy;
				fread(&dummy, sizeof(dummy), 1, fp);
			}
			continue;
		}

		// create object
		P_obj obj = read_object(obj_entry.vnum, VIRTUAL);
		if (!obj)
		{
			// skip contents
			for (int c = 0; c < obj_entry.num_contents; c++)
			{
				struct copyover_obj_content dummy;
				fread(&dummy, sizeof(dummy), 1, fp);
			}
			continue;
		}

		// restore state
		obj->type = obj_entry.type;
		memcpy(obj->value, obj_entry.value, sizeof(obj->value));
		memcpy(obj->timer, obj_entry.timer, sizeof(obj->timer));

		// restore name/desc for corpses
		// note: not freeing old pointers - they point to shared prototype strings
		if (obj_entry.name[0])
		{
			obj->name = str_dup(obj_entry.name);
		}
		if (obj_entry.short_desc[0])
		{
			obj->short_description = str_dup(obj_entry.short_desc);
		}
		if (obj_entry.description[0])
		{
			obj->description = str_dup(obj_entry.description);
		}

		// place in room
		obj_to_room(obj, rnum);

		// restore contents
		for (int c = 0; c < obj_entry.num_contents; c++)
		{
			struct copyover_obj_content cont_entry;
			if (fread(&cont_entry, sizeof(cont_entry), 1, fp) != 1)
				break;

			P_obj content = read_object(cont_entry.vnum, VIRTUAL);
			if (content)
			{
				obj_to_obj(content, obj);
			}
		}
	}

	// restore door states
	for (i = 0; i < header.num_rooms; i++)
	{
		if (fread(&room_entry, sizeof(room_entry), 1, fp) != 1)
			break;

		rnum = real_room(room_entry.vnum);
		if (rnum >= 0 && rnum <= top_of_world && room_entry.dir >= 0 && room_entry.dir < NUM_EXITS && world[rnum].dir_option[room_entry.dir])
		{
			world[rnum].dir_option[room_entry.dir]->exit_info = room_entry.state;
		}
	}

	fclose(fp);
	unlink(COPYOVER_FILE);

	logit(LOG_STATUS, "copyover_recover: complete");
}

// link up fighting pointers after zones loaded
void copyover_restore_combat(void)
{
	P_desc d;
	P_char ch, target;

	for (d = descriptor_list; d; d = d->next)
	{
		if (d->connected != CON_PLAYING || !d->character)
			continue;

		ch = d->character;
		if (ch->specials.copyover_fighting_type == 0)
			continue;

		if (ch->specials.copyover_fighting_type == 1)
		{
			// fighting mob - find hostile mob in room (skip our own pets)
			target = world[ch->in_room].people;
			while (target)
			{
				if (IS_NPC(target) && target != ch && get_linked_char(target, LNK_PET) != ch)
				{
					break;
				}
				target = target->next_in_room;
			}

			if (target && IS_NPC(target) && !IS_FIGHTING(ch))
			{
				set_fighting(ch, target);
				if (!IS_FIGHTING(target))
					set_fighting(target, ch);
				logit(LOG_STATUS, "copyover: restored combat %s vs %s", GET_NAME(ch), GET_NAME(target));

				// also make pets fight the same target
				struct follow_type *f;
				for (f = ch->followers; f; f = f->next)
				{
					if (IS_NPC(f->follower) && f->follower->in_room == ch->in_room && !IS_FIGHTING(f->follower))
					{
						set_fighting(f->follower, target);
						logit(LOG_STATUS, "copyover: pet %s joins combat vs %s", GET_NAME(f->follower), GET_NAME(target));
					}
				}
			}
		}
		else if (ch->specials.copyover_fighting_type == 2)
		{
			// fighting player
			target = find_player_by_name(ch->specials.copyover_fighting_name);
			if (target && !IS_FIGHTING(ch))
			{
				set_fighting(ch, target);
				if (!IS_FIGHTING(target))
					set_fighting(target, ch);
				logit(LOG_STATUS, "copyover: restored pvp %s vs %s", GET_NAME(ch), GET_NAME(target));
			}
		}

		ch->specials.copyover_fighting_type    = 0;
		ch->specials.copyover_fighting_id      = 0;
		ch->specials.copyover_fighting_name[0] = '\0';
	}
}

// buffer-based helpers for redis world state saves

void copyover_count_items(int *num_mobs, int *num_objs, int *num_rooms)
{
	P_char ch;
	P_obj  obj;
	int    room, dir;

	*num_mobs  = 0;
	*num_objs  = 0;
	*num_rooms = 0;

	for (ch = character_list; ch; ch = ch->next)
	{
		if (IS_NPC(ch) && ch->in_room >= 0 && !IS_PC_PET(ch))
		{
			(*num_mobs)++;
		}
	}

	for (obj = object_list; obj; obj = obj->next)
	{
		if (OBJ_ROOM(obj))
		{
			int vnum = OBJ_VNUM(obj);
			if (vnum == VOBJ_PANEL || vnum == VOBJ_ALL_SHIPS || vnum == VOBJ_CARGO_CRATE)
				continue;
			(*num_objs)++;
		}
	}

	for (room = 0; room <= top_of_world; room++)
	{
		for (dir = 0; dir < NUM_EXITS; dir++)
		{
			if (world[room].dir_option[dir] && IS_SET(world[room].dir_option[dir]->exit_info, EX_ISDOOR))
			{
				(*num_rooms)++;
			}
		}
	}
}

int copyover_write_mob_to_buffer(P_char mob, char *buf, size_t max_len)
{
	struct copyover_mob    entry;
	struct copyover_affect aff_entry;
	copyover_carried_item  inv_entry;
	struct affected_type  *af;
	P_obj                  obj;
	size_t                 offset = 0;

	if (max_len < sizeof(entry))
		return -1;

	int mob_rnum = GET_RNUM(mob);
	if (mob_rnum < 0)
		return -1;

	memset(&entry, 0, sizeof(entry));
	entry.vnum         = mob_index[mob_rnum].virtual_number;
	entry.idnum        = GET_IDNUM(mob);
	entry.room         = world[mob->in_room].number;
	entry.hit          = GET_HIT(mob);
	entry.max_hit      = GET_MAX_HIT(mob);
	entry.mana         = GET_MANA(mob);
	entry.max_mana     = GET_MAX_MANA(mob);
	entry.vitality     = GET_VITALITY(mob);
	entry.max_vitality = GET_MAX_VITALITY(mob);
	entry.position     = GET_POS(mob);

	if (mob->specials.fighting)
	{
		P_char target = mob->specials.fighting;
		if (!target)
		{
			// stale pointer, skip
		}
		else if (IS_NPC(target))
		{
			entry.fighting_type = 2;
			entry.fighting_id   = GET_IDNUM(target);
		}
		else
		{
			entry.fighting_type = 1;
			if (GET_NAME(target))
			{
				strncpy(entry.fighting_name, GET_NAME(target), sizeof(entry.fighting_name) - 1);
				entry.fighting_name[sizeof(entry.fighting_name) - 1] = '\0';
			}
		}
	}

	entry.num_affects = 0;
	for (af = mob->affected; af; af = af->next)
		entry.num_affects++;

	for (int w = 0; w < MAX_WEAR; w++)
	{
		if (mob->equipment[w])
			entry.equipment_vnums[w] = OBJ_VNUM(mob->equipment[w]);
		else
			entry.equipment_vnums[w] = -1;
	}

	entry.num_carrying = 0;
	for (obj = mob->carrying; obj; obj = obj->next_content)
		entry.num_carrying++;

	entry.gold = GET_GOLD(mob);

	memcpy(buf + offset, &entry, sizeof(entry));
	offset += sizeof(entry);

	// write affects
	for (af = mob->affected; af; af = af->next)
	{
		if (offset + sizeof(aff_entry) > max_len)
			return -1;

		memset(&aff_entry, 0, sizeof(aff_entry));
		aff_entry.type                   = af->type;
		aff_entry.wear_off_message_index = af->wear_off_message_index;
		aff_entry.duration               = af->duration;
		aff_entry.flags                  = af->flags;
		aff_entry.modifier               = af->modifier;
		aff_entry.location               = af->location;
		aff_entry.loc2                   = af->loc2;
		aff_entry.level                  = af->level;
		aff_entry.bitvector              = af->bitvector;
		aff_entry.bitvector2             = af->bitvector2;
		aff_entry.bitvector3             = af->bitvector3;
		aff_entry.bitvector4             = af->bitvector4;
		aff_entry.bitvector5             = af->bitvector5;

		memcpy(buf + offset, &aff_entry, sizeof(aff_entry));
		offset += sizeof(aff_entry);
	}

	// write inventory
	for (obj = mob->carrying; obj; obj = obj->next_content)
	{
		if (offset + sizeof(inv_entry) > max_len)
			return -1;

		memset(&inv_entry, 0, sizeof(inv_entry));
		inv_entry.obj_uid = obj->obj_uid;
		inv_entry.vnum    = OBJ_VNUM(obj);
		memcpy(buf + offset, &inv_entry, sizeof(inv_entry));
		offset += sizeof(inv_entry);
	}

	return (int)offset;
}

int copyover_write_obj_to_buffer(P_obj obj, char *buf, size_t max_len)
{
	struct copyover_obj         entry;
	struct copyover_obj_content cont_entry;
	P_obj                       content;
	size_t                      offset = 0;

	if (max_len < sizeof(entry))
		return -1;

	memset(&entry, 0, sizeof(entry));
	entry.obj_uid = obj->obj_uid;
	entry.vnum    = OBJ_VNUM(obj);
	entry.room    = world[obj->loc.room].number;
	entry.type    = obj->type;
	memcpy(entry.value, obj->value, sizeof(entry.value));
	memcpy(entry.timer, obj->timer, sizeof(entry.timer));

	if (obj->name)
	{
		strncpy(entry.name, obj->name, sizeof(entry.name) - 1);
		entry.name[sizeof(entry.name) - 1] = '\0';
	}
	if (obj->short_description)
	{
		strncpy(entry.short_desc, obj->short_description, sizeof(entry.short_desc) - 1);
		entry.short_desc[sizeof(entry.short_desc) - 1] = '\0';
	}
	if (obj->description)
	{
		strncpy(entry.description, obj->description, sizeof(entry.description) - 1);
		entry.description[sizeof(entry.description) - 1] = '\0';
	}

	entry.num_contents = 0;
	for (content = obj->contains; content; content = content->next_content)
		entry.num_contents++;

	memcpy(buf + offset, &entry, sizeof(entry));
	offset += sizeof(entry);

	// write contents
	for (content = obj->contains; content; content = content->next_content)
	{
		if (offset + sizeof(cont_entry) > max_len)
			return -1;

		memset(&cont_entry, 0, sizeof(cont_entry));
		cont_entry.obj_uid = content->obj_uid;
		cont_entry.vnum    = OBJ_VNUM(content);
		memcpy(buf + offset, &cont_entry, sizeof(cont_entry));
		offset += sizeof(cont_entry);
	}

	return (int)offset;
}

int copyover_write_door_to_buffer(int room_rnum, int dir, char *buf, size_t max_len)
{
	struct copyover_room entry;

	if (max_len < sizeof(entry))
		return -1;

	memset(&entry, 0, sizeof(entry));
	entry.vnum  = world[room_rnum].number;
	entry.dir   = dir;
	entry.state = world[room_rnum].dir_option[dir]->exit_info;

	memcpy(buf, &entry, sizeof(entry));
	return sizeof(entry);
}

int copyover_write_zone_age_to_buffer(int zone_rnum, char *buf, size_t max_len)
{
	struct zone_age_entry entry;

	if (max_len < sizeof(entry))
		return -1;

	memset(&entry, 0, sizeof(entry));
	entry.zone_rnum          = zone_rnum;
	entry.age                = zone_table[zone_rnum].age;
	entry.lifespan           = zone_table[zone_rnum].lifespan;
	entry.fullreset_age      = zone_table[zone_rnum].fullreset_age;
	entry.fullreset_lifespan = zone_table[zone_rnum].fullreset_lifespan;

	memcpy(buf, &entry, sizeof(entry));
	return sizeof(entry);
}

P_char copyover_restore_mob_from_buffer(const char *buf, size_t len, size_t *bytes_read)
{
	struct copyover_mob    mob_entry;
	struct copyover_affect aff_entry;
	copyover_carried_item  inv_entry;
	size_t                 offset = 0;
	int                    rnum;
	P_char                 mob;

	if (len < sizeof(mob_entry))
	{
		*bytes_read = 0;
		return NULL;
	}

	memcpy(&mob_entry, buf + offset, sizeof(mob_entry));
	offset += sizeof(mob_entry);

	int mob_rnum = real_mobile(mob_entry.vnum);
	if (mob_rnum < 0)
	{
		// skip affects and inventory
		offset += mob_entry.num_affects * sizeof(aff_entry);
		offset += mob_entry.num_carrying * sizeof(inv_entry);
		*bytes_read = offset;
		return NULL;
	}

	rnum = real_room(mob_entry.room);
	if (rnum < 0 || rnum > top_of_world)
	{
		offset += mob_entry.num_affects * sizeof(aff_entry);
		offset += mob_entry.num_carrying * sizeof(inv_entry);
		*bytes_read = offset;
		return NULL;
	}

	mob = read_mobile(mob_rnum, REAL);
	if (!mob)
	{
		offset += mob_entry.num_affects * sizeof(aff_entry);
		offset += mob_entry.num_carrying * sizeof(inv_entry);
		*bytes_read = offset;
		return NULL;
	}

	GET_IDNUM(mob) = mob_entry.idnum;
	char_to_room(mob, rnum, FALSE);
	GET_HIT(mob)          = mob_entry.hit;
	GET_MAX_HIT(mob)      = mob_entry.max_hit;
	GET_MANA(mob)         = mob_entry.mana;
	GET_MAX_MANA(mob)     = mob_entry.max_mana;
	GET_VITALITY(mob)     = mob_entry.vitality;
	GET_MAX_VITALITY(mob) = mob_entry.max_vitality;
	SET_POS(mob, POS_STANDING + STAT_NORMAL);
	GET_GOLD(mob) = mob_entry.gold;

	// restore affects
	for (int a = 0; a < mob_entry.num_affects; a++)
	{
		if (offset + sizeof(aff_entry) > len)
			break;

		memcpy(&aff_entry, buf + offset, sizeof(aff_entry));
		offset += sizeof(aff_entry);

		struct affected_type af;
		memset(&af, 0, sizeof(af));
		af.type                   = aff_entry.type;
		af.wear_off_message_index = aff_entry.wear_off_message_index;
		af.duration               = aff_entry.duration;
		af.flags                  = aff_entry.flags;
		af.modifier               = aff_entry.modifier;
		af.location               = aff_entry.location;
		af.loc2                   = aff_entry.loc2;
		af.level                  = aff_entry.level;
		af.bitvector              = aff_entry.bitvector;
		af.bitvector2             = aff_entry.bitvector2;
		af.bitvector3             = aff_entry.bitvector3;
		af.bitvector4             = aff_entry.bitvector4;
		af.bitvector5             = aff_entry.bitvector5;
		affect_to_char(mob, &af);
	}

	// restore equipment
	for (int w = 0; w < MAX_WEAR; w++)
	{
		if (mob_entry.equipment_vnums[w] > 0)
		{
			// debug: log redis equipment restore for artifact 58424
			if (mob_entry.equipment_vnums[w] == 58424)
			{
				logit(LOG_DEBUG, "[copyover.c] REDIS restoring artifact 58424 on mob '%s' vnum=%d room=%d slot=%d", GET_NAME(mob), mob_entry.vnum, mob_entry.room, w);
			}
			P_obj obj = read_object(mob_entry.equipment_vnums[w], VIRTUAL);
			if (obj)
				equip_char(mob, obj, w, 0);
		}
	}

	// restore inventory
	for (int c = 0; c < mob_entry.num_carrying; c++)
	{
		if (offset + sizeof(inv_entry) > len)
			break;

		memcpy(&inv_entry, buf + offset, sizeof(inv_entry));
		offset += sizeof(inv_entry);

		if (inv_entry.vnum > 0)
		{
			P_obj obj = read_object(inv_entry.vnum, VIRTUAL);
			if (obj)
				obj_to_char(obj, mob);
		}
	}

	// stash fighting info for later restoration
	if (mob_entry.fighting_type)
	{
		mob->specials.copyover_fighting_type = mob_entry.fighting_type;
		mob->specials.copyover_fighting_id   = mob_entry.fighting_id;
		if (mob_entry.fighting_name[0])
		{
			strncpy(mob->specials.copyover_fighting_name, mob_entry.fighting_name, sizeof(mob->specials.copyover_fighting_name) - 1);
			mob->specials.copyover_fighting_name[sizeof(mob->specials.copyover_fighting_name) - 1] = '\0';
		}
	}

	*bytes_read = offset;
	return mob;
}

P_obj copyover_restore_obj_from_buffer(const char *buf, size_t len, size_t *bytes_read)
{
	struct copyover_obj         obj_entry;
	struct copyover_obj_content cont_entry;
	size_t                      offset = 0;
	int                         rnum;
	P_obj                       obj;

	if (len < sizeof(obj_entry))
	{
		*bytes_read = 0;
		return NULL;
	}

	memcpy(&obj_entry, buf + offset, sizeof(obj_entry));
	offset += sizeof(obj_entry);

	rnum = real_room(obj_entry.room);
	if (rnum < 0 || rnum > top_of_world)
	{
		offset += obj_entry.num_contents * sizeof(cont_entry);
		*bytes_read = offset;
		return NULL;
	}

	obj = read_object(obj_entry.vnum, VIRTUAL);
	if (!obj)
	{
		offset += obj_entry.num_contents * sizeof(cont_entry);
		*bytes_read = offset;
		return NULL;
	}

	// restore saved obj_uid if valid
	if (obj_entry.obj_uid > 0)
		obj->obj_uid = obj_entry.obj_uid;

	obj->type = obj_entry.type;
	memcpy(obj->value, obj_entry.value, sizeof(obj->value));
	memcpy(obj->timer, obj_entry.timer, sizeof(obj->timer));

	if (obj_entry.name[0])
		obj->name = str_dup(obj_entry.name);
	if (obj_entry.short_desc[0])
		obj->short_description = str_dup(obj_entry.short_desc);
	if (obj_entry.description[0])
		obj->description = str_dup(obj_entry.description);

	obj_to_room(obj, rnum);

	// restore contents
	for (int c = 0; c < obj_entry.num_contents; c++)
	{
		if (offset + sizeof(cont_entry) > len)
			break;

		memcpy(&cont_entry, buf + offset, sizeof(cont_entry));
		offset += sizeof(cont_entry);

		P_obj content = read_object(cont_entry.vnum, VIRTUAL);
		if (content)
		{
			if (cont_entry.obj_uid > 0)
				content->obj_uid = cont_entry.obj_uid;
			obj_to_obj(content, obj);
		}
	}

	*bytes_read = offset;
	return obj;
}

int copyover_restore_door_from_buffer(const char *buf, size_t len, size_t *bytes_read)
{
	struct copyover_room room_entry;
	int                  rnum;

	if (len < sizeof(room_entry))
	{
		*bytes_read = 0;
		return -1;
	}

	memcpy(&room_entry, buf, sizeof(room_entry));
	*bytes_read = sizeof(room_entry);

	rnum = real_room(room_entry.vnum);
	if (rnum >= 0 && rnum <= top_of_world && room_entry.dir >= 0 && room_entry.dir < NUM_EXITS && world[rnum].dir_option[room_entry.dir])
	{
		world[rnum].dir_option[room_entry.dir]->exit_info = room_entry.state;
		return 0;
	}
	return -1;
}

int copyover_restore_zone_age_from_buffer(const char *buf, size_t len, size_t *bytes_read)
{
	struct zone_age_entry entry;

	if (len < sizeof(entry))
	{
		*bytes_read = 0;
		return -1;
	}

	memcpy(&entry, buf, sizeof(entry));
	*bytes_read = sizeof(entry);

	if (entry.zone_rnum >= 0 && entry.zone_rnum <= top_of_zone_table)
	{
		zone_table[entry.zone_rnum].age                = entry.age;
		zone_table[entry.zone_rnum].lifespan           = entry.lifespan;
		zone_table[entry.zone_rnum].fullreset_age      = entry.fullreset_age;
		zone_table[entry.zone_rnum].fullreset_lifespan = entry.fullreset_lifespan;
		return 0;
	}
	return -1;
}
