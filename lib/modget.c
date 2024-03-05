// vim: smarttab tabstop=8 shiftwidth=2 expandtab
/********************************************************/
/*							*/
/*	Module to recover data from tape                */
/*							*/
/********************************************************/
#include        <stdlib.h>
#include        <string.h>

#include	"subrou.h"
#include	"unidev.h"
#include	"uniprc.h"
#include	"unipar.h"
#include	"gestap.h"
#include	"modget.h"

static  int modopen=false;      //boolean module open/close
/*
^L
*/
/********************************************************/
/*                                                      */
/*	Procedure to recover backaup from tape          */
/*      tape found on devices.                          */
/*                                                      */
/********************************************************/
int recover(FILE *outfile,const char *tapelist,int argc, char *argv[])

{
int status;
const char *tapename;
LSTTYP **tlist;
TAPTYP *tape;
register int phase;
register int proceed;

status=-1;
tapename=argv[0];
tlist=(LSTTYP **)0;
tape=(TAPTYP *)0;
phase=0;
proceed=true;
while (proceed==true) {
  switch (phase) {
    case 0      :       //do we have a tapename
      if (tap_locktapelist(tapelist,LCK_LOCK,3)==false) {
        (void) rou_alert(0,"Unable to lock config 'tapelist' (other reader\?\?)");
        phase=999;
        }
      break;
    case 1      :       //reading the current tape list
      if ((tlist=tap_readtapefile(tapelist))==(LSTTYP **)0) {
        (void) rou_alert(0,"Unable to read config 'tapelist' (config\?\?)");
        phase=999;
        }
      break;
    case 2      :       //finding tape within tapelist
      LSTTYP **ptr;
  
      ptr=tlist;
      while (*ptr!=(LSTTYP *)0) {
        TAPTYP *loc;

        loc=(*ptr)->tapedata;
        if (loc!=(const TAPTYP *)0) {
          if ((strcmp(loc->id[0],tapename)==0)&&(loc->media!=(char *)0)) {
            tape=loc;
            break;
            }
          }
        ptr++;
        }
      if (tape==(const TAPTYP *)0) {
        (void) rou_alert(0,"Unable to find tape <%s> within "
                           "'tapelist' (config\?\?)",tapename);
        phase=999;
        }
      break;
    case 3      :       //tape found
      status=0;
      if (argc==1) {
        (void) tap_showdata(outfile,tape,0);
        (void) rou_alert(0,"");
        (void) rou_alert(0,"Caution! No tape segment specified, "
                           "displaying tape <%s> footer",tape->id[0]);
        }
      if (argc>1) 
        (void) tap_showdata(outfile,tape,atoi(argv[1]));
      (void) fflush(outfile);
      break;
    default     :
      (void) tap_locktapelist(tapelist,LCK_UNLOCK,1);
      tlist=(LSTTYP **)rou_freelist((void **)tlist,(freehandler_t)tap_freeentry);
      proceed=false;
      break;
    }
  phase++;
  }
return status;
}
/*
^L
*/
/********************************************************/
/*                                                      */
/*	Procedure to scan all devices and report       */
/*      tape found on devices.                          */
/*                                                      */
/********************************************************/
int get_readdev(int argc, char *argv[])

{
#define OPEP    "modget.c:sget_readdev"

//acceptable argument list
const char *arg="d:hr:";

int status;
ARGTYP *params;
int phase;
int proceed;

status=0;
params=(ARGTYP *)0;
foreground=true;
phase=0;
proceed=true;
while (proceed==true) {
  switch (phase) {
    case 0      :       //extracting parameters
      if ((params=par_getparams(argc,argv,arg))==(ARGTYP *)0) {
        phase=999;
        }
      break;
    case 1      :       //do we have at list a paramaters
      if (params->argv==0) {
        (void) rou_alert(0,"Tape name missing, Please specify a "
                           "tape name as first argument");
        phase=999;      //no need to go further
        }
      break;
    case 2      :       //reading device
      status=recover(stdout,(const char *)0,params->argc,params->argv);
      break;
    default     :
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
int get_openmodget()

{
int status;

status=0;
if (modopen==false) {
  (void) rou_opensubrou();
  (void) dev_openunidev();
  (void) par_openunipar();
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
int get_closemodget()

{
int status;

status=0;
if (modopen==true) {
  (void) tap_closegestap();
  (void) par_closeunipar();
  (void) dev_closeunidev();
  (void) rou_closesubrou();
  modopen=false;
  }
return status;
}
