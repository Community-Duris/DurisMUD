#ifndef __LOOKUP_PROCESS_H
#define __LOOKUP_PROCESS_H

#ifdef __CYGWIN32__
#include <cygwin/ipc.h>
#include <cygwin/msg.h>
#else
#include <sys/ipc.h>
#include <sys/msg.h>
#endif
#include <sys/types.h>

#define MSG_HOST_REQ  1
#define MSG_HOST_ANS  2

int run_lookup_host_process(int queue_id);

void  dnsdb_insert(char *key, char *host);
char *dnsdb_find(char *key);

struct host_request
{
	long               mtype;
	sh_int             desc;
	char               addr[24];
	struct sockaddr_in sock;
	char               padding[4];
};

struct host_answer
{
	long   mtype;
	sh_int desc;
	char   addr[24];
	char   name[MAX_HOSTNAME];
	char   padding[4];
};

#endif /* __LOOKUP_PROCESS_H */
