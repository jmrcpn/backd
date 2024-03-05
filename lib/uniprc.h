// vim: smarttab tabstop=8 shiftwidth=2 expandtab
/************************************************/
/*						*/
/*	Unit level process management           */
/*      declaration                             */
/*						*/
/************************************************/
#ifndef	UNIPRC
#define UNIPRC

#include        <stdio.h>

#define LCK_UNLOCK   0	/*unlocking request	*/
#define	LCK_LOCK     1	/*locking requets	*/

//to open a pipe with a specifi pipe command
extern FILE *prc_openpipe(char *pipecmd,char *type);

//to close a previously open pip
extern char *prc_closepipe(FILE *file,char *pipecmd);

//To allow application to core dump is memory is
//big trouble need to be investigated
extern void prc_allow_core_dump();

//To do an on purpose application memory core dump
//with an explication message
extern void prc_core_dump(const char *fmt,...);

//routine to check if a proces is still up and running
extern int prc_checkprocess(pid_t pidnumber);

//lock application (to avoid running multiple daemon)
extern int prc_locking(const char *lockname,int lock,int tentative);

//procedure to put application in deamon mode
extern pid_t prc_divedivedive();

//homework done by module before starting to use it
extern int prc_openuniprc();

//homework dto be done by module before exiting
extern int prc_closeuniprc();

#endif
