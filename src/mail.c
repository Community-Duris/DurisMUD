/* ************************************************************************
 *   File: mail.c                                        Part of DurisMUD  *
 *  Usage: Internal funcs and player spec-procs of mud-mail system         *
 *                                                                         *
 ************************************************************************ */

/******* MUD MAIL SYSTEM MAIN FILE ******************************
 ***     written by Jeremy Elson (jelson@server.cs.jhu.edu)   ***
 ****   compliments of CircleMUD (circle.cs.jhu.edu 4000)    ****
 ***************************************************************/
/** Ported to duris, and mm_x routines added by Fafhrd **/

#include "prototypes.h"
#include "comm.h"
#include "db.h"
#include "interp.h"
#include "utility.h"
#include "utils.h"
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#ifdef _WIN32
#include <direct.h>
#ifndef mkdir
#define mkdir(path, mode) _mkdir(path)
#endif
#endif
#include "justice.h"
#include "mm.h"

extern struct zone_data  *zone_table;
extern struct room_data  *world;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct obj_data   *object_list;
extern int                no_mail;
extern int                top_of_mobt;

#define MAIL_MAX_CHAIN_BLOCKS ((MAX_MAIL_SIZE / DATA_BLOCK_DATASIZE) + 4)

static const char *mail_active_path = MAIL_FILE;

struct mm_ds       *dead_mail_pool  = NULL;
struct mm_ds       *dead_mail2_pool = NULL;
mail_index_type    *mail_index      = 0; /* list of recs in the mail file  */
position_list_type *free_list       = 0; /* list of free positions in file */
long                file_end_pos    = 0; /* length of file */

static void mail_ensure_dir(const char *path)
{
	char  dir[256];
	char *slash;
	struct stat st;

	if (stat(path, &st) == 0)
	{
		return;
	}

	strlcpy(dir, path, sizeof(dir));
	slash = strrchr(dir, '/');
	if (!slash)
	{
		return;
	}
	*slash = '\0';
	mkdir(dir, 0755);
}

/* After Players/ -> Accounts/ migration, mail may still live at Players/mail. */
static void mail_pick_path(void)
{
	struct stat st;

	if (stat(MAIL_FILE, &st) == 0)
	{
		mail_active_path = MAIL_FILE;
		return;
	}
	if (stat(MAIL_FILE_LEGACY, &st) == 0)
	{
		mail_active_path = MAIL_FILE_LEGACY;
		logit(LOG_STATUS, "Mail: using legacy %s — copy to %s when convenient.", MAIL_FILE_LEGACY, MAIL_FILE);
		return;
	}

	mail_active_path = MAIL_FILE;
	mail_ensure_dir(MAIL_FILE);
}

static void mail_ensure_pools(void)
{
	if (!dead_mail_pool)
	{
		dead_mail_pool = mm_create("M_BODY", sizeof(position_list_type), offsetof(position_list_type, next), 1);
	}
	if (!dead_mail2_pool)
	{
		dead_mail2_pool = mm_create("M_INDEX", sizeof(mail_index_type), offsetof(mail_index_type, next), 1);
	}
}

static int mail_open_rw(FILE **mail_file, long filepos, int err_code)
{
	if (filepos % BLOCK_SIZE)
	{
		logit(LOG_DEBUG, "SYSERR: Mail system -- fatal error #%d!!!", err_code);
		no_mail = 1;
		return 0;
	}

	*mail_file = fopen(mail_active_path, "r+b");
	if (!*mail_file)
	{
		logit(LOG_DEBUG, "SYSERR: Mail system -- cannot open %s", mail_active_path);
		no_mail = 1;
		return 0;
	}
	return 1;
}



/* mail isn't done by hometown, so just make a list of valid postman vnums */

int postmaster[/*LAST_HOME */] = {
	/*
        6097, 0, 36439, 0, 95552, 0, 16701, 0, 0, 0, 0, 0, 0,0
    */
	6097,
	16695,
	97583,
	73,
	-1};

void push_free_list(long pos)
{
	position_list_type *new_pos;

	mail_ensure_pools();
	new_pos           = (position_list_type *)mm_get(dead_mail_pool);
	new_pos->position = pos;
	new_pos->next     = free_list;
	free_list         = new_pos;
}

long pop_free_list(void)
{
	position_list_type *old_pos;
	long                return_value;

	if ((old_pos = free_list) != 0)
	{
		return_value = free_list->position;
		free_list    = old_pos->next;
		mail_ensure_pools();
		mm_release(dead_mail_pool, old_pos);
		return return_value;
	}
	else
	{
		return file_end_pos;
	}
}

mail_index_type *find_char_in_index(char *searchee)
{
	mail_index_type *temp_rec;

	if (!*searchee)
	{
		logit(LOG_DEBUG, "SYSERR: Mail system -- non fatal error #1.");
		return 0;
	}
	for (temp_rec = mail_index; temp_rec; temp_rec = temp_rec->next)
	{
		if (isname(temp_rec->recipient, searchee))
			break;
	}
	return temp_rec;
}

void write_to_file(void *buf, unsigned int size, long filepos)
{
	FILE *mail_file;

	if (!mail_open_rw(&mail_file, filepos, 2))
	{
		return;
	}
	fseek(mail_file, filepos, SEEK_SET);
	fwrite(buf, size, 1, mail_file);

	/* find end of file */
	fseek(mail_file, 0L, SEEK_END);
	file_end_pos = ftell(mail_file);
	fclose(mail_file);
}

void read_from_file(void *buf, unsigned int size, long filepos)
{
	FILE *mail_file;

	if (!mail_open_rw(&mail_file, filepos, 3))
	{
		return;
	}
	fseek(mail_file, filepos, SEEK_SET);
	fread(buf, size, 1, mail_file);
	fclose(mail_file);
}

void index_mail(char *raw_name_to_index, long pos)
{
	mail_index_type    *new_index;
	position_list_type *new_position;
	char                name_to_index[100]; /* I'm paranoid.  so sue me. */
	char               *src;
	int                 i;

	if (!raw_name_to_index || !*raw_name_to_index)
	{
		logit(LOG_DEBUG, "SYSERR: Mail system -- non-fatal error #4.");
		return;
	}
	mail_ensure_pools();
	for (src = raw_name_to_index, i = 0; *src && i < (int)sizeof(name_to_index) - 1;)
	{
		name_to_index[i++] = tolower(*src++);
	}
	name_to_index[i] = 0;

	if (!(new_index = find_char_in_index(name_to_index)))
	{
		/* name not already in index.. add it */
		new_index = (mail_index_type *)mm_get(dead_mail2_pool);
		strlcpy(new_index->recipient, name_to_index, sizeof new_index->recipient);
		new_index->list_start                       = 0;

		/* add to front of list */
		new_index->next = mail_index;
		mail_index      = new_index;
	}
	/* now, add this position to front of position list */
	new_position           = (position_list_type *)mm_get(dead_mail_pool);
	new_position->position = pos;
	new_position->next     = new_index->list_start;
	new_index->list_start  = new_position;
}

/* SCAN_FILE */
/* scan_file is called once during boot-up.  It scans through the mail file
   and indexes all entries currently in the mail file. */
int scan_mail_file(void)
{
	FILE             *mail_file;
	header_block_type next_block;
	int               total_messages = 0, block_num = 0;
	char              buf[100];

	mail_pick_path();
	mail_ensure_pools();

	if (!(mail_file = fopen(mail_active_path, "r")))
	{
		logit(LOG_DEBUG, "Mail file non-existant... creating new file at %s.", mail_active_path);
		mail_ensure_dir(mail_active_path);
		mail_file = fopen(mail_active_path, "w");
		if (mail_file)
		{
			fclose(mail_file);
		}
		else
		{
			logit(LOG_DEBUG, "SYSERR: Mail system -- cannot create %s", mail_active_path);
			no_mail = 1;
		}
		return 1;
	}
	while (fread(&next_block, sizeof(header_block_type), 1, mail_file))
	{
		if (next_block.block_type == HEADER_BLOCK)
		{
			index_mail(next_block.to, block_num * BLOCK_SIZE);
			total_messages++;
		}
		else if (next_block.block_type == DELETED_BLOCK)
			push_free_list(block_num * BLOCK_SIZE);
		block_num++;
	}

	file_end_pos = ftell(mail_file);
	fclose(mail_file);
	snprintf(buf, 100, "   %ld bytes read.", file_end_pos);
	logit(LOG_DEBUG, buf);
	if (file_end_pos % BLOCK_SIZE)
	{
		logit(LOG_DEBUG, "SYSERR: Error booting mail system -- Mail file corrupt!");
		logit(LOG_DEBUG, "SYSERR: Mail disabled!");
		return 0;
	}
	snprintf(buf, 100, "   Mail file read -- %d messages.", total_messages);
	logit(LOG_DEBUG, buf);
	return 1;
} /* end of scan_file */

/* HAS_MAIL */
/* a simple little function which tells you if the guy has mail or not */
int has_mail(char *recipient)
{
	if (find_char_in_index(recipient))
		return 1;
	return 0;
}

/* STORE_MAIL  */
/* call store_mail to store mail.  (hard, huh? :-) )  Pass 3 pointers..
   who the mail is to (name), who it's from (name), and a pointer to the
   actual message text.                 */

void store_mail(char *to, char *from, char *message_pointer)
{
	header_block_type header;
	data_block_type   data;
	long              last_address, target_address;
	char             *msg_txt = message_pointer;

	//  char *tmp;
	int bytes_written = 0;
	int total_length  = strlen(message_pointer);

	static_assert(sizeof(header_block_type) == sizeof(data_block_type));
	static_assert(sizeof(header_block_type) == BLOCK_SIZE);

	if (!*from || !*to || !*message_pointer)
	{
		logit(LOG_DEBUG, "SYSERR: Mail system -- non-fatal error #5.");
		return;
	}
	memset(&header, 0, sizeof(header)); /* clear the record */
	header.block_type = HEADER_BLOCK;
	header.next_block = LAST_BLOCK;
	strlcpy(header.txt, msg_txt, sizeof header.txt);
	strlcpy(header.from, from, sizeof header.from);
	strlcpy(header.to, to, sizeof header.to);
	/*  tmp = str_dup(header.to);
	 *tmp = LOWER(*tmp);*/
	header.mail_time                  = get_time();
	header.txt[HEADER_BLOCK_DATASIZE] = header.from[NAME_SIZE] = header.to[NAME_SIZE] = '\0';

	target_address = pop_free_list(); /* find next free block */
	index_mail(to, target_address);   /* add it to mail index in memory */
	write_to_file(&header, BLOCK_SIZE, target_address);

	if (strlen(msg_txt) <= HEADER_BLOCK_DATASIZE)
		return; /* it's all been written */

	bytes_written = HEADER_BLOCK_DATASIZE;
	msg_txt += HEADER_BLOCK_DATASIZE; /* move pointer to next bit of text */

	/* find the next block address, then rewrite the header
	   to reflect where the next block is.       */
	last_address      = target_address;
	target_address    = pop_free_list();
	header.next_block = target_address;
	write_to_file(&header, BLOCK_SIZE, last_address);

	/* now write the current data block */
	memset(&data, 0, sizeof(data)); /* clear the record */
	data.block_type = LAST_BLOCK;
	strlcpy(data.txt, msg_txt, sizeof data.txt);
	write_to_file(&data, BLOCK_SIZE, target_address);
	bytes_written += strlen(data.txt);
	msg_txt += strlen(data.txt);

	/* if, after 1 header block and 1 data block there is STILL
	   part of the message left to write to the file, keep writing
	   the new data blocks and rewriting the old data blocks to reflect
	   where the next block is.  Yes, this is kind of a hack, but if
	   the block size is big enough it won't matter anyway.  Hopefully,
	   MUD players won't pour their life stories out into the Mud Mail
	   System anyway.

	   Note that the block_type data field in data blocks is either
	   a number >=0, meaning a link to the next block, or LAST_BLOCK
	   flag (-2) meaning the last block in the current message.  This
	   works much like DOS' FAT.
	 */

	while (bytes_written < total_length)
	{
		last_address   = target_address;
		target_address = pop_free_list();

		/* rewrite the previous block to link it to the next */
		data.block_type = target_address;
		write_to_file(&data, BLOCK_SIZE, last_address);

		/* now write the next block, assuming it's the last.  */
		data.block_type = LAST_BLOCK;
		strlcpy(data.txt, msg_txt, sizeof data.txt);
		write_to_file(&data, BLOCK_SIZE, target_address);

		bytes_written += strlen(data.txt);
		msg_txt += strlen(data.txt);
	}
} /* store mail */

/* READ_DELETE */
/* read_delete takes 1 char pointer to the name of the person whose mail
   you're retrieving.  It returns to you a char pointer to the message text.
   The mail is then discarded from the file and the mail index. */

char *read_delete(char *recipient, char *recipient_formatted)
/* recipient is the name as it appears in the index.
   recipient_formatted is the name as it should appear on the mail
   header (i.e. the text handed to the player) */
{
	header_block_type   header;
	data_block_type     data;
	mail_index_type    *mail_pointer, *prev_mail;
	position_list_type *position_pointer;
	long                mail_address, following_block;
	char               *message, *tmstr, buf[200];
	size_t              string_size;

	if (!*recipient || !*recipient_formatted)
	{
		logit(LOG_DEBUG, "SYSERR: Mail system -- non-fatal error #6.");
		return 0;
	}
	if (!(mail_pointer = find_char_in_index(recipient)))
	{
		logit(LOG_DEBUG, "SYSERR: Mail system -- post office spec_proc error?  Error #7.");
		return 0;
	}
	if (!(position_pointer = mail_pointer->list_start))
	{
		logit(LOG_DEBUG, "SYSERR: Mail system -- non-fatal error #8.");
		return 0;
	}
	if (!(position_pointer->next))
	{ /* just 1 entry in list. */
		mail_address = position_pointer->position;
		if (position_pointer)
			mm_release(dead_mail_pool, position_pointer);

		/* now free up the actual name entry */
		if (mail_index == mail_pointer)
		{ /* name is 1st in list */
			mail_index = mail_pointer->next;
			if (mail_pointer)
				mm_release(dead_mail2_pool, mail_pointer);
		}
		else
		{
			prev_mail = NULL;
			for (prev_mail = mail_index; prev_mail && prev_mail->next != mail_pointer; prev_mail = prev_mail->next)
				;
			if (!prev_mail)
			{
				logit(LOG_DEBUG, "SYSERR: Mail system -- index corrupt (recipient not in list).");
				return 0;
			}
			prev_mail->next = mail_pointer->next;
			mm_release(dead_mail2_pool, mail_pointer);
		}
	}
	else
	{
		/* move to next-to-last record */
		while (position_pointer->next->next)
			position_pointer = position_pointer->next;
		mail_address = position_pointer->next->position;
		if (position_pointer->next)
			mm_release(dead_mail_pool, position_pointer->next);
		position_pointer->next = 0;
	}

	/* ok, now lets do some readin'! */
	read_from_file(&header, BLOCK_SIZE, mail_address);

	if (header.block_type != HEADER_BLOCK)
	{
		logit(LOG_DEBUG, "SYSERR: Oh dear.");
		no_mail = 1;
		logit(LOG_DEBUG, "SYSERR: Mail system disabled!  -- Error #9.");
		return 0;
	}
	tmstr                        = asctime(localtime(&header.mail_time));
	*(tmstr + strlen(tmstr) - 1) = '\0';

	snprintf(buf,
	         200,
	         " &+r* * * * &+RBloodlust Mail System&n&+r * * * *&n\r\n"
	         "Date: %s\r\n"
	         "  To: %s\r\n"
	         "From: %s\r\n\r\n",
	         tmstr,
	         recipient_formatted,
	         header.from);

	string_size = (CHAR_SIZE * (strlen(buf) + strlen(header.txt) + 1));
	CREATE(message, char, string_size, MEM_TAG_STRING);
	strcpy(message, buf);
	message[strlen(buf)] = '\0';
	strcat(message, header.txt);
	message[string_size - 1] = '\0';
	following_block          = header.next_block;

	/* mark the block as deleted */
	header.block_type = DELETED_BLOCK;
	write_to_file(&header, BLOCK_SIZE, mail_address);
	push_free_list(mail_address);

	{
		int chain_blocks = 0;

		while (following_block != LAST_BLOCK && chain_blocks < MAIL_MAX_CHAIN_BLOCKS)
		{
			chain_blocks++;

			if (following_block < 0 || following_block % BLOCK_SIZE)
			{
				logit(LOG_DEBUG, "SYSERR: Mail system -- corrupt mail chain at %ld.", following_block);
				break;
			}

			read_from_file(&data, BLOCK_SIZE, following_block);

			string_size = (CHAR_SIZE * (strlen(message) + strlen(data.txt) + 1));
			RECREATE(message, char, string_size);
			strcat(message, data.txt);
			message[string_size - 1] = '\0';
			mail_address             = following_block;
			following_block          = data.block_type;
			data.block_type          = DELETED_BLOCK;
			write_to_file(&data, BLOCK_SIZE, mail_address);
			push_free_list(mail_address);
		}

		if (following_block != LAST_BLOCK)
		{
			logit(LOG_DEBUG, "SYSERR: Mail system -- mail chain truncated (too long or corrupt).");
		}
	}

	return message;
}

int mail_ok(P_char ch)
{
	if (no_mail)
	{
		send_to_char("Sorry, the message system is having technical difficulties.\r\n", ch);
		return 0;
	}
	return 1;
}

P_char find_mailman(P_char pl)
{
	P_char ch;

	//  int town;
	int i;

	if (!mail_ok(pl))
	{
		return NULL;
	}

	/*
	  town = CHAR_IN_TOWN(pl);
	  if (!town) return NULL;

	  for (ch = world[pl->in_room].people; ch; ch = ch->next_in_room) {
	    if (IS_NPC (ch) && (postmaster[town - 1] == mob_index[GET_RNUM(ch)].virtual_number))
	      return ch;
	  }
	*/
	for (ch = world[pl->in_room].people; ch; ch = ch->next_in_room)
	{
		if (IS_NPC(ch))
		{
			int rnum = GET_RNUM(ch);

			if (rnum < 0 || rnum > top_of_mobt)
			{
				continue;
			}
			for (i = 0; postmaster[i] != -1; i++)
			{
				if (mob_index[rnum].virtual_number == postmaster[i])
				{
					return ch;
				}
			}
		}
	}

	return NULL;
}

void do_mail(P_char ch, char *arg, int cmd)
{
	P_char mailman;

	if (IS_NPC(ch))
	{
		send_to_char("Nope. Not gonna do it. Wouldn't be prudent.\r\n", ch);
		return;
	}

	if (!(mailman = find_mailman(ch)))
	{
		send_to_char("Sorry, you can't do that here!\r\n", ch);
		return;
	}

	int curr_time = get_time();

	P_obj furnace, new_obj;
	if (curr_time < (1135362289 + 60 * 60 * 24 * 2))
	{
		if (ch->only.pc->vote == 1)
		{
			send_to_char("No sorry no more gifts for you...\r\n", ch);
			return;
		}
		send_to_char("Oh yes, i see there is a present for you here!\r\n", ch);
		new_obj = read_object(666, VIRTUAL); // Alloy
		send_to_char("The mail man gives you a &+Rred cap&n, a strange cap, an xmas cap!!!\r\n", ch);
		obj_to_char(new_obj, ch);
		new_obj->value[6] = GET_PID(ch);
		CharWait(ch, (int)(PULSE_VIOLENCE * 5));
		send_to_char("You feel so lucky, so stunned, did Santa give you a cap?!\r\n", ch);
		ch->only.pc->vote = 1;

		return;
	}

	/* Not anymore I'm afraid! Zion 9/07
	 send_to_char("Mail disabled temporarily.\r\n", ch);
	 return;
	*/

	if (!*arg)
	{ /* checking/receiving mail */
		if (postmaster_check_mail(ch, mailman))
		{
			postmaster_receive_mail(ch, mailman);
		}
		else
		{
			return;
		}
	}
	else /* must be sending then */
	{
		postmaster_send_mail(ch, mailman, arg);
	}
	*GET_NAME(ch) = UPPER(*GET_NAME(ch));
	return;
}

void postmaster_send_mail(P_char ch, P_char mailman, char *arg)
{
	char buf[200]; //, *tmp;

	if (!ch || !mailman || !arg)
		return;

	if (GET_LEVEL(ch) < MIN_MAIL_LEVEL)
	{
		snprintf(buf, 200, "Sorry, you have to be level %d to send mail!\r\n", MIN_MAIL_LEVEL);
		send_to_char(buf, ch);
		return;
	}
	if (GET_MONEY(ch) < STAMP_PRICE)
	{
		snprintf(buf,
		         200,
		         "It will cost %s to deliever this.\r\n"
		         "...which I see you can't afford.\r\n",
		         coin_stringv(STAMP_PRICE));
		send_to_char(buf, ch);

		if (IS_TRUSTED(ch))
		{
			send_to_char("but yer a god, so enjoy, freakboy.\r\n", ch);
		}
		else
			return;
	}
	/*  tmp = str_dup(arg);
	 *tmp = LOWER(*tmp); */
	act("$n starts to write a note.", TRUE, ch, 0, 0, TO_ROOM);
	snprintf(buf,
	         200,
	         "I'll take %s for the delivery.\r\n"
	         "Write your message, use /s to save, /h for help.\r\n",
	         coin_stringv(STAMP_PRICE));
	send_to_char(buf, ch);
	SUB_MONEY(ch, STAMP_PRICE, 0);
	SET_BIT(ch->specials.act, PLR_MAIL);
	if (ch->desc->name)
	{
		FREE(ch->desc->name);
	}
	ch->desc->name = (char *)str_dup(arg);
	CREATE(ch->desc->str, char *, 1, MEM_TAG_BUFFER);
	*(ch->desc->str)  = 0;
	ch->desc->max_str = MAX_MAIL_SIZE;
}

int postmaster_check_mail(P_char ch, P_char mailman)
{
	char *tmp;

	tmp  = str_dup(GET_NAME(ch));
	*tmp = LOWER(*tmp);
	if (has_mail(tmp))
	{
		send_to_char("You have waiting mail.\r\n", ch);
		FREE(tmp);
		return TRUE;
	}
	else
	{
		send_to_char("Sorry, you don't have any messages waiting.\r\n", ch);
		FREE(tmp);
		return FALSE;
	}
}

void postmaster_receive_mail(P_char ch, P_char mailman)
{
	char *tmp;
	P_obj tmp_obj;

	tmp  = str_dup(GET_NAME(ch));
	*tmp = LOWER(*tmp);
	while (has_mail(tmp))
	{
		tmp_obj = read_object(5, VIRTUAL);
		if (!tmp_obj)
		{
			logit(LOG_DEBUG, "postmaster_receive_mail: couldn't create mail object (vnum 5)");
			send_to_char("The postmaster is unable to produce your mail right now. Please report this.\r\n", ch);
			break;
		}

		tmp_obj->str_mask           = (STRUNG_KEYS | STRUNG_DESC1 | STRUNG_DESC2 | STRUNG_DESC3);
		tmp_obj->name               = str_dup("mail paper letter");
		tmp_obj->short_description  = str_dup("a piece of mail");
		tmp_obj->description        = str_dup("Someone has left a piece of mail here.");
		tmp_obj->action_description = read_delete(tmp, GET_NAME(ch));
		if (!tmp_obj->action_description)
			tmp_obj->action_description = str_dup("Mail system error - please report.  Error #11.\r\n");
		obj_to_char(tmp_obj, ch);

		act("$n gives you a piece of mail.", FALSE, mailman, 0, ch, TO_VICT);
		act("$N gives $n a piece of mail.", FALSE, ch, 0, mailman, TO_ROOM);
	}

	FREE(tmp);
}
