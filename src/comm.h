/* ***************************************************************************
 *  file: comm.h , Communication module.                     Part of Duris *
 *  Usage: Prototypes for the common functions in comm.c                     *
 *************************************************************************** */

#ifndef _SOJ_COMM_H_
#define _SOJ_COMM_H_

#ifndef _SOJ_STRUCTS_H_
#include "structs.h"
#endif

#define TO_ROOM           0
#define TO_VICT           1
#define TO_NOTVICT        2
#define TO_CHAR           3
#define TO_VICTROOM       4
#define TO_NOTVICTROOM    5
#define TO_ZONE           6
#define TO_WORLD          7
#define ACT_IGNORE_ZCOORD 8
#define ACT_SILENCEABLE   16
#define ACT_TERSE         32
#define ACT_NOTTERSE      64
#define ACT_NOEOL         128
#define ACT_PRIVATE       256

#define SEND_TO_Q(messg, desc) write_to_q((messg), &(desc)->output, 1)

/* following was io.h, for asych I/O operations, not used currently.  If we
   ever use it, makes more sense for it to be here.  JAB */

extern long sentbytes;

/* Resolve a client address without blocking the select() game loop. */
void resolve_descriptor_hostname_async(const char *address, int descriptor);

#if 0
/*
 * io.h - Auxiliary support structure for file descriptors
 *
 * To provide better mechanism for asynchronosity for the
 * DikuMUD server, we CANNOT have the DikuMUD process
 * waiting for completion of all I/O read/write operations.
 * We need to provide the fd_set to be used by "select" and
 * provide a callback mechanism.
 *
 * Written by:	Andrew Choi
 * Date:	10/28/1991
 *
 */

/* Macros */

#define IO_MAX_IO_STRUCT 16
/* Max number of IOStruct that can be stored in the internal IO list 	*/
/* Note this number really depends on the number of available of 	*/
/* descriptors (see getdtablesize), and should NOT be set to a value	*/
/* larger than the descriptor table size.				*/

#define IO_MAX_CALLBACKS 1
/* Max number of callbacks in a single callback list.  Note that this	*/
/* constant CANNOT be changed because some package functions make	*/
/* the assumption that there is ONLY 1 callback per list.		*/
/* This is provided for future extension only.			*/

/* Types -- IOState */

/* IOState determines what fd_set bit of select to be set 	*/
/* For example, if IO_READ, this means file descriptor is 	*/
/* involved in read operation and one should do 		*/
/* FD_SET(&read_fds, file descriptor) ... 			*/
/* Note that these are bitmasks and can be bitwise OR together 	*/

enum _IOState {
  IO_READ = 1,
  IO_WRITE = 2,
  IO_EXCEPT = 4
};

typedef enum _IOState IOState;

/* Types -- IOCallback and IOCallbackList */

/* A callback is somewhat similiar to a "special procedure" 	*/
/* of Mobiles in MUD.  It gives user a way to specify his/her 	*/
/* functions when IO is detected to be ready 			*/

/* The return value of the Callback function is as followed: 	*/
/*								*/
/*	0 if OK and done with this IOstruct... iostruct will be */
/*	  removed as a result 					*/
/*	1 if OK but NOT done with this IOstruct.. iostruct will */
/*	  not be removed. 					*/
/*	-1 if error occurs, as a result, io_processFDS returns 	*/
/*	  error too.  iostruct will also be removed as a result */

typedef int (*IOCallback) (int fd, void *callback_data);

/* CallbackList is an array of callbacks.  The number 		*/
/* of actual valid callbacks is stored in field "io_size". 	*/
/* Note that elements io_callbacks[0 .. io_size - 1] are 	*/
/* always the valid elements. 					*/

struct _IOCallbackList {
  int io_size;			/* Number of callbacks */
  IOCallback io_callbacks[IO_MAX_CALLBACKS];	/* Actual callbacks */
};

typedef struct _IOCallbackList IOCallbackList;

/* Types -- IOStruct */

/* IOStruct is used to describe each outstanding IO object. 	*/
/* Each object basically has a callback function, so that 	*/
/* program can invoke callback when select indicates that 	*/
/* descriptor is ready for specified operation(s). 		*/

struct _IOStruct {
  int io_fd;			/* The actual descriptor */
  IOState io_state;		/* This determines what fd_set */

  IOCallbackList io_read;	/* Callback(s) for read */
  IOCallbackList io_write;	/* Callback(s) for write */
  IOCallbackList io_except;	/* Callback(s) for except */

  void *io_data;		/* Data to be passed to callbacks */
};

typedef struct _IOStruct IOStruct;

/* Extern Variables */

extern fd_set io_readfds;
extern fd_set io_writefds;
extern fd_set io_exceptfds;

/* These are the OR product of all IOStruct's.  These should */
/* be treated as READ ONLY. */

extern int io_error_size;
extern IOStruct *io_error_struct[IO_MAX_IO_STRUCT];

/* Basically for errors.  See io_processFDS for how these are */
/* used. */

/* Functions */

int io_init(void);

/*
  ** Precondition:  NONE
  **
  ** This function MUST be called first and only once before calling
  ** of any other library functions provided by this package.  This
  ** function initializes certain private fields used by this package.
  **
  ** All other library functions assume that this function has been
  ** called already.
  **
  ** Return 0 if OK, or -1 if error occurs in initialization.
  ** If error occurs, one must NOT use any package functions, as
  ** it will result in an inconsistent (and dangerous) state.
  */

int io_addNewIOStruct(int fd, IOState, IOCallbackList[], void *data);

/*
  ** Precondition:  "fd" is a valid file descriptor opened for
  **		    legal operations according to IOState.
  **
  ** This function adds a new IOStruct created for "fd" to an internal
  ** list.  Extern variables io_*fds will be adjusted accordingly as
  ** to reflect the change.
  **
  ** "data" will be passed to the callback function directly.  This
  ** is to provide user's defined callback function some degree of
  ** freedom.
  **
  ** Callback list[0] is for the io_read callback
  ** Callback list[1] is for the io_write callback
  ** Callback list[2] is for the io_except callback
  **
  ** User only needs to init the field of callback which will be used,
  ** for example, callback list [0] can contain garbage if state is
  ** IO_WRITE.
  **
  ** Function returns 0 if OK, or -1 if error occurs.
  */

int io_processFDS(fd_set * rfds, fd_set * wfds, fd_set * efds);

/*
  ** Precondition:  *fds is the result of a successful call to
  **		    "select" system call.
  **
  ** This function calls the callbacks of all descriptors which
  ** have ready IO status.  Read is done before Write, and Write
  ** is done before Exceptional.
  **
  ** Note that depending on the value returned by the callbacks,
  ** the corresponding IOStruct may or may not be removed from
  ** list of callbacks.  If removal occurs, io_*fds will be updated
  ** accordingly.
  **
  ** Function returns 0 if all callbacks called return 0 or 1, and
  ** returns -1 if any 1 of callbacks return -1.  Note that
  ** all callbacks are processed (even if some encounters error),
  ** and then the returned status is determined.
  **
  ** If a callback returns 0 or -1, the corresponding file descriptor
  ** will be closed automatically, and the space for that particular
  ** IOStruct will be destroyed.
  **
  ** In case of error, io_error_size contains the number of IOStruct
  ** which return errors, and io_error_struct[0 .. io_error_size - 1]
  ** points to the IOStructs which encountered errors.
  **
  ** Note that these 2 variables are set correctly ONLY after an
  ** error occurs.  They should not be examined if this function
  ** returns 0.
  */

#endif /* 0 */

/* Escapes all '$' characters in player-controlled text to '$$' so that
 * act() interprets them as literal '$' rather than $-directives ($n, $p,
 * $O, etc.).  Prevents type-confusion crashes where a player types e.g.
 * "say $O" and act() casts the vict_obj character pointer to P_obj. */
void escape_act_dollars(char *dst, size_t dst_size, const char *src);

#endif /* _SOJ_COMM_H_ */
