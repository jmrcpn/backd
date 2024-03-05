// vim: smarttab tabstop=8 shiftwidth=2 expandtab
/********************************************************/
/*							*/
/*	Module to recover data from tape                */
/*							*/
/********************************************************/

#include	"subrou.h"
#include	"unidev.h"
#include	"unipar.h"
#include	"gestap.h"
#include	"modsca.h"

static  int modopen=false;      //boolean module open/close
/*
^L
*/
/********************************************************/
/*                                                      */
/*	Procedure to scan all devices and report       */
/*      tape found on devices.                          */
/*                                                      */
/********************************************************/
int sca_scandev(int argc, char *argv[])

{
#define OPEP    "modsca.c:sca_scandev"

//acceptable argument list
const char *arg="d:hr:x";

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
    case 1      :       //do we have media type
      if (params->argc==0) {
        const char *msg[]={
                "media type (DAT-72, etc...)",
                "tape label"
                };
        register int num;

        num=0;
        if (params->extended==true) 
          num=1;
        (void) rou_alert(0,"%s %s","Please specify (at least) one",msg[num]);
        status=-1;
        phase=999;
        }
      break;
    case 2      :       //scanning media 
      FILE *fileout;

      fileout=stdout;
      for (int i=0;i<params->argc;i++) {
        register char *name;

        name=params->argv[i];
        if (params->extended==true) {
          (void) fprintf(fileout,"\n\n");
          (void) tap_showtapefooter(fileout,(const char *)0,name);
          }
        else
          (void) tap_listtapeonline(fileout,name);
        }
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
int sca_openmodsca()

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
int sca_closemodsca()

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
