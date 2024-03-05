// vim: smarttab tabstop=8 shiftwidth=2 expandtab
/********************************************************/
/*							*/
/*	Low level subroutine implementation	        */
/*	handle schedule file contents                   */
/*							*/
/********************************************************/
#include        <stdio.h>
#include        <stdlib.h>
#include        <string.h>

#include	"subrou.h"
#include	"unisch.h"


//definition of scheduler available command
typedef enum    {
        cmd_frequency,          //backup frequency
        cmd_media,              //type of media to use
        cmd_keep,               //how long to keep backup
        cmd_levels,             //backup levels (set of files to backup)
        cmd_unknown             //default value (trouble)
        }CMDCOD;

//structure used to convert cron directives to next date
typedef	struct	{
                int hours[24];
                int days[31];
                int months[12];
                int dinmo[7];
		}TTYPE;

static  int modopen;            //boolean module open/close
/*

*/
/********************************************************/
/*							*/
/*	Subroutine to "compute" the next acceptable	*/
/*	date.						*/
/*							*/
/********************************************************/
static time_t nextoccurence(TTYPE *schedule)

{
int search;
time_t curdate;

curdate=(time((long *)0)/3600)*3600;
search=true;
while (search) {
  struct tm *timeptr;
  int i;

  curdate +=3600;
  timeptr=localtime(&curdate);
  search=false;
  for (i=0;i<4;i++) {
    switch (i) {
      case  0 : 
        search |= !schedule->hours[timeptr->tm_hour];
        break;
      case  1 : 
        search |= !schedule->days[timeptr->tm_mday];
        break;
      case  2 : 
        search |= !schedule->months[timeptr->tm_mon];
        break;
      case  3 : 
        search |= !schedule->dinmo[timeptr->tm_wday];
        break;
      }
    }
  }
return curdate;
}
/*

*/
/********************************************************/
/*							*/
/*	Subroutine to analyse schedule component from	*/
/*	the frequency field in a backup file.		*/
/*	dedicated to day name (Sun, Monday, etc..)	*/
/*							*/
/********************************************************/
static void getmyday(const char *detail,int *info,int taille)

{
static char *dayname[]=
	{
	"sun","mon","tue","wed","thu","fri","sat"
	};

int index;
int last;
int suite;
int dsp;
char theday[30];

index=0;
last=0;
suite=0;
(void) memset(info,'\000',sizeof(int)*taille);
while (detail[index]!='\000') {
  dsp=0;
  if (sscanf(&(detail[index]),"%[^*,-\n\r]%n",theday,&dsp)==1) {
    uint i;

    index +=dsp;
    for (i=0;i<sizeof(dayname)/sizeof(char *);i++) {
      if (!strncasecmp(theday,dayname[i],strlen(dayname[i]))) {
        info[i]=1;
        if (suite==1) {
          uint j;
          for (j=last;j<i;j++)
            info[j]=1;
	  last=0;
	  suite=0;
          }
        else
          last=i;
        break;
        }
      }
    }
  else {
    switch ((int)detail[index]) {
      case (int)'-' :
        suite=1;
        break;
      case (int)'*' :
        {
        int i;

        for (i=0;i<taille;i++)
          info[i]=1;
        }
        break;
      case (int)',' :
        break;
      }
    index++;
    }
  }
}
/*

*/
/********************************************************/
/*							*/
/*	Subroutine to analyse schedule component from	*/
/*	the frequency field in a backup file.		*/
/*							*/
/********************************************************/
static void getdetail(const char *detail,int *info,int taille,int offset)

{
int index;
int last;
int suite;
int divise;
int dsp;

index=0;
last=0;
suite=0;
divise=0;
(void) memset(info,'\000',sizeof(int)*taille);
while (detail[index]!='\000') {
  int number;
  
  number=0;
  dsp=0;
  if (sscanf(&(detail[index]),"%d%n",&number,&dsp)==1) {
    index +=dsp;
    number -=offset;
    if ((number>=0)&&(number<taille)) {
      info[number]=1;
      if (suite==1) {
        int i;

        for (i=last;i<number;i++)
          info[i]=1;
	last=0;
	suite=0;
        }
      else {
        if (divise==1) {
          int i;

          for (i=0;i<taille;i++)
            if ((i+offset)%number==0)
	      info[i]=1;
	    else
	      info[i]=0;
          divise=0;
          }
 	else
          last=number;
        }
      }
    }
  else {
    switch ((int)detail[index]) {
      case (int)'-' :
        suite=1;
        break;
      case (int)'*' :
        {
        int i;

        for (i=0;i<taille;i++)
          info[i]=1;
        }
        break;
      case (int)'/' :
        divise=1;
        break;
      case (int)',' :
        break;
      }
    index++;
    }
  }
}
/*

*/
/********************************************************/
/*							*/
/*	Subroutine to extract the next backup date from	*/
/*	the frequency string				*/
/*							*/
/********************************************************/
static time_t getschdate(const char *frequency)

{
#define OPEP    "unisch.c:getschdate"
#define	NBRFIELD 4

char **zone;
TTYPE schedule;
int i;

(void) memset(&schedule,'\000',sizeof(schedule));
zone=(char **)calloc(NBRFIELD,sizeof(char *));
for (i=0;i<NBRFIELD;i++) {
  zone[i]=(char *)calloc(strlen(frequency),sizeof(char));
  (void) strcpy(zone[i],"*");
  }
(void) sscanf(frequency,"%s %s %s %s",
	                 zone[0],zone[1],zone[2],zone[3]);
for (i=0;i<NBRFIELD;i++) {
  switch (i) {
    case   0 :
      (void) getdetail(zone[i],schedule.hours,24,0);
      break;
    case   1 :
      (void) getdetail(zone[i],schedule.days,31,1);
      break;
    case   2 :
      (void) getdetail(zone[i],schedule.months,12,1);
      break;
    case   3 :
      (void) getmyday(zone[i],schedule.dinmo,7);
      break;
    default  :
      (void) rou_alert(0,"%s Unexpected zone[%d] (bug?)",OPEP,i);
      break;
    }
  }
for (i=0;i<NBRFIELD;i++)
  (void) free(zone[i]);
(void) free(zone);
return nextoccurence(&schedule);
#undef  NBRFIELD
#undef  OPEP
}
/*
^L
*/
/********************************************************/
/*                                                      */
/*	Procedure to parse scheduler command information*/
/*                                                      */
/********************************************************/
static  CMDCOD findcmd(char *command)

{
typedef struct  {
        const char *id;         //command string
        CMDCOD code;            //command code
        }CMDTYP;

//list of scheduler entry.
const CMDTYP cmd_list[]={
        {"frequency",cmd_frequency},
        {"media",cmd_media},
        {"keep",cmd_keep},
        {"levels",cmd_levels},
        {(const char *)0,cmd_unknown}
        };

CMDCOD code;
const CMDTYP *ptr;

code=cmd_unknown;
ptr=cmd_list;
while (ptr->id!=(char *)0) {
  if (strcmp(ptr->id,command)==0) {
    code=ptr->code;
    break;
    }
  ptr++;
  }
return code;
}

/*
^L
*/
/********************************************************/
/*                                                      */
/*	Procedure to scan a line looking for sceduling  */
/*      information.                                    */
/*                                                      */
/********************************************************/
static SCHTYP *scanline(SCHTYP *sch,char *line)

{
#define OPEP    "unisch.c:scanline"
#define FRMS    "<%s> is a duplicate to <%s=%s,...> in file <%s> (config problem?)"
#define FRMD    "<%s> is a duplicate to <%s=%d> in file <%s> (config problem?)"

int phase;
int proceed;
char command[100];
char info[100];

phase=0;
proceed=true;
while (proceed==true) {
  switch (phase) {
    case 0      :       //can we parse the line?
      if (strlen(line)>(sizeof(command)+sizeof(info))) {
        (void) rou_alert(0,"%s, line <%s> in file <%s> is too long (check config!)",
                            OPEP,line,sch->configname);
        phase=999;      //no need to go further
        }
      break;
    case 1      :       //check scanning
      if (sscanf(line,"%[^=\n\r]=%[^\n\r]",command,info)!=2) {
        (void) rou_alert(0,"%s, unable to parse line <%s> in config file <%s> "
                           "(check config!)",
                           OPEP,line,sch->configname);
        phase=999;      //no need to go further
        }
      break;
    case 2      :       //what command do we have
      switch (findcmd(command)) {
        case cmd_frequency      :
          if (sch->frequency!=(char *)0) {
            (void) rou_alert(0,FRMS,line,command,sch->frequency,sch->configname);
            (void) free(sch->frequency);
            }
          sch->frequency=strdup(info); 
          sch->start=getschdate(info);
          (void) rou_alert(7,"Found schedule <%s> to be at <%s>",
                              sch->configname,
                              rou_localedate(sch->start));
          break;
        case cmd_media          :
          if (sch->media!=(char *)0) {
            (void) rou_alert(0,FRMS,line,command,sch->media,sch->configname);
            (void) free(sch->media);
            }
          sch->media=strdup(info); 
          break;
        case cmd_keep           :
          if (sch->days>=0) {
            (void) rou_alert(0,FRMD,line,command,sch->days,sch->configname);
            }
          sch->days=atoi(info); 
          break;
        case cmd_levels         :
          if (sch->levels!=(char **)0) {
            (void) rou_alert(0,FRMS,line,command,sch->levels[0],sch->configname);
            sch->levels=(char **)rou_freelist((void **)sch->levels,
                                              (freehandler_t)free);
            }
          //splitting level data
          if (strlen(info)>0) {
            char *ptr;

            ptr=info;
            while (ptr!=(char *)0) {
              char *comma;

              if ((comma=strchr(ptr,','))!=(char *)0) {
                *comma='\000';
                comma++;
                if (strlen(comma)==0)
                  comma=(char *)0;
                }
              sch->levels=(char **)rou_addlist((void **)sch->levels,
                                               (void *)strdup(ptr));
              ptr=comma;
              }
            }
          break;
        default                 :
          (void) rou_alert(0,"Unable to scan line <%s> "
                             "within scheduler config file <%s> (format?)",
                             line,sch->configname);
          break;
        }
      break;
    default     :       //task completed;
      proceed=false;
      break;
    }
  phase++;
  }
return sch;
#undef  FRMT
#undef  OPEP
}
/*
^L
*/
/********************************************************/
/*                                                      */
/*	Procedure to free memory used by one scheduler  */
/*      entry.                                          */
/*                                                      */
/********************************************************/
SCHTYP *sch_freesch(SCHTYP *sch)

{
if (sch!=(SCHTYP *)0) {
  if (sch->levels!=(char **)0)
    sch->levels=(char **)rou_freelist((void **)sch->levels,
                                      (freehandler_t)free);
  if (sch->media!=(char *)0)
    (void) free(sch->media);
  if (sch->frequency!=(char *)0)
    (void) free(sch->frequency);
  if (sch->configname!=(char *)0)
    (void) free(sch->configname);
  (void) free(sch);
  }
return sch;
}
/*
^L
*/
/********************************************************/
/*                                                      */
/*	Procedure to scan a scheduler list and return   */
/*      the very next one in due time.                  */
/*                                                      */
/********************************************************/
SCHTYP *sch_nextsch(SCHTYP **todolist)

{
SCHTYP *selected;

selected=(SCHTYP *)0;
if (todolist!=(SCHTYP **)0) {
  while (*todolist!=(SCHTYP *)0) {
    if (selected==(SCHTYP *)0)
      selected=*todolist;
    else {
      if (selected->start>(*todolist)->start) 
        selected=*todolist;
      }
    todolist++;
    }
  }
return selected;
}
/*
^L
*/
/********************************************************/
/*                                                      */
/*	Procedure to scan a scheduler file and build    */
/*      an scheduler entry.                             */
/*                                                      */
/********************************************************/
SCHTYP *sch_filetosch(char *filename)

{
#define OPEP    "unisch.c:sch_filetosch"
SCHTYP *sch;
FILE *fichier;
int phase;
int proceed;

sch=(SCHTYP *)0;
phase=0;
proceed=true;
while (proceed==true) {
  switch (phase) {
    case 0      :       //do we have a filename
      if (filename==(char *)0) {
        (void) rou_alert(0,"%s, filename is missing altogether (BUG!)",OPEP);
        proceed=false;
        }
      break;
    case 1      :       //opening file
      if ((fichier=fopen(filename,"r"))==(FILE *)0) {
        (void) rou_alert(0,"%s, Unable to open file <%s> (error=<%s>)",
                            OPEP,filename,strerror(errno));
        proceed=false;
        }
      break;
    case 2      :       //scanning file contents
      char line[100];

      sch=(SCHTYP *)calloc(1,sizeof(SCHTYP));
      sch->days=-1;
      sch->configname=strdup(filename);
      while (rou_getstr(fichier,line,sizeof(line)-1,false,'#')!=(char *)0) {
        sch=scanline(sch,line);
        }
      (void) fclose(fichier);
      break;
    case 3      :       //checking record and setting default values
      if (sch->frequency==(char *)0)
        sch->frequency=strdup("15 * * Fri");
      if (sch->media==(char *)0)
        sch->media=strdup("DAT72");
      if (sch->days==-1)
        sch->days=0;
      if (sch->levels==(char **)0)
        sch->levels=(char **)rou_addlist((void **)0,(void *)strdup("systems"));
      break;
    default     :
      proceed=false;
      break;
    }
  phase++;
  }
return sch;
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
int sch_openunisch()

{
int status;

status=0;
if (modopen==false) {
  (void) rou_opensubrou();
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
int sch_closeunisch()

{
int status;

status=0;
if (modopen==true) {
  (void) rou_closesubrou();
  modopen=false;
  }
return status;
}
