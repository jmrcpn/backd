// vim: smarttab tabstop=8 shiftwidth=2 expandtab
/************************************************/
/*						*/
/*	management level, take care of all      */
/*      configuration file parsing.             */
/*						*/
/************************************************/
#ifndef	GESFIL
#define GESFIL

#include        "unibck.h"
#include        "unisch.h"

//procedure 
//procedure to get all available scheduler definition
//within the configuration directory
extern SCHTYP **fil_getschedlist(char *confdir);

//establish a list of files to included on the
//scheduled backup
extern BCKTYP **fil_getbcklist(const char *bckdir,SCHTYP *sch,char *tapeid);

//homework done by module before starting to use it
extern int fil_opengesfil();

//homework to be done by module before exiting
extern int fil_closegesfil();

#endif
