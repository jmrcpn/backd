// vim: smarttab tabstop=8 shiftwidth=2 expandtab
/************************************************/
/*						*/
/*	module level, take care of all          */
/*      backup daemon function.                 */
/*						*/
/************************************************/
#ifndef	MODBCK
#define MODBCK

//the backd daemon starting procedure
extern int bck_daemon(char *name,int argc,char *argv[]);

//homework done by module before starting to use it
extern int bck_openmodbck();

//homework to be done by module before exiting
extern int bck_closemodbck();

#endif
