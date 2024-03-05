// vim: smarttab tabstop=8 shiftwidth=2 expandtab
/********************************************************/
/*							*/
/*	Low level subroutine declaration	        */
/*	handle schedule file contents                   */
/*							*/
/********************************************************/
#ifndef	UNISCH
#define UNISCH

#include        <time.h>

//Tape header structure
typedef struct  {
        char    *configname;    //configuration filename
        char    *frequency;     //cron style backup frequency definition
        char    *media;         //Media to be used in backup
        int      days;          //number of days to kept media
        time_t   start;         //time to start backup
        char    **levels;       //backup levels to consider
        }SCHTYP;

//procedure to free memory used by scheduler entry
extern SCHTYP *sch_freesch(SCHTYP *sch);

//procedure to select the next schedule according date
extern SCHTYP *sch_nextsch(SCHTYP **todolist);

//procedure to scan a file and extract scheduling information
extern SCHTYP *sch_filetosch(char *filename);

//homework done by module before starting to use it
extern int sch_openunisch();

//homework dto be done by module before exiting
extern int sch_closeunisch();

#endif
