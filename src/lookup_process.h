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
#define MSG_IDENT_REQ 3
#define MSG_IDENT_ANS 4

int run_lookup_host_process(int queue_id);
int run_lookup_ident_process(int queue_id);

void  dnsdb_insert(char *key, char *host);
char *dnsdb_find(char *key);

struct host_request
{
	long               mtype;
	sh_int             desc;
	char               addr[64];
	sockaddr_in6       sock;
	char               padding[4];
};

struct host_answer
{
	long   mtype;
	sh_int desc;
	char   addr[64];
	char   name[MAX_HOSTNAME];
	char   padding[4];
};

struct ident_request
{
	long               mtype;
	sh_int             desc;
	struct sockaddr_in laddr;
	struct sockaddr_in raddr;
	time_t             stamp;
	char               padding[4];
};

int MakeIdentReq(int qid, int desc, time_t stamp);

struct ident_answer
{
	long   mtype;
	sh_int desc;
	time_t stamp;
	char   name[10];
	char   padding[4];
};

#endif /* __LOOKUP_PROCESS_H */
