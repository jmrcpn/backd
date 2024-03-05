// vim: smarttab tabstop=8 shiftwidth=2 expandtab
/********************************************************/
/*							*/
/*	Low level subroutine declaration	        */
/*	to hanle an argv list and extract	        */
/*	parameters.				        */
/*							*/
/********************************************************/
#ifndef	UNIPAR
#define UNIPAR

#include        <uuid/uuid.h>

#include        "subrou.h"

//list of backup program components
typedef enum {			//application type
	wha_backd,		//'backd', the application main daemon
	wha_marker,		//'backd-marker' to mark the TAPE
	wha_reader,		//'backd-reader' To read part of tape
                                //(to read backup data segment)
	wha_scanner,		//'backd-scanner' To scan all device 
                                // looking for tapes
	wha_unknown		//default value (trouble)
	}WHATTYPE;

//list of time action
typedef enum {			//timer value
        act_standard,           //backup at standard time
        act_quick,              //backup is 10 minute after start
        act_atonce,             //backup is atonce if tape available
        act_never               //unknow delay
        }ACTTYP;

//structure of argument
typedef struct	{
	char *device;		//The (tape) device 
        char *media;            //the tape media type
        uuid_t uuid;            //session unique ID
        u_int mediasize;        //tape media size in Megabytes
        ACTTYP delay;           //start backup delay type
        int extended;           //extended report
	int argc;		//number of main argument
	char **argv;	        //list of argument
	}ARGTYP;

//free allocated memory used by a ARGTYP structure
extern ARGTYP *par_freeparams(ARGTYP *params);

//allocated memory and parse an argment list
extern ARGTYP *par_getparams(int argc,char * const argv[],const char *optstring);

//find out about the program name
extern WHATTYPE par_whatami(char *backdname);

//homework done by module before starting to use it
extern int par_openunipar();

//homework dto be done by module before exiting
extern int par_closeunipar();

#endif
