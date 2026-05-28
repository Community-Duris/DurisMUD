
/*
 * copyright (c) 1996 by Gary Dezern
 */

#include "prototypes.h"
#include "structs.h"
#include "comm.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

#define MAX_LINES 4095

static int    edit_has_ansi(char *s);
static char **edit_text_to_data(const char *source);
static char  *edit_data_to_text(struct edit_data *data);
static int    edit_delete_line(struct edit_data *data, int loc);
static int    edit_insert_data(struct edit_data *data, char **lines);
static void   edit_list_data(struct edit_data *data);

void edit_free(struct edit_data *data)
{
	int i = 0;

	if (!data)
		return;

	while (data->lines[i])
		FREE(data->lines[i++]);

	data->desc->editor = NULL;
	FREE(data);
}

static int edit_has_ansi(char *s)
{
	char *t;

	return 1; /* alow ansi on boards & descs */

	if ((!s) || (!*s))
		return 0;

	for (t = s; *t; t++)
		if (*t == '&')
			switch (*(t + 1))
			{
				case '+':
				case '-':
				case '=':
				case 'n':
				case 'N':
					return 1;
				default:
					break;
			}
	return 0;
}

/* take a stream of unformatted text, and build it into a edit_data struct
 * format.  Do word-wrapping based on the edit_num_chars() function.
 * Lines are also broken on '\r' or '\n' chars.  If a '\r' follows a '\n'
 * (or vice versa), ignore (strip) it. */

static char **edit_text_to_data(const char *source)
{
	char        *buf; /* a copy of the source I can chop up  */
	char        *s;   /* current source 'line' pointer  */
	int          cur_line = 0;
	static char *stor[MAX_LINES + 1];

	if (!source)
		return NULL;

	memset(stor, 0, (MAX_LINES + 1) * sizeof(char *));

	buf = str_dup(source);
	s   = buf;

	/* special case if my _original_ string contains nothing but a null */

	if (!*s)
	{
		stor[0] = s;
		return stor;
	}
	while (*s)
	{
		int c    = 0;       /* how many printable chars have I passed?  */
		int mark = 0;       /* marks spaces as I pass them  */
		int loc  = 0;       /* how many chars (including invis
		                       ones) I've passed  */
		char *t = s;        /* t is a work pointer.  As well, at the
		                       end of the below 'while' loop, it
		                       should point to the character AFTER the
		                       last one in a line (a null will be
		                       inserted here) */
		char *n_new = NULL; /* this lets me know where the
		                       next line starts */
		while (!n_new && (c < 75))
		{
			if (!*t)
			{              /* eek.. end of buffer  */
				n_new = t; /* so it finds the null */
				break;
			}
			if (*t == '\r')
			{ /* ahh! newline! */
				n_new = t + 1;
				if (*(t + 1) == '\n') /* skip paired CR/LF's */
					n_new++;
				break;
			}
			if (*t == '\n')
			{
				n_new = t + 1;
				if (*(t + 1) == '\r')
					n_new++;
				break;
			}
			if (*t == '&') /* ansi check...  */
				switch (*(t + 1))
				{
					case '+':
					case '-':
					case '=':
						t++;
						loc++;
					case 'n':
					case 'N':
						t += 2;
						loc += 2;
						continue; /* find the next char... */
				}
			if (c && (*t == ' '))
			{ /* note this for word wrap... */
				mark = loc;
			}
			c++;
			t++;
			loc++;
		}
		/* okay... at this point, c is the number of characters we counted
		   which are printable.  'loc' is how many total we counted.
		   'n_new' is a pointer to the next piece of data.  If 'n_new' is
		   NULL, then we use 'mark' to determine where to word wrap... if
		   'n_new' is null, and there is no mark (mark == 0), then the line
		   has 76 chars, with no linefeed/CR, and no spaces.  In that
		   case, fuck them  */
		if (!n_new)
		{
			if (!mark)
			{
				/* duh.. forget to use the spacebar?! */
				FREE(buf);
				return NULL;
			}
			t     = s + mark; /* but we have no count of printable now */
			n_new = t + 1;
		}
		/* note that 'n_new' should NEVER be equal to 't', unless we are at
		   the end of the buffer */
		if (*n_new && (n_new == t))
			abort();

		*t = '\0';

		stor[cur_line++] = str_dup(s);
		if (cur_line > MAX_LINES)
		{
			/* GASP  */
			logit(LOG_EXIT, "editor.c, edit_text_to_data: %d lines exceeded!", MAX_LINES);
			abort();
		}
		s = n_new;
	}
	FREE(buf);
	return stor;
	/* okay.. thats all she wrote..  */
}

/* take an edit_data structure, and build a single text stream from it.
   Return the text stream in a newly allocated char pointer */
static char *edit_data_to_text(struct edit_data *data)
{
	char  *text;
	size_t space = 0;
	int    i;

	/* instead of walking through the array, an resizing the allocated
	   buffer as we go, first walk though the data, and find out just how
	   much space we need.... the length, 1 for \r, 1 for \n and 1 for a null */

	for (i = 0; data->lines[i]; i++)
		space += strlen(data->lines[i]) + 3;

	/* now add on a safe zone.. */
	space += 20;

	CREATE(text, char, space, MEM_TAG_STRING);

	*text = '\0';

	/* now walk through the array again, copying everything to 'text'.
	   Also, append a "\r\n" to each line */
	for (i = 0; data->lines[i]; i++)
	{
		strcat(text, data->lines[i]);
		strcat(text, "\r\n");
	}

	return text;
}

/* given an edit_data struct, and a line number which is 0-based, delete
   that line from the struct, and re-pack the array.   Return 0 on
   success, non-0 on error.  If 'loc' doesn't exist, just return non-zero.
   (its the only possible non-fatal error anyway) */

static int edit_delete_line(struct edit_data *data, int loc)
{
	int j = 0;
	int i;

	while (data->lines[j])
		j++;

	if (loc >= j)
		return 1;

	if ((loc < 0) || !data->lines[loc])
	{
		/* this is impossible.. but we check just to make sure  */
		abort();
	}
	/* free the string.. */
	FREE(data->lines[loc]);

	/* adjust cur_line if needed */
	if (loc < data->cur_line)
		data->cur_line--;

	/* finally walk through the array, moving each entry up one slot  */
	i = loc;
	while (i < j)
		data->lines[i] = data->lines[++i];

	return 0;
}

/* given an edit_data struct, and an array of strings, insert the strings
   into the edit_data at the proper position.  Check for overrunning the
   max size of the edit_data.

   If everything seems to have worked, return 0. If there was a max size
   overflow, return -1. If some "other" error, return 1 */

static int edit_insert_data(struct edit_data *data, char **lines)
{
	int i = 0, j = 0, new_cur;

	if (!lines)
		return 1;

	/* count the lines to make sure they fit  */
	while (lines[i])
		i++;

	while (data->lines[j])
		j++;

	if ((i + j) > data->max_lines)
	{
		i = 0;
		while (lines[i])
			FREE(lines[i++]);
		return -1;
	}
	/* stick 'em in.  First thing to do is move j lines from the cur_line
	   point down i lines.  Start at the BOTTOM to prevent overlap
	   problems  */

	/* loop from j to curline, moving each line down i lines  */
	while (j > data->cur_line)
		data->lines[j - 1 + i] = data->lines[--j];

	/* need the value of 'i' to update curline, but can't update curline
	   yet... */
	new_cur = i;

	/* then fill the "gap" with the new data */
	while (i)
		data->lines[data->cur_line + i - 1] = lines[--i];

	/* update cur_line  */
	data->cur_line += new_cur;
	return 0;
}

/* just list the lines in an edit_data struct */

static void edit_list_data(struct edit_data *data)
{
	char buf[4096];
	int  i = 0;

	snprintf(buf, 4096, "\r\n&+CEdit to a maximum of %d lines&N\r\n", data->max_lines);
	SEND_TO_Q(buf, data->desc);
	SEND_TO_Q("\r\n&+YType \"&+W+h&+Y\" on line by itself for help&N\r\n", data->desc);
	SEND_TO_Q("&+C------------------------------------&N\r\n", data->desc);

	while (data->lines[i])
	{
		if (i == data->cur_line)
			SEND_TO_Q("&+G --- CURRENT INSERT POINT ---&N\r\n", data->desc);
		snprintf(buf, 4096, "%3d&+W:&N %s\r\n", i + 1, data->lines[i]);
		SEND_TO_Q(buf, data->desc);
		i++;
	}
	SEND_TO_Q("\r\n", data->desc);
}

void edit_string_add(struct edit_data *data, char *str)
{
	char buf[4096];

	buf[0] = 0;

	if (!data)
		abort();

	if (!str)
		str = buf;

	if (!strcmp(str, "+l"))
	{
		edit_list_data(data);
	}
	else if (!strcmp(str, "+d all"))
	{
		int i = 0;

		while (data->lines[i])
		{
			FREE(data->lines[i]);
			data->lines[i++] = NULL;
		}
		data->cur_line = 0;
		SEND_TO_Q("&+YMessage cleared!&N\r\n\r\n", data->desc);
	}
	else if (!strncmp(str, "+d ", 3))
	{
		int ln = atoi(str + 3);

		if (!ln)
		{
			SEND_TO_Q("&+RInvalid line number!&N\r\n", data->desc);
		}
		else
		{
			if (edit_delete_line(data, ln - 1))
			{
				SEND_TO_Q("&+RInvalid line number!&N\r\n", data->desc);
			}
			else
			{
				snprintf(buf, sizeof buf, "&+YLine %d deleted.&N\r\n", ln);
				SEND_TO_Q(buf, data->desc);
			}
		}
	}
	else if (!strncmp(str, "+g ", 3))
	{
		int ln = atoi(str + 3);

		if (!ln)
		{
			SEND_TO_Q("&+RInvalid line number!&N\r\n", data->desc);
		}
		else
		{
			int c = 0;

			while (data->lines[c])
				c++;
			if (ln > c)
			{
				snprintf(buf, sizeof buf, "&+YMoved to end of buffer.&N\r\n");
				ln = c + 1;
			}
			else
			{
				snprintf(buf, sizeof buf, "&+YMoved to line %d.&N\r\n", ln);
			}
			SEND_TO_Q(buf, data->desc);
			data->cur_line = (ln - 1);
		}
	}
	else if (!strcmp(str, "+q"))
	{
		SEND_TO_Q("&+MCancelled edit!&N\r\n", data->desc);
		data->callback(data->desc, data->callback_data, NULL);
		edit_free(data);
		return;
	}
	else if (!strcmp(str, "+w"))
	{
		char *ret;

		if (data->lines)
			ret = edit_data_to_text(data);
		else
			CREATE(ret, char, 1, MEM_TAG_STRING);

		SEND_TO_Q("&+MOk.&N\r\n", data->desc);
		data->callback(data->desc, data->callback_data, ret);

		edit_free(data);
		return;
	}
	else if (!strcmp(str, "+h"))
	{
		SEND_TO_Q("\r\n&+CEditor commands:&N\r\n\r\n", data->desc);
		SEND_TO_Q("   &+W+l&N&+c     - list text&N\r\n", data->desc);
		SEND_TO_Q("   &+W+d n&N&+c   - delete line n&N\r\n", data->desc);
		SEND_TO_Q("   &+W+d all&N&+c - delete ALL lines&N\r\n", data->desc);
		SEND_TO_Q("   &+W+g n&N&+c   - goto line n and begin inserting lines there&N\r\n", data->desc);
		SEND_TO_Q("   &+W+q&N&+c     - quit WITHOUT saving&N\r\n", data->desc);
		SEND_TO_Q("   &+W+w&N&+c     - save and quit&N\r\n\r\n", data->desc);
	}
	else
	{
		/* oh boy.. */
		int    r;
		char **heh;

		if (!data->is_god && edit_has_ansi(str))
		{
			SEND_TO_Q("&+RNo ANSI characters allowed as input.  &+YDiscarding line.&N\r\n", data->desc);
			snprintf(buf, sizeof buf, "%3d&+W:&N ", data->cur_line + 1);
			SEND_TO_Q(buf, data->desc);
			return;
		}
		heh = edit_text_to_data(str);
		if (!heh)
		{
			SEND_TO_Q("&+RInvalid input.  &+YDiscarding line.&N\r\n", data->desc);
		}
		else
		{
			r = edit_insert_data(data, heh);
			if (r == -1)
			{
				SEND_TO_Q("&+RThat would exceed the maximum number of lines allowed!&N\r\n", data->desc);
			}
			else if (r == 1)
			{
				SEND_TO_Q("&+R&-LStrange error #1 in the editor.  Tell an Imp!&N\r\n", data->desc);
			}
		}
	}
	snprintf(buf, sizeof buf, "%3d&+W:&N ", data->cur_line + 1);
	SEND_TO_Q(buf, data->desc);
}

void edit_start(P_desc desc, char *old_text, int max_lines, void (*callback)(P_desc, int, char *), int callback_data)
{
	struct edit_data *data;
	char              buf[20];

	CREATE(data, edit_data, 1, MEM_TAG_EDITDAT);

	data->max_lines     = max_lines ? max_lines : MAX_LINES;
	data->cur_line      = 0;
	data->is_god        = (GET_LEVEL(desc->character) >= MINLVLIMMORTAL);
	data->desc          = desc;
	data->callback      = callback;
	data->callback_data = callback_data;
	CREATE(data->lines, char *, (unsigned)data->max_lines + 1, MEM_TAG_BUFFER);

	/* let the rest of the mud know that this person is in the editor */
	desc->editor = data;

	/* now take care of stuffing the old_text into the array */
	if (old_text && *old_text)
	{
		edit_string_add(data, old_text);
	}
	SEND_TO_Q("\r\n", data->desc);
	edit_list_data(data);

	snprintf(buf, 20, "%3d&+W:&N ", data->cur_line + 1);
	SEND_TO_Q(buf, data->desc);
	return;
}
