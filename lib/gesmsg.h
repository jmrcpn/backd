// vim: smarttab tabstop=8 shiftwidth=2 expandtab
/************************************************/
/*						*/
/*	management level, take care of all      */
/*      message to be send via:                 */
/*              email                           */
/*              logs                            */
/*              console                         */
/*						*/
/************************************************/
#ifndef	GESMSG
#define GESMSG

#include        "unidev.h"
#include        "unitap.h"
#include        "unisch.h"

typedef enum    {
    msg_ok,             //Backup successfully done
    msg_insert,         //Insert tape
    msg_tapeready,      //Tape found within device
    msg_notape,         //no tape available for backup
    msg_unknown         //unexpected  message (bug!)
    }MSGENU;

//manage a message dispatching to email/log/console
extern void msg_sendmsg(MSGENU msg,SCHTYP *schedule,TAPTYP *tape,DEVTYP *device);

//homework done by module before starting to use it
extern int msg_opengesmsg();

//homework to be done by module before exiting
extern int msg_closegesmsg();

#endif
