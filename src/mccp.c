#include <zlib.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "structs.h"
#include "prototypes.h"
#include "telnet.h"
#include "mccp.h"
#include "utils.h"

/* external variables used by this module */
extern P_desc descriptor_list;
extern long sentbytes;

/* global variables provided by this module */
int      mccp_alloc = 0;
int      mccp_free = 0;

const unsigned char compress_on_str[] = { IAC, WILL, TELOPT_COMPRESS, '\0' };
const unsigned char compress2_on_str[] = { IAC, WILL, TELOPT_COMPRESS2, '\0' };
const unsigned char enable_compress[] = { IAC, SB, TELOPT_COMPRESS, WILL, SE, '\0' };
const unsigned char enable_compress2[] = { IAC, SB, TELOPT_COMPRESS2, IAC, SE, '\0' };

void    *zlib_alloc(void *opaque, unsigned int items, unsigned int size);
void     zlib_free(void *opaque, void *address);
int      raw_write_to_descriptor(int desc, const char *txt, const int total);

void    *zlib_alloc(void *opaque, unsigned int items, unsigned int size)
{
  char *p;
  mccp_alloc++;
  CREATE(p, char, items * size, MEM_TAG_ZSTREAM);
  return (void *) p;
}

void zlib_free(void *opaque, void *address)
{
  mccp_free++;
  FREE(address);
}

void advertise_mccp(int desc)
{
  write_to_descriptor(desc, (const char *)compress2_on_str);
  write_to_descriptor(desc, (const char *)compress_on_str);
}

/* parse telnet options and return amount of characters 
 * to "cut" from input stream
 * If you ever need to make it "the right way", look into
 * sources of telnetd demon */
int parse_telnet_options(P_desc player, char *buf)
{
  char    *p = buf;
  int      mccp_ver = 0;

  if (*p != (signed char) IAC)
    return 0;
  switch (*(p + 1))
  {
  case (signed char) DO:
    switch (*(p + 2))
    {
    case TELOPT_COMPRESS:
      if (mccp_ver < MCCP_VER2) // prefer version 2 over 1
        mccp_ver = MCCP_VER1;
      break;
    case TELOPT_COMPRESS2:
      mccp_ver = MCCP_VER2;
      break;
    }
    break;
  case (signed char) DONT:
  case (signed char) WILL:
  case (signed char) WONT:
    if (*(p + 2) != '\0')
      return 3;
    else
      return 2;
    break;
  }

  if (mccp_ver)
  {
    compress_start(player, mccp_ver);
    return 3;
  }
  return 1;                     /* lets cut at least IAC from stream */
}

int compress_start(P_desc player, int mccp_version)
{
  z_stream *s;

  if (player->z_str)
    return 0;

  CREATE(s, z_stream, 1, MEM_TAG_ZSTREAM);
  //s = (z_stream *) malloc(sizeof(z_stream));
  CREATE(player->out_compress_buf, char, COMPRESS_BUF_SIZE, MEM_TAG_BUFFER);
  //player->out_compress_buf = (char *) malloc(COMPRESS_BUF_SIZE);

  s->next_in = NULL;
  s->avail_in = 0;
  s->next_out = (Bytef *) player->out_compress_buf;
  s->avail_out = COMPRESS_BUF_SIZE;
  s->zalloc = zlib_alloc;
  s->zfree = zlib_free;
  s->opaque = NULL;

  if (deflateInit(s, COMPRESS_EFFICIENCY) != Z_OK)
  {
    FREE(player->out_compress_buf);
    FREE(s);
    logit(LOG_DEBUG, "MCCP: deflateInit failed");
    return -1;
  }

  if (mccp_version == MCCP_VER1)
  {
    write_to_descriptor(player->descriptor, (const char *)enable_compress);
  }
  else if (mccp_version == MCCP_VER2)
  {
    write_to_descriptor(player->descriptor, (const char *)enable_compress2);
  }
  else
  {
    logit(LOG_DEBUG, "MCCP: unknown version %d", mccp_version);
  }
  player->out_compress = mccp_version;
  player->z_str = s;

  return 0;
}


/* ZMUD seems not to handle Z_FINISH event properly, so socket needs
 to be closed immediatly after stopping compression! */
int compress_end(P_desc player, int flush)
{
  unsigned char dummy[1] = { ' ' };
  int      status, len;

  if (!player->out_compress || !player->z_str)
    return 0;

  player->z_str->avail_in = 0;
  player->z_str->next_in = dummy;
  player->z_str->next_out = (Bytef *) player->out_compress_buf;
  player->z_str->avail_out = COMPRESS_BUF_SIZE;

  /* flush all pending data, Z_OK means there's still more data to process */
  if (flush)
  {
    do
    {
      status = deflate(player->z_str, Z_FINISH);
      if (status != Z_STREAM_END && status != Z_OK)
      {
        break;
      }
      len = (long) player->z_str->next_out - (long) player->out_compress_buf;
      raw_write_to_descriptor(player->descriptor, player->out_compress_buf,
                              len);
    }
    while (status != Z_STREAM_END);
  }

  deflateEnd(player->z_str);    /* free memory allocated by zlib */
  FREE(player->out_compress_buf);
  FREE(player->z_str);
  player->out_compress = 0;

  return 0;
}

/* use this function whenever you want to send anything to player,
 do not attempt to call raw_write_to_descriptor, or you may
 screw up compression */
int write_to_descriptor(int desc, const char *txt)
{
  int      len, total, status, i, j;
  P_desc   player;
  char     static_conv_buf[MAX_STRING_LENGTH];
  char    *conv_buf = static_conv_buf;;

  for (player = descriptor_list; player; player = player->next)
  {
    if (player->descriptor == desc)
      break;
  }


  if ((len = strlen(txt)) > MAX_STRING_LENGTH * 0.8)
    CREATE(conv_buf, char, (unsigned int)(len * 1.1), MEM_TAG_BUFFER);
    //conv_buf = (char *) malloc((unsigned int) (len * 1.1));

  for (i = 0, j = 0; txt[i]; i++)
  {
    if (txt[i] == '\n')
    {
      conv_buf[j++] = '\r';
      conv_buf[j++] = '\n';
    }
    else if (txt[i] != '\r')
      conv_buf[j++] = txt[i];
  }

  conv_buf[j] = '\0';
  txt = conv_buf;
  total = j;

  if (!player || !player->out_compress)
  {
    raw_write_to_descriptor(desc, txt, total);
  }
  else
  {
    if (player && player->z_str)
    {
      player->z_str->next_in = (unsigned char *) txt;
      player->z_str->avail_in = total;

      while (player->z_str->avail_in)
      {
        do
        {
          player->z_str->next_out = (Bytef *) player->out_compress_buf;
          player->z_str->avail_out = COMPRESS_BUF_SIZE;

          status = deflate(player->z_str, Z_SYNC_FLUSH);
          if (status != Z_OK)
          {
            logit(LOG_DEBUG, "MCCP: deflate failed");
            if (conv_buf != static_conv_buf)
              FREE(conv_buf);
            return (-1);
          }

          len = (long) player->z_str->next_out -
            (long) player->out_compress_buf;
          raw_write_to_descriptor(desc, player->out_compress_buf, len);

        }
        while (player->z_str->avail_out == 0);
      }
    }
  }

  if (conv_buf != static_conv_buf)
    FREE(conv_buf);

  return (0);
}

/* never ever call this function, unless you are write_to_descriptor */
int raw_write_to_descriptor(int desc, const char *txt, const int total)
{
  int      sofar, thisround;
  P_desc   d;

  sofar = 0;

  sentbytes += total;

  //Let's look up the player name and add bytes to it.
  for (d = descriptor_list; d; d = d->next)
  {
	
      if(d->descriptor == desc)
	 if(d->character)
  	   d->character->only.pc->send_data = d->character->only.pc->send_data + total;
  }

  do
  {
    thisround = write(desc, txt + sofar, (unsigned) (total - sofar));
    if (thisround < 0)
    {
      logit(LOG_COMM, "Write to socket error");
      return (-1);
    }
    sofar += thisround;
  }
  while (sofar < total);

  return (0);
}

int compress_get_ratio(P_desc player)
{
  if (!player->z_str)
    return 0;

  if (player->z_str->total_in == 0 || player->z_str->total_out == 0)
    return 0;

  return (player->z_str->total_in -
          player->z_str->total_out) * 100 / player->z_str->total_in;
}
