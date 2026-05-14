/*
 * ttype.h - terminal type (rfc 1091) and mtts support
 */

#ifndef _TTYPE_H_
#define _TTYPE_H_

#include "structs.h"

/* ttype negotiation states */
#define TTYPE_NONE     0
#define TTYPE_SENT_DO  1
#define TTYPE_CYCLING  2
#define TTYPE_COMPLETE 3

/* mtts bitvector flags (mud terminal type standard) */
#define MTTS_ANSI          1
#define MTTS_VT100         2
#define MTTS_UTF8          4
#define MTTS_256COLOR      8
#define MTTS_MOUSE         16
#define MTTS_OSC_PALETTE   32
#define MTTS_SCREEN_READER 64
#define MTTS_PROXY         128
#define MTTS_TRUECOLOR     256
#define MTTS_MNES          512

/* function prototypes */
void ttype_negotiate(P_desc d);
void ttype_handle_negotiation(P_desc d, int cmd);
void ttype_handle_subnegotiation(P_desc d, const unsigned char *data, int len);

/* helper macros */
#define HAS_UTF8(d)      ((d) && ((d)->mtts_flags & MTTS_UTF8))
#define HAS_256COLOR(d)  ((d) && ((d)->mtts_flags & MTTS_256COLOR))
#define HAS_TRUECOLOR(d) ((d) && ((d)->mtts_flags & MTTS_TRUECOLOR))
#define HAS_ANSI(d)      ((d) && ((d)->mtts_flags & MTTS_ANSI))

#endif /* _TTYPE_H_ */
