// vim: smarttab tabstop=8 shiftwidth=2 expandtab
/********************************************************/
/*							*/
/*	Module to manage tap                            */
/*							*/
/********************************************************/
#include        <stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include        <unistd.h>

#include	"subrou.h"
#include	"gestap.h"
#include	"modtap.h"

static  int modopen;            //boolean module open/close
/*

*/
/********************************************************/
/*							*/
/*	marker; this procedure stamp the tape with	*/
/*	a label.					*/
/*							*/
/*	Parameters list					*/
/*	[-r debug-root]					*/
/*	label						*/
/********************************************************/
int tap_marker(int argc,char *argv[])

{
#define OPEP    "modtap.c:tap_marker,"

#define TLLOCK "tapelist_lock"

//acceptable argument list
const char *arg="b:d:hm:r:s:";	

//local parameters
int status;
TAPTYP *tape;
DEVTYP **devlist;
LSTTYP **tapelist;
ARGTYP *params;
register int phase;
register int proceed;

status=0;
foreground=true;
tape=(TAPTYP *)0;
devlist=(DEVTYP **)0;
tapelist=(LSTTYP **)0;
params=(ARGTYP *)0;
phase=0;
proceed=true;
while (proceed==true) {
  switch (phase) {
    case 0	:	//extracting the parameters
      if ((params=par_getparams(argc,argv,arg))==(ARGTYP *)0) 
	phase=999;	//not going further
      break;
    case 1	:	//reading the device list
      if ((devlist=dev_readdevfile((char *)0))==(DEVTYP **)0) {
	(void) rou_alert(0,"%s Unable to load device list (config?)",OPEP);
	phase=999;	//not going further
        }
      break;
    case 2	:	//locking the tapelist access
      if (tap_locktapelist((char *)0,true,5)==false) {
	(void) rou_alert(0,"%s Unable to lock access to tapelist (busy?)", 
                            OPEP);
	phase=999;	//not going further
        }
      break;
    case 3	:	//Preparing tape structure
      if ((tape=tap_inittape("none",params))==(TAPTYP *)0) {
	(void) rou_alert(0,"%s Unable to assign tape structure (system Bug?)",
                            OPEP);
	phase=999;	//not going further
        }
      break;
    case 4	:	//getting the current tape list
      if ((tapelist=tap_readtapefile((char *)0))==(LSTTYP **)0) {
	(void) rou_alert(0,"%s Unable to get current tape liste (config missing?)",
                            OPEP);
        status=-1;
	phase=999;	//not going further
        }
      break;
    case 5	:	//everything right
      int i;
      TAPENUM togo;

      for (i=0;(i<params->argc)&&(status==0);i++) {
        const DEVTYP *found;
        char *ptr;

        if ((ptr=strchr(params->argv[i],','))==(char *)0) {
          (void) fprintf(stdout,"<%s> wrong format\n",
                                 params->argv[i]);
          continue;
          }
        *ptr='\000'; 
        (void) strncpy(tape->device,params->argv[i],sizeof(tape->device)-1);
        ptr++;
        (void) strcpy(params->argv[i],ptr);
        if (tap_findtape(tapelist,params->argv[i])!=(TAPTYP *)0) {
          (void) fprintf(stdout,"Tapeid <%s> already within tapelist\n",
                                 params->argv[i]);
          continue;
          }
        if ((found=dev_finddev(devlist,tape->media,tape->device))==(const DEVTYP *)0) {
          (void) fprintf(stdout,"Unable to find at least one entry for "
                                "\"%s\" and \"%s \" in tapedev file\n",
                                 tape->media,tape->device);
          continue;
          }
        (void) strncpy(tape->id[0],params->argv[i],sizeof(tape->id[0])-1);
        (void) fprintf(stdout,"Tapeid <%s> to be written on tape\n",tape->id[0]);
	togo=tap_initheader(tape,found->blksize);
        switch(togo) {
          case tap_ok           :       //everything fine, lets continue
            tapelist=tap_addtolist(tapelist,tape);
            (void) tap_writetapefile((const char *)0,tapelist);
            (void) fprintf(stdout,"Tape on unit <%s> is now set with label <%s>\n",
                                   tape->device,tape->id[0]);
            break;
          case tap_nodevice     :       //device missing
            (void) fprintf(stdout,"Unable to find device <%s>\n",tape->device);
            status=-1;
            break;
          default               :
            (void) rou_alert(0,"unable to write id <%s> on tape Unit <%s>",
                                tape->id[0],params->device);
            status=-1;
            break;
	  }
	}
      tape=tap_freetape(tape);
      tapelist=(LSTTYP **)rou_freelist((void **)tapelist,
                                       (freehandler_t)tap_freeentry);
      break;
    default	:	//exiting procedure
      (void) tap_locktapelist((char *)0,false,1);
      proceed=false;
      break;
    }
  phase++;
  }
devlist=(DEVTYP **)rou_freelist((void **)devlist,(freehandler_t)dev_freedev);
params=par_freeparams(params);
return status;
#undef  OPEP
}
/*
^L
*/
/********************************************************/
/*                                                      */
/*	Procedure to "open" to module usage.            */
/*      homework purpose                                */
/*      return zero if everything right                 */
/*                                                      */
/********************************************************/
int tap_openmodtap()

{
int status;

status=0;
if (modopen==false) {
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
int tap_closemodtap()

{
int status;

status=0;
if (modopen==true) {
  (void) tap_closegestap();
  modopen=false;
  }
return status;
}
