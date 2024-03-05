// vim: smarttab tabstop=8 shiftwidth=2 expandtab
/************************************************/
/*						*/
/*	module level, take care of all          */
/*      backup scan functions.                  */
/*						*/
/************************************************/
#ifndef	MODSCA
#define MODSCA

//to scan device and report found tape
extern int sca_scandev(int argc, char *argv[]);

//homework done by module before starting to use it
extern int sca_openmodsca();

//homework to be done by module before exiting
extern int sca_closemodsca();

#endif
