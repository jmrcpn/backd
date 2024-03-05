// vim: smarttab tabstop=8 shiftwidth=2 expandtab
/************************************************/
/*						*/
/*	management level, take care of all      */
/*      tap (or alike backup tap) access.       */
/*	to hanle an argv list and extract	*/
/*	parameters.				*/
/*						*/
/************************************************/
#ifndef	GESTAP
#define GESTAP

#include        "subrou.h"
#include        "unipar.h"
#include        "unisch.h"
#include        "unitap.h"
#include        "unidev.h"

#define TAPES   CONFDIR"tapelist"

//write tape error status
typedef enum    {
        tap_ok,                 //everything fine
        tap_nodevice,           //device not available
        tap_wrongtapid,         //not expected tape ID
        tap_nolock,             //unable to get lock on device
        tap_trouble             //unexpected trouble/bug
      }TAPENUM;

//tapelist file  contents
typedef struct  {
      TAPTYP *tapedata;         //converted tape data (if needed)
      char *comment;            //comment part of the line
      }LSTTYP;

//init a tap structure
extern LSTTYP *tap_freeentry(LSTTYP *entry);

//duplicate a tap structure
extern LSTTYP *tap_dupentry(LSTTYP *entry);

//lock/unlock tapelist acces
extern int tap_locktapelist(const char *filename,int lock,int tentative);

//lock/unlock device acces
extern int tap_lockdevice(DEVTYP *device,int lock);

//procedure to add a tape entry within the tapeliste
extern LSTTYP **tap_addtolist(LSTTYP **list,TAPTYP *tape);

//init a tape structure
extern TAPTYP *tap_inittape(char *label,ARGTYP *params);

//procedure to write a new label on tape device
extern TAPENUM tap_initheader(TAPTYP *tape,u_int blksize);

//procedure to write a label on a tape device
extern int tap_writeheader(THANDLE *handle,TAPTYP *tape,u_int blksize);

//procedure to read a label on a tape device
extern TAPTYP *tap_readheader(THANDLE *handle,u_int blksize);

//procedure to build a tape list from a file content
extern LSTTYP **tap_readtapefile(const char *filename);

//read device and return true if a tape is availabel in device
extern int tap_in_device(DEVTYP *devref,TAPTYP *used);

//procedure to write/update a tapelist file from
//liste of tape entries
extern int tap_writetapefile(const char *filename,LSTTYP **list);

//procedure to return a tape structure looking for a label
extern const TAPTYP *tap_findtape(LSTTYP **list,const char *label);

//procedure to update the tapelist file with the used tape information
extern int tap_updatetape(const char *filename,TAPTYP *used);

//check if needed tape is avaliable for backup.
extern TAPTYP *tap_isatapeready(SCHTYP *sch);

//procedure to send to 'outfile' tape data segment
extern void tap_showdata(FILE *outfile,TAPTYP *tape,int segnum);

//procedure to display tape footer contents
extern void tap_showtapefooter(FILE *outfile,const char *tapelist,const char *tapename);

//procedure to list tape on all device (serving a specfic media).
extern void tap_listtapeonline(FILE *fileout,const char *media);

//homework done by module before starting to use it
extern int tap_opengestap();

//homework to be done by module before exiting
extern int tap_closegestap();

#endif
