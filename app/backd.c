// vim: smarttab tabstop=8 shiftwidth=2 expandtab
/********************************************************/
/*							*/
/*	Daemon which handle backup			*/
/*							*/
/********************************************************/
#include	<stdlib.h>
#include	<string.h>
#include	<syslog.h>
#include	<uuid/uuid.h>

#include	"subrou.h"
#include	"unipar.h"
#include	"modbck.h"
#include	"modget.h"
#include	"modsca.h"
#include	"modtap.h"

#define	BLKSIZE		32768

#define	SENDMSG		"sendmsg.sh"

#define	SIGPROBE	0	//send signal 0 to a process

typedef	enum {		/*status type			*/
	s_ok,		/*backup successfully terminated*/
	s_notape,	/*no tape avaliable		*/
	s_notinserted,	/*tape not in tape reader.	*/
	s_tapein,	/*tape ready and inserted	*/
	s_noslot,	/*no slot found on tape		*/
	s_toobig,	/*data volume too big for tape	*/
	s_wrongtape,	/*tape ready and inserted	*/
	s_unknown	/*unknown error			*/
	}STATYPE;
/*

*/
/********************************************************/
/*							*/
/*	Main routine					*/
/*		dispatch action according program name	*/
/*							*/
/********************************************************/
int main(int argc,char *argv[])

{
int status;
char *ptr;
char *name;

status=0;
(void) rou_opensubrou();
(void) par_openunipar();
if ((ptr=strrchr(argv[0],'/'))!=(char *)0) 
   ptr++;
else
   ptr=argv[0];
name=strdup(ptr);
(void) openlog(name,LOG_PID,LOG_USER);
switch (par_whatami(name)) {
  case wha_backd	:		//backd daemon
    (void) bck_openmodbck();
    status=bck_daemon(name,argc,argv);
    (void) bck_closemodbck();
    break;
  case wha_marker	:		//mark the tap with a label
    (void) tap_openmodtap();
    status=tap_marker(argc,argv);
    (void) tap_closemodtap();
    break;
  case wha_reader	:		//read/extract backup from device
    (void) get_openmodget();
    status=get_readdev(argc,argv);
    (void) get_closemodget();
    break;
  case wha_scanner	:		//scan device listing tape
    (void) sca_openmodsca();
    status=sca_scandev(argc,argv);
    (void) sca_closemodsca();
    break;
  default		:
    foreground=true;
    (void) rou_alert(0,"'%s' program name is unexpected (bug?)",name);
    break;
  }
(void) closelog();
if (status!=0) {
  (void) rou_alert(0,"the '%s' command was not successfull "
                     "(error code='%d')\n",name,status);
  }
(void) free(name);
(void) par_closeunipar();
(void) rou_closesubrou();
(void) exit(status);
}
