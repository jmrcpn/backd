// vim: smarttab tabstop=8 shiftwidth=2 expandtab
/********************************************************/
/*							*/
/*	Module to manage configuration file access and  */
/*      parsing.                                        */
/*							*/
/********************************************************/
#include        <sys/stat.h>
#include        <dirent.h>
#include        <fcntl.h>
#include        <stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include        <unistd.h>

#include	"subrou.h"
#include	"unibck.h"
#include	"unisch.h"
#include	"gesfil.h"

static  int modopen;            //boolean module open/close
/*

*/
/********************************************************/
/*							*/
/*	Subroutine to select file with a specific       */
/*      extension with a direntry.                      */
/*	directory.					*/
/*							*/
/********************************************************/
static int selextension(const struct dirent *dir,const char *ext)

{
char *ptr;
struct stat bufstat;

if (dir->d_ino==0)
  return 0;
if ((ptr=strrchr(dir->d_name,'.'))==(char *)0)
  return 0;
ptr++;
if (strcmp(ptr,ext)!=0)
  return 0;
if (stat(dir->d_name,&bufstat)<0)
  return 0;
return S_ISREG(bufstat.st_mode);
}
/*

*/
/********************************************************/
/*							*/
/*	Subroutine to select scheduler file in the conf	*/
/*	directory.					*/
/*							*/
/********************************************************/
static int selsched(const struct dirent *dir)

{
return selextension(dir,"sch");
}
/*

*/
/********************************************************/
/*							*/
/*	Subroutine to select backup list file in the	*/
/*	backd.d directory.				*/
/*							*/
/********************************************************/
static int selbackd(const struct dirent *dir)

{
return selextension(dir,"bck");
}
/*
^L
*/
/********************************************************/
/*                                                      */
/*	Procedure to collect all backlist definition    */
/*      availabel within the configuration directory.   */
/*      return zero if everything right                 */
/*                                                      */
/********************************************************/
SCHTYP **fil_getschedlist(char *confdir)

{
#define OPEP    "gesfil.c:getschedlist"
SCHTYP **todo;
struct dirent **dirlst;
char curdir[PATH_MAX ];
char *dname;
int nbrdir;
int phase;
int proceed;

todo=(SCHTYP **)0;
dirlst=(struct dirent **)0;
(void) memset(curdir,'\000',sizeof(curdir));
dname=(char *)0;
nbrdir=0;
phase=0;
proceed=true;
while (proceed==true) {
  switch (phase) {
    case 0      :       //getting the currendirectoy
      if (getcwd(curdir,sizeof(curdir)-1)<0) {
        (void) rou_alert(0,"%s Unable get current directory name (error=<%s>)",
                            OPEP,strerror(errno));
        proceed=false;  //direct exit
        }
      break;
    case 1      :       //find the configuration directory
      if (confdir==(char *)0)
        confdir=CONFDIR;
      dname=rou_apppath(confdir);
      if (chdir(dname)<0) {
        (void) rou_alert(0,"%s Unable change directory to <%s> (error=<%s>)",
                            OPEP,dname,strerror(errno));
        proceed=false;  //direct exit
        }
      break;
    case 2      :       //scanning the configuration directory
      if ((nbrdir=scandir(".",&dirlst,selsched,alphasort))<0) {
        (void) rou_alert(0,"%s Unable to scan directory <%s> (error=<%s>)",
                            OPEP,dname,strerror(errno));
        phase=999;      //no need to go further
        }
      break;
    case 3      :       //scaning the configuration directory
      int i;

      for (i=0;i<nbrdir;i++) {
        SCHTYP *sch;

        if ((sch=sch_filetosch(dirlst[i]->d_name))!=(SCHTYP *)0)
          todo=(SCHTYP **)rou_addlist((void **)todo,(void *)sch);
        (void) free(dirlst[i]);
        }
      (void) free(dirlst);
      break;
    default     :
      if (chdir(curdir)<0) {
        (void) rou_alert(0,"%s Unable retrieve previous directory <%s> "
                           "(error=<%s>)",OPEP,curdir,strerror(errno));
        }
      proceed=false;
      break;
    }
  phase++;
  }
if (dname!=(char *)0)
  (void) free(dname); 
return todo;
#undef  OPEP
}
/*
^L
*/
/********************************************************/
/*                                                      */
/*	Procedure to collect list of file to be backup  */
/*      at the next scheduled time                      */
/*                                                      */
/********************************************************/
BCKTYP **fil_getbcklist(const char *backdir,SCHTYP *sch,char *tapeid)

{
#define OPEP    "gesfil.c:fil_getbcklist"

BCKTYP **tobackup;
struct dirent **dirlst;
int nbrdir;
char curdir[PATH_MAX ];
int phase;
int proceed;

tobackup=(BCKTYP **)0;
dirlst=(struct dirent **)0;
nbrdir=0;
(void) memset(curdir,'\000',sizeof(curdir));
phase=0;
proceed=true;
while (proceed==true) {
  switch (phase) {
    case 0      :       //Getting the current drectory
      if (getcwd(curdir,sizeof(curdir)-1)<0) {
        (void) rou_alert(0,"%s Unable get current directory name (error=<%s>)",
                            OPEP,strerror(errno));
        proceed=false;  //direct exit
        }
      break;
    case 1      :       //find the configuration directory
      char *dname;

      if (backdir==(const char *)0)
        backdir=BACKDIR;
      dname=rou_apppath(backdir);
      if (chdir(dname)<0) {
        (void) rou_alert(0,"%s Unable change directory to <%s> (error=<%s>)",
                            OPEP,dname,strerror(errno));
        proceed=false;  //direct exit
        }
      (void) free(dname); 
      break;
    case 2      :       //scanning the configuration directory
      if ((nbrdir=scandir(".",&dirlst,selbackd,alphasort))<0) {
        (void) rou_alert(0,"%s Unable to scan directory <%s> (error=<%s>)",
                            OPEP,dname,strerror(errno));
        phase=999;      //no need to go further
        }
    case 3      :       //no backup directory?
      if (nbrdir==0) {
        (void) rou_alert(1,"%s backup directory <%s> is empty\?\?",
                           OPEP,dname);
        phase=999;
        }
      break;
    case 4      :       //adding backup header
      BCKTYP *bck;
      char included[40];

      (void) snprintf(included,sizeof(included)-1,"%s.hdr",tapeid);
      bck=(BCKTYP *)calloc(1,sizeof(BCKTYP)); 
      bck->report=rpt_header;
      bck->included=(char **)rou_addlist((void **)bck->included,
                                         (void *)strdup(included));
      tobackup=(BCKTYP **)rou_addlist((void **)tobackup,(void *)bck);
      break;
    case 5      :       //scaning the backup directory directory
      int i;

      for (i=0;i<nbrdir;i++) {
        BCKTYP *bck;

        if ((bck=bck_getbcklist(dirlst[i]->d_name,sch->levels))!=(BCKTYP *)0) {
          bck->report=rpt_data;
          tobackup=(BCKTYP **)rou_addlist((void **)tobackup,(void *)bck);
          }
        (void) free(dirlst[i]);
        }
      (void) free(dirlst);
      break;
    default     :       //task completed
      if (chdir(curdir)<0) {
        (void) rou_alert(0,"%s Unable retrieve previous directory <%s> "
                           "(error=<%s>)",OPEP,dname,strerror(errno));
        }
      proceed=false;
      break;
    }
  phase++;
  }
return tobackup;
#undef  OPEP
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
int fil_opengesfil()

{
int status;

status=0;
if (modopen==false) {
  (void) rou_opensubrou();
  (void) sch_openunisch();
  (void) bck_openunibck();
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
int fil_closegesfil()

{
int status;

status=0;
if (modopen==true) {
  (void) bck_closeunibck();
  (void) sch_closeunisch();
  (void) rou_closesubrou();
  modopen=false;
  }
return status;
}
