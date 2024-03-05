// vim: smarttab tabstop=8 shiftwidth=2 expandtab
/************************************************/
/*						*/
/*	module level, take care of all          */
/*      tap (or alike backup tap) access.       */
/*						*/
/************************************************/
#ifndef	MODTAP
#define MODTAP

//marking a tape according argiments.
extern int tap_marker(int argc,char *argv[]);

//homework done by module before starting to use it
extern int tap_openmodtap();

//homework to be done by module before exiting
extern int tap_closemodtap();

#endif
