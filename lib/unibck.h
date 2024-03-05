// vim: smarttab tabstop=8 shiftwidth=2 expandtab
/********************************************************/
/*							*/
/*	Low level subroutine declaration	        */
/*	to manage backup file list                      */
/*							*/
/********************************************************/
#ifndef	UNIBCK
#define UNIBCK

#include        <stdio.h>

#include        "subrou.h"

typedef enum  {
        rpt_header,             //header report
        rpt_data,               //data report
        rpt_footer,             //report footer
        rpt_unkwn               //unknown report type
        }RPTENU;

typedef struct  {
        RPTENU report;          //bckup type
        char *bckfile;          //backup directive file name
        char *level;            //backup level
        char *tapename;         //backup tape name
        int slot;               //tape slot
        ul64 cnumber;           //number of char in slot
        off_t blkstart;         //backup segment start position
        long blknum;            //backup segment size
        TIMESPEC duration;      //backup duration in nanosec
        char **included;        //list of included file
        char **excluded;        //list of excluded filename pattern
        }BCKTYP;

//procedure to display display "Byte size" in unit (as By, KB, MB, GB);
extern char *bck_bytesformat(char *strnumber,int maxsize,ul64 taille);

//Procedure to free memory used by a backup directive
//entry
extern BCKTYP *bck_freebck(BCKTYP *bck);

//Procedure to collect the list of file/directory to
//backup
extern BCKTYP *bck_getbcklist(const char *filename,char **levels);

//procedure to store a "search" result to a backup device
extern int bck_todevice(char *search,char *devname,int slot);

//homework done by module before starting to use it
extern int bck_openunibck();

//homework dto be done by module before exiting
extern int bck_closeunibck();

#endif
