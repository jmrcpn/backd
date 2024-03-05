// vim: smarttab tabstop=8 shiftwidth=2 expandtab
/************************************************/
/*						*/
/*	Unit level signal management            */
/*      declaration                             */
/*						*/
/************************************************/
#ifndef	UNISIG
#define UNISIG

#include        <signal.h>

typedef void (*sighandler_t)(int);


extern  int hangup;       //Hangup signal received
extern  int reload;       //reload configuration signal received
extern  int atonce;      //override scheduled backup time and
                          //do it at once

//trapping segmentation violation signal
extern void sig_trapsegv(int onoff,sighandler_t trap);

//trapping application signal (SIGTERM, SIGHUP, etc...
extern void sig_trapsignal(int onoff,sighandler_t trap);

//homework done by module before starting to use it
extern int sig_openunisig();

//homework dto be done by module before exiting
extern int sig_closeunisig();

#endif
