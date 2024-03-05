// vim: smarttab tabstop=8 shiftwidth=2 expandtab
/********************************************************/
/*							*/
/*	Low level subroutine declaration	        */
/*	handle tape contents                            */
/*							*/
/********************************************************/
#ifndef	UNITAP
#define UNITAP

#include        <time.h>
#include        <uuid/uuid.h>

#include        "subrou.h"

#define LABESIZ 30              //label maximun size
#define MEDSIZE 15              //media name maximun size

//Tape header structure
typedef struct  {
        char    id[3][LABESIZ]; //tape ID (current, previous, next)
        char    media[MEDSIZE]; //tape media type
        int     numrot;         //tape number of rotation (aging)
        time_t  stamp;          //backup stamp date
        time_t  tobkept;        //Backup retention date
        char    vers[10];       //backd version used to do backup
        char    device[60];     //device used by tape
        u_int   numsegment;     //number of backup segment
        u_int   mediasize;      //tape size in megabytes
        ul64    lastblk;        //last position on the tape
        ulong   lastused;       //date of tape last used
        u_int   frozen;         //how long the tape must kept
        u_int   cycled;         //number of time tape was cycled
        pid_t   pidlock;        //Tape is lock by backd daemon
        }TAPTYP;

//free memory used by a tap structure
extern TAPTYP *tap_freetape(TAPTYP *data);

//prepare a tape structure
extern TAPTYP *tap_newtape();

//duplcate tape information 
extern TAPTYP *tap_duplicate(TAPTYP *data);

//convert a tap structure to a string
extern char *tap_tapetostr(TAPTYP *tape);

//convert a string to a tape structure
extern TAPTYP *tap_strtotape(const char *data);

//convert a tap header string to a tape structure
extern TAPTYP *tap_headertotape(void *header);

//convert a tap structure to a block area
extern char *tap_tapetoheader(TAPTYP *data,ul64 blksize);

//check if tape is available to do backup
extern TAPTYP *tap_findavail(const char *media,TAPTYP *nominee,TAPTYP *challenger);

//homework done by module before starting to use it
extern int tap_openunitap();

//homework dto be done by module before exiting
extern int tap_closeunitap();

#endif
