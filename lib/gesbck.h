// vim: smarttab tabstop=8 shiftwidth=2 expandtab
/************************************************/
/*						*/
/*	management level, take care of all      */
/*      action to store data on a specific      */
/*      device.                                 */
/*	to hanle an argv list and extract	*/
/*	parameters.				*/
/*						*/
/************************************************/
#ifndef	GESBCK
#define GESBCK

#include        <stdio.h>

#include        "unibck.h"
#include        "unidev.h"
#include	"unitap.h"

//procedure to summarize backup done on tape
extern BCKTYP *bck_dolist(int toadd,THANDLE *handle,TAPTYP *tape,BCKTYP **tobackup);

//procedure to store a list of file to be backuped
//to a device
extern int bck_dobackup(TAPTYP *tape,THANDLE *dev,BCKTYP **tobackup);

//homework done by module before starting to use it
extern int bck_opengesbck();

//homework to be done by module before exiting
extern int bck_closegesbck();

#endif
