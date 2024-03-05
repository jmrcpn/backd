// vim: smarttab tabstop=8 shiftwidth=2 expandtab
/********************************************************/
/*							*/
/*	Module for signal handling level                */
/*							*/
/********************************************************/
#include        <stdlib.h>

#include        "subrou.h"
#include        "unisig.h"

int hangup;             //Hangup signal received
int reload;             //reload configuration signal received
int atonce;            //override scheduled backup time and
                        //do it at once


static  int modopen;    //boolean module open/close

/*
^L
*/
/********************************************************/
/*                                                      */
/*	Procedure to set a signal handler in case of    */
/*      segmentation violation.                         */
/*                                                      */
/********************************************************/
void sig_trapsegv(int onoff,sighandler_t trap)

{
static int prvon=false;
static struct sigaction oldsa;

if (onoff==true) {
  struct sigaction newsa;

  if (prvon==true)      //in case we set signal twice
    (void) sigaction(SIGSEGV,&oldsa,(struct sigaction *)0);
  newsa.sa_flags=0;
  newsa.sa_handler=trap;
  (void) sigemptyset(&newsa.sa_mask);
  (void) sigaction(SIGSEGV,&newsa,&oldsa);
  }
else {
  (void) sigaction(SIGSEGV,&oldsa,(struct sigaction *)0);
  }
prvon=onoff;
}
/*
^L
*/
/********************************************************/
/*                                                      */
/*	Procedure to trap all meaningfull signal needed */
/*      bay application                                 */
/*                                                      */
/********************************************************/
void sig_trapsignal(int onoff,sighandler_t trap)

{
#define OPEP    "modbck.c:settrap"
#define NUMINTR	7

static struct sigaction *olds[NUMINTR];
static int alldone=false;

if (onoff==alldone) {
  switch (onoff) {
    case true	:
      (void) rou_crash("%s signal trap already set (Bug?)",OPEP);
      break;
    case false	:
      (void) rou_crash("%s signal trap already UNset (Bug?)",OPEP);
      break;
    default	:
      (void) rou_crash("%s unproper settrap value (very bad Bug!)",OPEP);
      break;
    } 
  }
if (onoff==true) {
  struct sigaction *newsa;
  int i;

  newsa=(struct sigaction *)calloc(1,sizeof(struct sigaction));
  newsa->sa_flags=0;
  newsa->sa_handler=trap;
  for (i=0;i<NUMINTR;i++) {
    if (olds[i]==(struct sigaction *)0) 
      olds[i]=calloc(1,sizeof(struct sigaction));
    }
  (void) sigaction(SIGUSR2,newsa,olds[0]);
  (void) sigaction(SIGUSR1,newsa,olds[1]);
  (void) sigaction(SIGINT,newsa,olds[2]);
  (void) sigaction(SIGTERM,newsa,olds[3]);
  (void) sigaction(SIGQUIT,newsa,olds[4]);
  (void) sigaction(SIGHUP,newsa,olds[5]);
  (void) sigaction(SIGALRM,newsa,olds[6]);
  (void) free(newsa);
  }
else {
  int i;

  (void) sigaction(SIGALRM,olds[6],(struct sigaction *)0);
  (void) sigaction(SIGHUP,olds[5],(struct sigaction *)0);
  (void) sigaction(SIGQUIT,olds[4],(struct sigaction *)0);
  (void) sigaction(SIGTERM,olds[3],(struct sigaction *)0);
  (void) sigaction(SIGINT,olds[2],(struct sigaction *)0);
  (void) sigaction(SIGUSR1,olds[1],(struct sigaction *)0);
  (void) sigaction(SIGUSR2,olds[0],(struct sigaction *)0);
  for (i=0;i<NUMINTR;i++) {
    if (olds[i]!=(struct sigaction *)0) 
      (void) free(olds[i]);
    olds[i]=(struct sigaction *)0;
    }
  }
alldone=onoff;
#undef  OPEP
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
int sig_openunisig()

{
int status;

status=0;
if (modopen==false) {
  (void) rou_opensubrou();
  reload=false;
  hangup=false;
  atonce=false;
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
int sig_closeunisig()

{
int status;

status=0;
if (modopen==true) {
  (void) rou_closesubrou();
  reload=false;
  hangup=false;
  modopen=false;
  }
return status;
}
