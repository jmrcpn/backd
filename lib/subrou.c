// vim: smarttab tabstop=8 shiftwidth=2 expandtab
/********************************************************/
/*							*/
/*	Module for low level subroutine			*/
/*							*/
/********************************************************/
#include	<sys/prctl.h>
#include	<sys/resource.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<math.h>
#include	<signal.h>
#include        <stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<syslog.h>
#include	<time.h>
#include	<unistd.h>

#include	"subrou.h"

//version definition 
#define VERSION "3.2"
#define RELEASE "12"

/*current debug level					*/
int debug=0;
/*process is started in foreground mode			*/
int foreground=false;

/*Time offset (debug purpose only                       */
time_t off64_time=(time_t)0;
time_t off_date=(time_t)0;

//application specific working directory (debugging)
char *rootdir=(char *)0;


static  int modopen;            //boolean module open/close
/*

*/
/********************************************************/
/*							*/
/*	Subroutine to store a string in another pointer */
/*							*/
/********************************************************/
char *rou_strdup(char *ptr,const char *info)

{
int needed;

needed=2;
if (info!=(const char *)0)
  needed+=strlen(info);
else
  info="";
if (ptr!=(char *)0)
  ptr=(char *)realloc(ptr,needed);
else
  ptr=(char *)calloc(1,needed);
(void) strcpy(ptr,info);
return ptr;
}
/*

*/
/********************************************************/
/*							*/
/*	procedure to allocate a path within the root    */
/*      directory if needed                             */
/*							*/
/********************************************************/
char *rou_apppath(const char *path)

{
char *root;
char *newpath;
int taille;
char loc[3];

root="";
newpath=(char *)0;
if (rootdir!=(char *)0)
  root=rootdir;
taille=snprintf(loc,sizeof(loc),"%s%s",root,path);
newpath=(char *)calloc(taille+3,sizeof(char));
(void) sprintf(newpath,"%s%s",root,path);
return newpath;
}
/*

*/
/********************************************************/
/*							*/
/*	Subroutine to get add a pointer (not null)      */
/*	to a list of pointer.                           */
/*							*/
/********************************************************/
void **rou_addlist(void **list,void *entry)

{
if (entry!=(void *)0) {
  register int num;

  num=0;
  if (list==(void  **)0)
    list=(void **)calloc(num+2,sizeof(void *));
  else {
    while (list[num]!=(void *)0) 
      num++;
    list=realloc((void *)list,(num+2)*sizeof(void *));
    }
  list[num]=entry;
  num++;
  list[num]=(void *)0;
  }
return list;
}
/*

*/
/********************************************************/
/*							*/
/*	Procedure to free memoy used by a list of       */
/*      pointer.                                        */
/*      Use a specific free procedure (according        */
/*      upstream pointer type)                          */
/*							*/
/********************************************************/
void **rou_freelist(void **list,freehandler_t handler)

{
if ((list!=(void **)0)&&(handler!=(freehandler_t)0)) {
  void **ptr;

  ptr=list;
  while ((*ptr)!=(void *)0) {
    (void) handler(*ptr);
    ptr++;
    }
  (void) free(list);
  list=(void **)0;
  }
return list;
}
/*

*/
/********************************************************/
/*							*/
/*	Subroutine to get the local system time,        */
/*	can be offset by hours or days for	        */
/*	testing purpose.			        */
/*							*/
/********************************************************/
time_t rou_systime()

{
return time((time_t *)0)+off64_time+off_date;
}
/*

*/
/********************************************************/
/*							*/
/*	Subroutine to log event on syslog               */
/*							*/
/********************************************************/
void rou_valert(const int dlevel,const char *fmt,va_list ap)

#define	DEBMAX	80

{
if (debug>=dlevel)
  {
  char *ptr;
  char strloc[10000];

  (void) memset(strloc,'\000',sizeof(strloc));
  (void) vsnprintf(strloc,sizeof(strloc)-1,fmt,ap);
  ptr=strloc;
  if (foreground==true)
    (void) fprintf(stderr,"%s\n",strloc);
  else {
    while (strlen(ptr)>DEBMAX) {
      char locline[DEBMAX+10];
  
      (void) strncpy(locline,ptr,DEBMAX);
      locline[DEBMAX]='\000';
      (void) syslog(LOG_INFO,"%s",locline);
      ptr +=DEBMAX;
      } 
    (void) syslog(LOG_INFO,"%s",ptr);
    }
  }
}
/*

*/
/********************************************************/
/*							*/
/*	Subroutine to log event on syslog               */
/*							*/
/********************************************************/
void rou_alert(const int dlevel,const char *fmt,...)

{
va_list args;

va_start(args,fmt);
(void) rou_valert(dlevel,fmt,args);
va_end(args);
}
/*

*/
/********************************************************/
/*							*/
/*	Subroutine to CORE-DUMP the application         */
/*							*/
/********************************************************/
void rou_crash(const char *fmt,...)

{
#define	RELAX	5
va_list args;
char strloc[10000];

va_start(args,fmt);
(void) vsnprintf(strloc,sizeof(strloc),fmt,args);
(void) rou_alert(0,"Crashed on purpose:");
(void) rou_alert(0,"\t--> '%s'",strloc);
(void) rou_alert(0,"Crash delayed by '%d' second",RELAX);
(void) sleep(RELAX);	//To avoid immediat restart
va_end(args);
(void) kill(getpid(),SIGSEGV);
(void) exit(-1);	//unlikely to reach here
}
/*

*/
/********************************************************/
/*							*/
/*	Subroutine to return the date+time under        */
/*	a string format.			        */
/*							*/
/********************************************************/
char *rou_getstrfulldate(time_t curtime)

{
static char timeloc[50];

struct tm *ttime;

(void) strcpy(timeloc,"");
ttime=localtime(&curtime);
(void) snprintf(timeloc,sizeof(timeloc),
                        "%4d/%02d/%02d-%02d:%02d:%02d",
                        ttime->tm_year+1900,ttime->tm_mon+1,ttime->tm_mday,
			ttime->tm_hour,ttime->tm_min,ttime->tm_sec);
return timeloc;
}
/*

*/
/********************************************************/
/*							*/
/*	Subroutine to return a time stamp under         */
/*	the AAAAMMJJ format.			        */
/*							*/
/********************************************************/
u_long rou_normdate(time_t timestamp)

{
struct tm *ttime;

ttime=localtime(&timestamp);
return (((u_long)(ttime->tm_year+1900)*10000)+(ttime->tm_mon+1)*100)+ttime->tm_mday; 
}
/*

*/
/********************************************************/
/*							*/
/*	Subroutine to return the date only under        */
/*	a string format.			        */
/*							*/
/********************************************************/
char *rou_getstrdate(time_t normdate)

{
static char dateloc[50];

(void) strcpy(dateloc,"");
(void) snprintf(dateloc,sizeof(dateloc),
                        "%4ld/%02ld/%02ld",
                        normdate/10000,(normdate/100)%100,normdate%100);
return dateloc;
}
/*

*/
/************************************************/
/*						*/
/*	Subroutine to return the time only under*/
/*	a string format.			*/
/*						*/
/************************************************/
char *rou_getstrtime(time_t curtime)

{
static char timeloc[50];

struct tm *ttime;

ttime=localtime(&curtime);
(void) sprintf(timeloc,"%02d:%02d:%02d",
                        ttime->tm_hour,
                        ttime->tm_min,
                        ttime->tm_sec);
return timeloc;
}
/*

*/
/************************************************/
/*						*/
/*	Subroutine to return the hight precision*/
/*	time as string format.			*/
/*						*/
/************************************************/
char *rou_getstrchrono(TIMESPEC *chrono)

{
static char timeloc[50];

register TIMESPEC temps;

temps.tv_sec=(time_t)0;
temps.tv_nsec=(long)0;
if (chrono!=(TIMESPEC *)0) {
  temps.tv_sec=chrono->tv_sec;
  temps.tv_nsec=chrono->tv_nsec;
  }
temps.tv_sec%=(24*3600);      //one day max
(void) sprintf(timeloc,"%02ld:%02ld:%02ld.%03ld",
                        temps.tv_sec/3600,
                       (temps.tv_sec/60)%60,
                        temps.tv_sec%60,
                       (long)(round(temps.tv_nsec/1.0e6)));
return timeloc;
}
/*

*/
/************************************************/
/*						*/
/*	Subroutine to return the hight precision*/
/*	time, if start is NULL, then return     */
/*      the current high precision time.        */
/*						*/
/************************************************/
TIMESPEC *rou_getchrono(TIMESPEC *chrono,TIMESPEC *start)

{
#define OPEP    "subrou.c:rou_getchrono"
if (clock_gettime(CLOCK_MONOTONIC,chrono)<0) {
  (void) rou_alert(0,"%s, Unable to get chrono (error=<%s>)",
                    OPEP,strerror(errno));
  }
else {
  if (start!=(TIMESPEC *)0) {
    chrono->tv_sec-=start->tv_sec;
    chrono->tv_nsec-=start->tv_nsec;
    if (chrono->tv_nsec<0) {
      chrono->tv_sec--;
      chrono->tv_nsec+=1e6;
      }
    }
  }
return chrono;
#undef  OPEP
}
/*

*/
/************************************************/
/*						*/
/*	Subroutine to add incremental time to   */
/*	chrono counter.                         */
/*						*/
/************************************************/
TIMESPEC *rou_addchrono(TIMESPEC *chrono,TIMESPEC *increment)

{
if ((chrono!=(TIMESPEC *)0)&&(increment!=(TIMESPEC *)0)) {
  chrono->tv_sec  += increment->tv_sec;
  chrono->tv_nsec  += increment->tv_nsec;
  if (chrono->tv_nsec>1e9) {
    chrono->tv_sec+=1;
    chrono->tv_nsec-=1e9;
    }
  }
return chrono;
}
/*

*/
/************************************************/
/*						*/
/*	Subroutine to add X days to a time 	*/
/*						*/
/************************************************/
u_long rou_caldate(u_long date,int month,int days)

{
int locmonth;
int annees;
struct tm *ttime;
struct tm memtime;
time_t curdate;

if (date==0)
  date=19000101;
(void) memset((void *)&memtime,'\000',sizeof(memtime));
memtime.tm_mday=date%100;
memtime.tm_mon=((date/100)%100)-1;
memtime.tm_year=(date/10000)-1900;
memtime.tm_hour=12;
annees=abs(month)/12;
locmonth=abs(month)%12;
if (month>0) {
  memtime.tm_year+=annees;
  memtime.tm_mon+=locmonth;
  if (memtime.tm_mon>11) {
    memtime.tm_year++;
    memtime.tm_mon-=12;
    }
  }
if (month<0)
  {
  memtime.tm_year-=annees;
  memtime.tm_mon-=locmonth;
  if (memtime.tm_mon<0) {
    memtime.tm_year--;
    memtime.tm_mon+=12;
    }
  }
curdate=mktime(&memtime);
curdate+=(days*(24*3600));
ttime=localtime(&curdate);
return (((u_long)(ttime->tm_year+1900)*10000)+(ttime->tm_mon+1)*100)+ttime->tm_mday;
}
/*

*/
/********************************************************/
/*							*/
/*	Subroute to read a text file, get rid off the	*/
/*	commented in line (if requested) and return	*/
/*	the meaningful texte.				*/
/*							*/
/********************************************************/
char *rou_getstr(FILE *fichier,char *str,int taille,int comment,char carcom)

{
char *strloc;

while ((strloc=fgets(str,taille,fichier))!=(char *)0) {
  char *ptrloc;

  if (comment==false) {
    if (str[0]==carcom)
      continue;
    }
  ptrloc=str;
  while ((ptrloc=strchr(ptrloc,carcom))!=(char *)0) {
    if (*(ptrloc-1)=='\\') {
      (void) strcpy(ptrloc-1,ptrloc);	
      ptrloc++;
      }
    else {
      if (comment==false)
        *ptrloc='\000';
      break;
      }
    }
  if (str[strlen(str)-1]=='\n')
    str[strlen(str)-1]='\000';
  break;
  }
return strloc;
}
/*

*/
/********************************************************/
/*                                                      */
/*	Subroutine to return a time expressed	        */
/*	as a local date				        */
/*						        */
/********************************************************/
char *rou_localedate(time_t date)

{
char *str;

str=asctime(localtime(&date));
str[strlen(str)-1]='\000';
return str;
}
/*
^L
*/
/********************************************************/
/*                                                      */
/*      Returne the version number and application      */
/*      name.                                           */
/*                                                      */
/********************************************************/
char *rou_getversion()

{
return VERSION"."RELEASE;
}
/*
^L
*/
/********************************************************/
/*                                                      */
/*	Procedure to "open" usager to module.           */
/*      homework purpose                                */
/*      return zero if everything right                 */
/*                                                      */
/********************************************************/
int rou_opensubrou()

{
int status;

status=0;
if (modopen==false) {
  debug=0;
  foreground=false;
  off64_time=(time_t)0;
  off_date=(time_t)0;
  if (rootdir!=(char *)0) {
    (void) free(rootdir);
    status=-1;          //Rootdir should be found as NULL
    }
  rootdir=strdup("");
  modopen=true;
  }
return status;
}
/*
^L
*/
/********************************************************/
/*                                                      */
/*	Procedure to "close" usager to module.          */
/*      homework purpose                                */
/*      return zero if everything right                 */
/*                                                      */
/********************************************************/
int rou_closesubrou()

{
int status;

status=0;
if (modopen==true) {
  if (rootdir!=(char *)0) {
    (void) free(rootdir);
    rootdir=(char *)0;
    }
  modopen=false;
  }
return status;
}
