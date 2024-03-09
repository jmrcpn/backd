// vim: smarttab tabstop=8 shiftwidth=2 expandtab
/********************************************************/
/*							*/
/*	Module to manage tap                            */
/*							*/
/********************************************************/
#include        <fcntl.h>
#include        <sys/wait.h>
#include        <stdarg.h>
#include        <stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include        <signal.h>
#include        <syslog.h>
#include        <unistd.h>

#include	"subrou.h"
#include	"unibck.h"
#include	"unidev.h"
#include	"unipar.h"
#include	"uniprc.h"
#include	"unisig.h"
#include	"gesbck.h"
#include	"gesfil.h"
#include	"gestap.h"
#include	"gesmsg.h"
#include	"modbck.h"

static  int modopen=false;      //boolean module open/close
/*

*/
/********************************************************/
/*							*/
/*	Sub routine to trap signal event		*/
/*							*/
/********************************************************/
static void sigalrm(int sig)

{
#define OPEP    "modback.c:sigalrm"

if (sig!=SIGALRM)
  (void) rou_alert(9,"%s, signal <%s> received",
                      OPEP,strsignal(sig));
switch (sig)
  {
  case SIGCHLD		:
    while (waitpid(-1,(int *)0,WNOHANG)>0);
    break;
  case SIGQUIT		:
  case SIGTERM		:
    hangup=true;
    break;
  case SIGINT		:
  case SIGHUP		:
    reload=true;
    break;
  case SIGALRM		:
    atonce=true;
    break;
  case SIGUSR1		:
    debug++;
    if (debug>10)
      debug=10;
    (void) rou_alert(0,"%s deamon, new increased debug level now set to '%d'",
                        APPNAME,debug);
    break;
  case SIGUSR2		:
    debug--;
    if (debug<0)
      debug=0;
    (void) rou_alert(0,"%s deamon, new decreased debug level now set to '%d'",
                        APPNAME,debug);
    break;
  default        	:
    (void) rou_alert(0,"Unexpected Signal [%d]/<%s> received",
                        sig,strsignal(sig));
    break;
  }
(void) signal(sig,sigalrm);
#undef  OPEP
}
/*

*/
/********************************************************/
/*						        */
/*	Procedure to prepare a core_dump in	        */
/*	the right directory.			        */
/*						        */
/********************************************************/
static void core_dump(char *fmt,...)

{
va_list args;

va_start(args,fmt);
(void) prc_core_dump(fmt,args);
va_end(args);
}
/*

*/
/********************************************************/
/*						        */
/*	Procedure to catch signal and do what is        */
/*	needed.					        */
/*						        */
/********************************************************/
static void trpmempbls(int sig)

{
#define	OPEP	"modbck.c:trpmempbls,"

switch (sig) {
  case SIGSEGV   :
    (void) core_dump("Genuine memory violation (Bug?)");
    break;
  default        :
    (void) core_dump("%s Unexpected signal <%s> received (BUG!)",
		      OPEP,strsignal(sig));
    break;
  }
#undef  OPEP
}
/*

*/
/********************************************************/
/*						        */
/*	procedure to set/unset trapped signal	        */
/*						        */
/********************************************************/
static void settrap(int set)

{
(void) prc_allow_core_dump();
(void) sig_trapsegv(set,trpmempbls);
(void) sig_trapsignal(set,sigalrm);
}
/*
^L
*/
/********************************************************/
/*                                                      */
/*	procedure to select device used to read/write   */
/*      tape.                                           */
/*                                                      */
/********************************************************/
static DEVTYP **get_devices(TAPTYP *used)

{
DEVTYP **gooddev;
DEVTYP **devs;
int phase;
int proceed;

gooddev=(DEVTYP **)0;
devs=(DEVTYP **)0;
phase=0;
proceed=true;
while (proceed==true) {
  switch (phase) {
    case 0      :       //loadint the configuration device list
      if ((devs=dev_readdevfile((const char *)0))==(DEVTYP **)0) {
        (void) rou_alert(0,"Unable to read tapedev file (config\?\?)");
        phase=999;      //trouble trouble
        }
      break;
    case 1      :       //selecting the all devices alling the media
      DEVTYP **ptr;

      ptr=devs;
      while (*ptr!=(DEVTYP *)0) {
        if (strcmp(used->media,(*ptr)->media)==0) {
          gooddev=(DEVTYP **)rou_addlist((void **)gooddev,(void *)dev_dupdev(*ptr));
          }
        ptr++;
        }
      break;
    case 2      :       //freeing the devlist
      devs=(DEVTYP **)rou_freelist((void **)devs,(freehandler_t)dev_freedev);
      break;
    default     :
      proceed=false;
      break;
    }
  phase++;
  }
return gooddev;
}
/*
^L
*/
/********************************************************/
/*                                                      */
/*	procedure to check if selected tape is inserted */
/*      in device.                                      */
/*                                                      */
/********************************************************/
static DEVTYP *is_tape_inserted(SCHTYP *sch,TAPTYP *used)

{
#define REPEAT  3       //maximun delay in minute to repeate message

time_t lastcall;
DEVTYP **gooddev;
DEVTYP *ready;
int phase;
int proceed;

lastcall=(time_t)0;
gooddev=(DEVTYP **)0;
ready=(DEVTYP *)0;
atonce=false;          //removing received previous "atonce" signal
reload=false;
hangup=false;
phase=0;
proceed=true;
while (proceed==true) {
  switch (phase) {
    case 0      :       //get device list
      if ((gooddev=get_devices(used))==(DEVTYP **)0) 
        phase=999;      //trouble trouble
      break;
    case 1      :       //chek if the tape is still in device
      if (ready!=(DEVTYP *)0) {
        if (tap_in_device(ready,used)==false) {
          (void) tap_lockdevice(ready,LCK_UNLOCK);
          ready=(DEVTYP *)0;    //tape was removed?
          }
        else
          phase++;              //no need to search tape
        }
      break;
    case 2      :       //searching for tape in device list
      DEVTYP **ptr;

      ptr=gooddev;
      while (*ptr!=(DEVTYP *)0) {
        if (tap_lockdevice(*ptr,LCK_LOCK)==true) {
          if (tap_in_device(*ptr,used)==true) {
            ready=*ptr;   //we found the tape
            (void) msg_sendmsg(msg_tapeready,sch,used,ready);
            break;        //no need to search further
            }
          (void) tap_lockdevice(*ptr,LCK_UNLOCK);
          }
        ptr++;
        }
      break;
    case 3      :       //check if it is the time to check tape
      time_t curtime;
      
      curtime=time((time_t)0); 
      if (curtime<(sch->start)) {
        int delay;

        if ((ready==(DEVTYP *)0)&&((curtime-lastcall)>(REPEAT*60))) {
          (void) msg_sendmsg(msg_insert,sch,used,(DEVTYP *)0);
          lastcall=curtime;
          }
        delay=(sch->start-curtime);
        if (delay>2)
          delay/=2;
        (void) rou_alert(1,"%s now in sleep mode (%d sec):",APPNAME,delay);
        if (delay>(24*60*60))    //delay is more than one day
          (void) rou_alert(1,"      will wake up at: %s",
                              rou_getstrfulldate(curtime+delay));
        else
          (void) rou_alert(1,"      will wake up shortly at: %s",
                              rou_getstrtime(curtime+delay));
        (void) sleep(delay);    //gently waiting for backup time
        phase=0;                //lets try again to check tape
        }
      else {
        phase=999;      //it is mow the backup scheduled time
        }
      break;
    default     :
      proceed=false;
      break;
    }
  if ((reload==true)||(hangup==true)) {
    if (ready!=(DEVTYP *)0) {
      (void) tap_lockdevice(ready,LCK_UNLOCK);
      ready=(DEVTYP *)0;
      }
    break;
    }
  if ((ready!=(DEVTYP *)0)&&(atonce==true)) {
    (void) rou_alert(0,"%s Received ALRM signal!, doing backup at once:",APPNAME);
    proceed=false;
    break;
    }
  phase++;
  }
ready=dev_dupdev(ready);
gooddev=(DEVTYP **)rou_freelist((void **)gooddev,(freehandler_t)dev_freedev);
atonce=false;
return ready;
#undef  REPEAT
}
/*
^L
*/
/********************************************************/
/*                                                      */
/*	procedure to do one backup                      */
/*      return a status false if not successfull, false */
/*      otherwise.                                      */
/*                                                      */
/********************************************************/
static int do_storage(SCHTYP *sch,TAPTYP *used,BCKTYP ***bcklist)


{
int status;
BCKTYP *resbck;
DEVTYP *ready;
THANDLE *handle;
int phase;
int proceed;

status=false;
resbck=(BCKTYP *)0;
handle=(THANDLE *)0;
ready=(DEVTYP *)0;
phase=0;
proceed=true;
while (proceed==true) {
  switch (phase) {
    case 0      :       //reporting backup start
      if ((sch==(SCHTYP *)0)||(used==(TAPTYP *)0)) {
        (void) rou_alert(0,"Cannot do backup; tape or scheduler "
                          "data are missing (Bad Bug!!)");
        phase=999;      //trouble trouble
        }
      break;
    case 1      :       //check if tape is inserted and time
      if ((ready=is_tape_inserted(sch,used))==(DEVTYP *)0) {
        (void) rou_alert(0,"tape <%s> not found inserted in a device in due time",
                          used->id[0]);
        phase=999;      //no backup possible
        }
      break;
    case 2      :       //opening tape device
      used->stamp=rou_systime();
      used->lastused=rou_normdate(time((time_t *)0));
      used->frozen=sch->days;   //reporting how log the tape must be freezed
      used->cycled++;
      if ((handle=dev_open(ready->devname,ready->blksize,O_RDWR))==(THANDLE *)0) {
        (void) rou_alert(0,"Unable to open tape device <%s> (error=<%s>)",
                            ready->devname,strerror(errno));
        phase=999;      //trouble trouble
        }
      break;
    case 3      :       //storing header
      TIMESPEC start;
      int numsegment;

      status=tap_ok;
      (void) rou_getchrono(&start,(TIMESPEC *)0);
      (void) rou_alert(0,"Backup now starting on tape <%s>",used->id[0]);
      numsegment=0;
      while ((*bcklist)[numsegment]!=(BCKTYP *)0)
        numsegment++;
      used->numsegment=numsegment;
      (**bcklist)->cnumber=tap_writeheader(handle,used,ready->blksize);
      (void) dev_setmark(handle);
      (void) rou_getchrono(&((*bcklist)[0]->duration),&start);
      if ((**bcklist)->cnumber<0) {
        status=tap_nodevice;
        (void) rou_alert(0,"Unable to write header on tape <%s> "
                           "(status='%d', aborting backup)",used->id[0],status);
        phase=999;      //trouble trouble
        }
      break;
    case 4      :       //doing backup itself
      if ((status=bck_dobackup(used,handle,*bcklist))!=true) 
        phase=999;      //backup not successfull
      break;
    case 5      :       //adding footer
      if ((resbck=bck_dolist(true,handle,used,*bcklist))==(BCKTYP *)0) {
        (void) rou_alert(0,"Unable to add footer list on tape <%s>  (aborting)",
                            used->id[0]);
        phase=999;      //backup not successfull
        }
      break;
    case 6      :       //reporting backup list
      *bcklist=(BCKTYP **)rou_addlist((void **)(*bcklist),(void *)resbck);
      if ((resbck=bck_dolist(false,handle,used,*bcklist))!=(BCKTYP *)0) {
        resbck=bck_freebck(resbck);     //very unlikely as back_dolist(false,...)
        }
      break;
    case 7      :       //unlock acces to device
      (void) tap_lockdevice(ready,LCK_UNLOCK);
      break;
    case 8      :       //backup successful
      (void) rou_alert(0,"Backup successfully completed and sending reports");
      (void) msg_sendmsg(msg_ok,sch,used,ready);
      break;
    default     :
      (void) dev_close(handle,false);
      proceed=false;
      break;
    }
  phase++;
  if ((reload==true)||(hangup==true)) {
    (void) rou_alert(0,"Backup on tape <%s> aborted by signal",used->id[0]);
    (void) tap_lockdevice(ready,LCK_UNLOCK);
    if (handle!=(THANDLE *)0) 
      (void) dev_close(handle,false);
    proceed=false;          //break, break, break...
    }
  }
ready=dev_freedev(ready);
return status;
}
/*
^L
*/
/********************************************************/
/*                                                      */
/*	procedure to do one backup                      */
/*                                                      */
/********************************************************/
static int dobackup(SCHTYP *sch,ACTTYP delay)


{
#define MINDELAY        5      //delay in minutes

int status;
TAPTYP *used;                   //selected tape reference
BCKTYP **bcklist;
int phase;
int proceed;

status=0;
used=(TAPTYP *)0;
bcklist=(BCKTYP **)0;
phase=0;
proceed=true;
while (proceed==true) {
  switch (phase) {
    case 0      :       //find an available tape
      (void) rou_alert(0,"Next backup time scheduled at <%s>",
                          rou_localedate(sch->start));
      if (delay!=act_standard) {
        sch->start=time((time_t)0)+(60*MINDELAY);
        (void) rou_alert(0,"Backup scheduled in quick mode (starting time <%s>)",
                          rou_getstrtime(sch->start));
        }
      break;
    case 1      :       //select lock the tape
      if ((used=tap_isatapeready(sch))==(TAPTYP *)0)        {
        (void) rou_alert(0,"Unable to do backup, (no tape available!)");
        (void) rou_alert(0,"--> Check configuration within the \"tapelist\" file");
        phase=999;      //no need to go further
        }
      break;
    case 2      :       //find list of backup
      if ((bcklist=fil_getbcklist((const char *)0,sch,used->id[0]))==(BCKTYP **)0) {
        (void) rou_alert(0,"List of file to be backup is empty!! (*.bck config?)");
        phase=999;      //no need to go further
        }
      break;
    case 3      :       //do we need to do backup atonce
      if (delay==act_atonce) {
        sch->start=time((time_t)0)+2;
        (void) rou_alert(0,"Backup scheduled AT ONCE (starting time <%s>)",
                          rou_getstrtime(sch->start));
        }
      break;
    case 4      :       //backup process by itself
      if ((status=do_storage(sch,used,&bcklist))==false) {
        (void) rou_alert(0,"Backup process started but was "
                           "not successfully completed");
        phase=999;      //no need to update tapelist file
        }
      break;
    case 5      :       //updating tapelist about the used tape
      break;
    default     :       //task termination
      if (used!=(TAPTYP *)0) {
        used->pidlock=(pid_t)0;
        if (tap_updatetape((const char *)0,used)<0) 
          (void) rou_alert(0,"Unable to set tape <%s> free (config?)",used->id[0]);
        }
      used=tap_freetape(used);
      bcklist=(BCKTYP **)rou_freelist((void **)bcklist,(freehandler_t)bck_freebck);
      proceed=false;
      break;
    }
  phase++;
  if ((hangup==true)||(reload==true)) 
    phase=999;          //break, break, break
  }
return status;
#undef  MINDELAY
}
/*
^L
*/
/********************************************************/
/*                                                      */
/*	Daemon core procedure, wait for ever for either */
/*      an action to execute or a signal.               */
/*      Procedure return 0 if backup daemon exited      */
/*      normally.                                       */
/*                                                      */
/********************************************************/
static int waitandsee(ACTTYP delay)

{
#define MAXDELAIS       (24*60*60)
int status;
time_t delais;
SCHTYP **todo;
register int phase;
register int proceed;

status=0;
delais=(time_t)60;
(void) settrap(true);
todo=(SCHTYP **)0;
phase=0;
proceed=true;
while (proceed==true) {
  switch (phase) {
    case 0      :       //empty phase
      break;
    case 1      :	//setting signal trap
      reload=false;
      (void) rou_alert(0,"Loading current Scheduler files");
      if ((todo=fil_getschedlist((char *)0))==(SCHTYP **)0) {
	(void) rou_alert(0,"Waiting for something to backup (%s)",
                           "schedule files missing from config?)");
        (void) sleep(60);
        phase--;                //looping to find something to do
        }
      break;
    case 2      :	        //do we have a backup to do
      if (todo==(SCHTYP **)0) {
        (void) rou_alert(0,"No backup to be scheduled (config\?\?)");
        (void) sleep(10);       //relaxe time
        phase=0;
        }
      break;
    case 3      :	//doing the backup itself
      (void) dobackup(sch_nextsch(todo),delay);
      break;
    case 4      :	//check signal situation
      todo=(SCHTYP **)rou_freelist((void **)todo,(freehandler_t)sch_freesch);
      if (reload==true)
        phase=0;        //backd daemon still up and running
      if (hangup==true) //signal TERM recieved
        phase=999;
      if (delay!=act_standard) //backup was a one shot deal
        phase=999;      //no other backup to do
      break;
    case 5      :       //Just relaxe little bit
      time_t curdate;

      (void) time(&curdate);
      curdate+=delais;
      (void) rou_alert(0,"Doing backup cleanup, waiting until <%s> "
                         "to check again about next backup",rou_getstrtime(curdate));
      (void) sleep(delais);
      delais*=2;
      if (delais>MAXDELAIS)
        delais=MAXDELAIS;
      phase=0;
      break;
    default	:	//exiting procedure
      proceed=false;
      break;
    }
  phase++;
  }
(void) settrap(false);
return status;
#undef  MAXDELAIS
}
/*
^L
*/
/********************************************************/
/*                                                      */
/*	Procedure to start an control the backup daemon.*/
/*      Procedure return 0 if backup daemon exited      */
/*      normally.                                       */
/*                                                      */
/********************************************************/
int bck_daemon(char *name,int argc,char *argv[])

{
#define OPEP    "modbck.c:bck_daemon,"

//acceptable argument list
const char *arg="fd:hnr:v";	

//local parameters
int status;
ARGTYP *params;
pid_t backdpid;
int phase;
int proceed;

status=0;
params=(ARGTYP *)0;
backdpid=(pid_t)0;
phase=0;
proceed=true;
(void) openlog(name,LOG_PID,LOG_DAEMON);
while (proceed==true) {
  switch (phase) {
    case 0	:	//extracting the parameters
      if ((params=par_getparams(argc,argv,arg))==(ARGTYP *)0) {
	phase=999;	//not going further
        }
      break;
    case 1	:	//starting deamon process
      if ((foreground==false)&&((backdpid=prc_divedivedive())!=0)) {
        status=0;
        phase=999;      //daemon started
        }
      break;
    case 2	:	//Set locking file
      if (prc_locking(name,true,5)==false) {
	(void) rou_alert(0,"Unable to lock <%s> (config problem?)",name);
        status=-1;
        phase=999;      //daemon aborting process
        }
      break;
    case 3	:	//The daemon process itself
      status=waitandsee(params->delay);
      if (hangup==true) 
        (void) rou_alert(0,"Signal Hangup recieved, exiting from daemon mode");
      else
        (void) rou_alert(0,"Exiting from daemon mode");
      break;
    case 4	:	//Exiting daemon
      (void) prc_locking(name,false,1);
      break;
    default	:	//exiting procedure
      proceed=false;
      break;
    }
  phase++;
  }
params=par_freeparams(params);
return status;
#undef  OPEP
}
/*
^L
*/
/********************************************************/
/*                                                      */
/*	Procedure to "open" module usage.               */
/*      homework purpose                                */
/*      return zero if everything right                 */
/*                                                      */
/********************************************************/
int bck_openmodbck()

{
int status;

status=0;
if (modopen==false) {
  (void) rou_opensubrou();
  (void) dev_openunidev();
  (void) par_openunipar();
  (void) prc_openuniprc();
  (void) sig_openunisig();
  (void) bck_opengesbck();
  (void) fil_opengesfil();
  (void) msg_opengesmsg();
  (void) tap_opengestap();
  modopen=true;
  }
return status;
}
/*
^L
*/
/********************************************************/
/*                                                      */
/*	Procedure to "close" to module usage.           */
/*      homework purpose                                */
/*      return zero if everything right                 */
/*                                                      */
/********************************************************/
int bck_closemodbck()

{
int status;

status=0;
if (modopen==true) {
  (void) tap_closegestap();
  (void) msg_closegesmsg();
  (void) fil_closegesfil();
  (void) bck_closegesbck();
  (void) sig_closeunisig();
  (void) par_closeunipar();
  (void) prc_closeuniprc();
  (void) dev_closeunidev();
  (void) rou_closesubrou();
  modopen=false;
  }
return status;
}
