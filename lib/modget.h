// vim: smarttab tabstop=8 shiftwidth=2 expandtab
/************************************************/
/*						*/
/*	module level, take care of all          */
/*      backup extraction functions.            */
/*						*/
/************************************************/
#ifndef	MODGET
#define MODGET

//to retreive backup contents
extern int get_readdev(int argc, char *argv[]);

//homework done by module before starting to use it
extern int get_openmodget();

//homework to be done by module before exiting
extern int get_closemodget();

#endif
