// vim: smarttab tabstop=8 shiftwidth=2 expandtab
/************************************************/
/*						*/
/*	Low level subroutine declaration	*/
/*						*/
/************************************************/
#ifndef	SUBROU
#define SUBROU

#include	<linux/types.h>
#include	<sys/types.h>
#include	<errno.h>
#include	<stdarg.h>
#include	<stdio.h>
#include	<time.h>

#define APPNAME "backd3"         //application main name

//application configuration directory
#define	CONFDIR	"/etc/"APPNAME"/"
#define	BACKDIR	CONFDIR"backd.d"

#define	BACKUP	"bcklst"        //backup definition dir
#define	SHELL	"shell"         //shell directory

/*defining boolean value			*/
typedef enum {false, true} bool;

//64 bit unsigned integer
typedef unsigned long long ul64;

//defining confivenient timespec
typedef struct timespec TIMESPEC;

extern int	debug;	//application debug mode
extern int foreground;	//process is in foreground mode

extern char  *rootdir;  //application root directory

//to return allocated memory duplicating a string
extern char *rou_strdup(char *ptr,const char *info);

//to compute an application path with the root directory
extern char *rou_apppath(const char *path);

//to add a not null pointer to a list of pointer
extern void **rou_addlist(void **list,void *entry);

//to fee a list of pointer
typedef void (*freehandler_t)(void *);
extern void **rou_freelist(void **list,freehandler_t handler);

//local system time (plus offset if needed for debug purpose)
extern time_t rou_systime();

//to display message on console (verbose mode) or
//via syslog (LOG_INFO) using variable argument list macros
void rou_valert(const int dlevel,const char *fmt,va_list ap);

//to display message on console (verbose mode) or
//via syslog (LOG_DAEMON)
extern void rou_alert(const int dlevel,const char *fmt,...);

//To do an on purpose crash the application with an
//explication message
extern void rou_crash(const char *fmt,...);

//return the current system time in string format
extern char *rou_getstrfulldate(time_t curdate);

//return current system time in AAAAMMJJ format
extern u_long rou_normdate(time_t timestamp);

//return a time as date ONLY
extern char *rou_getstrdate(time_t curdate);

//return a time as hour:minute:second format
extern char *rou_getstrtime(time_t curdate);

//return a timespec as hour:minute:second.ms format
extern char *rou_getstrchrono(TIMESPEC *highres);

//return a timespec value
extern TIMESPEC *rou_getchrono(TIMESPEC *chrono,TIMESPEC *start);

//add time between 2 chrono
extern TIMESPEC *rou_addchrono(TIMESPEC *chrono,TIMESPEC *increment);

//add month and days to a time (seen as a date)
extern u_long rou_caldate(u_long date,int month,int days);

//read an ascii file line, removeing line with comment
extern char *rou_getstr(FILE *fichier,char *str,int taille,int comment,char carcom);

//return the locale date (using time zone)
extern char *rou_localedate(time_t date);

//return program version
extern char *rou_getversion();

//homework done by module before starting to use it
extern int rou_opensubrou();

//homework dto be done by module before exiting
extern int rou_closesubrou();

#endif
