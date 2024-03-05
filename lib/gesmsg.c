// vim: smarttab tabstop=8 shiftwidth=2 expandtab
/********************************************************/
/*							*/
/*	Module to manage message to be sent to the      */
/*      administrator.                                  */
/*							*/
/********************************************************/
#include        <stdlib.h>
#include        <string.h>

#include	"subrou.h"
#include	"gesmsg.h"

//directory where all the application shell scripts are
#define DIRSH   "/usr/lib/"APPNAME"/shell"
#define SENDMSG "sendmsg.sh"

static  int modopen;            //boolean module open/close
/*
^L
*/
/********************************************************/
/*                                                      */
/*	Procedure to send an acknoledge to the backup   */
/*      administrator                                   */
/*                                                      */
/********************************************************/
static void acknowledge(const char *msgtyp,const char *arguments)

{
#define OPEP    "gesmsg.c:acknoledge"

FILE *file;
char pathname[300];

(void) memset(pathname,'\000',sizeof(pathname));
(void) snprintf(pathname,sizeof(pathname)-1,
                        "%s/%s/%s \"%s\" %s %s",
                         rootdir,DIRSH,SENDMSG,
                         rootdir,msgtyp,arguments);
if ((file=popen(pathname,"w"))!=(FILE *)0) {
  (void) pclose(file);
  }
else
  (void) rou_alert(0,"%s Unable to open pip <%s> (error=<%s>)",
                      OPEP,pathname,strerror(errno));
#undef  OPEP
}
/*
^L
*/
/********************************************************/
/*                                                      */
/*	Procedure to compose and dispatch a message.    */
/*      this message can be send via email and store    */
/*      in log (according message type) or display on   */
/*      console (when working in fore ground mode       */
/*                                                      */
/********************************************************/
void msg_sendmsg(MSGENU msg,SCHTYP *schedule,TAPTYP *tape,DEVTYP *device)

{
const char *msgtyp;
char arguments[100];
char message[1000];

msgtyp=(const char *)0;
(void) memset(arguments,'\000',sizeof(arguments));
(void) memset(message,'\000',sizeof(message));
switch (msg) {
  case msg_ok           :
    msgtyp="M_OK";      //sendmsg.sh Backup OK
    (void) snprintf(arguments,sizeof(arguments)-1,
                    "'%s' '%s'",
                    tape->id[0],device->devname);
    (void) snprintf(message,sizeof(message)-1,
                    "Remove tape '%s' from device '%s'\n",
                    tape->id[0],device->devname);
    break;
  case msg_insert       :
    msgtyp="M_NOTAPE";  //sendmsg.sh not tape message
    (void) snprintf(arguments,sizeof(arguments)-1,
                    "'%s' '%s'",
                    tape->id[0],rou_getstrfulldate(schedule->start));
    (void) snprintf(message,sizeof(message)-1,
                    "Tape '%s' not yet available for backup\n"
                    "          Backup starting is scheduled at %s\n",
                    tape->id[0],rou_getstrfulldate(schedule->start));
    break;
  case msg_tapeready    :
    msgtyp="M_TAPEOK";  //sendmsg.sh tape ready message
    (void) snprintf(arguments,sizeof(arguments)-1,
                    "'%s' '%s' '%s'",
                    tape->id[0],rou_getstrfulldate(schedule->start),
                    device->devname);
    (void) snprintf(message,sizeof(message)-1,
                    "Tape '%s' is now available for backup in device '%s'\n"
                    "          Backup starting is scheduled at %s",
                    tape->id[0],device->devname,
                    rou_getstrfulldate(schedule->start));
    break;
  default               :
    (void) rou_alert(0,"%s, message type unexpected (bug!!)");
    break;
  }
if (strlen(arguments)>0) {
  if (foreground==true) {
    (void) rou_alert(0,"%s",message);
    }
  else {
    (void) acknowledge(msgtyp,arguments);
    }
  }
}
/*
^L
*/
/********************************************************/
/*                                                      */
/*	Procedure to "open" usager to module.           */
/*      homework purpose                                */
/*      return zero if everything right                 */
/*                                                      */
/********************************************************/
int msg_opengesmsg()

{
int status;

status=0;
if (modopen==false) {
  (void) rou_opensubrou();
  (void) dev_openunidev();
  (void) tap_openunitap();
  (void) sch_openunisch();
  modopen=true;
  }
return status;
}
/*
^L
*/
/********************************************************/
/*                                                      */
/*	Procedure to "close" usager to module.          */
/*      homework purpose                                */
/*      return zero if everything right                 */
/*                                                      */
/********************************************************/
int msg_closegesmsg()

{
int status;

status=0;
if (modopen==true) {
  (void) sch_closeunisch();
  (void) tap_closeunitap();
  (void) dev_closeunidev();
  (void) rou_closesubrou();
  modopen=false;
  }
return status;
}
