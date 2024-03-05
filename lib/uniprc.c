// vim: smarttab tabstop=8 shiftwidth=2 expandtab
/********************************************************/
/*							*/
/*	Module for low level subroutine			*/
/*							*/
/********************************************************/

#include	<sys/prctl.h>
#include	<sys/resource.h>
#include	<sys/stat.h>
#include	<dirent.h>
#include	<errno.h>
#include	<fcntl.h>
#include	<stdarg.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<signal.h>
#include	<syslog.h>
#include	<unistd.h>

#include        "subrou.h"
#include        "uniprc.h"


//directory to set lock
#define DIRLOCK                 "/var/run/"APPNAME

static  int modopen;            //boolean module open/close
/*

*/
/********************************************************/
/*						        */
/*	Procedure to find out a possible drip zone      */
/*      for abort() core memory dump.                   */
/*						        */
/********************************************************/
static char *get_dropzone()

{
const char *dropzone="/tmp/backd-crash";

char command[100];

(void) snprintf(command,sizeof(command)-2,"mkdir -p %s",dropzone);
(void) system(command);
return strdup(dropzone);
}
/*

*/
/********************************************************/
/*						        */
/*	Subroutine to open a pipe, return NULL if       */
/*	trouble.				        */
/*      						*/
/********************************************************/
FILE *prc_openpipe(char *pipecmd,char *type)

{
#define OPEP    "uniprc.c:openpipe"

FILE *file;

file=(FILE *)0;
if (pipecmd!=(char *)0) {
  if ((file=popen(pipecmd,type))==(FILE *)0) {
    (void) rou_alert(0,"%s, Unable to open pipe to '%s' in mode '%s' "
                       "(error=<%s>)",OPEP,pipecmd,type,strerror(errno));
    }
  }
return file;
#undef  OPEP
}
/*

*/
/********************************************************/
/*						        */
/*	Subroutine to close a pipe, free command string */
/*						        */
/********************************************************/
char *prc_closepipe(FILE *file,char *pipecmd)

{
#define OPEP    "uniprc.c:closepipe"

if (pipecmd!=(char *)0) {
  if (file!=(FILE *)0) {
    if (pclose(file)<0) {
      (void) rou_alert(0,"%s, Unable to close pip for command <%s> "
                         "(sys defect?, error=<%s>)",
                         OPEP,pipecmd,strerror(errno));
      }
    }
  (void) free(pipecmd);
  pipecmd=(char *)0;
  }
return pipecmd;
#undef OPEP
}
/*

*/
/********************************************************/
/*						        */
/*	Procedure to allow process core DUMP	        */
/*	to trace origin of problem		        */
/*						        */
/*      NOTE:                                           */
/*      On linux to have a working coredump, you MUST   */
/*      add to /etc/sysctl.conf                         */
/*          fs.suid_dumpable=1                          */
/*          kernel.core_uses_pid=1                      */
/*          kernel.core_pattern=./core.%e.%p            */
/*						        */
/********************************************************/
void prc_allow_core_dump()

{
struct rlimit limites;

if (getrlimit(RLIMIT_CORE,&limites)<0) {
  (void) fprintf(stderr,"getrlimit error='%s'",strerror(errno));
  }
limites.rlim_cur=limites.rlim_max;
if (setrlimit(RLIMIT_CORE,&limites)<0) {
  (void) fprintf(stderr,"setrlimit error='%s'",strerror(errno));
  }
(void) prctl(PR_SET_DUMPABLE,1,0,0,0);/*to allow core-dump	*/
}
/*

*/
/********************************************************/
/*      						*/
/*	Procedure to display an core_dump	        */
/*	message and terminate application	        */
/*						        */
/********************************************************/
void prc_core_dump(const char *fmt,...)

{
#define MAXCRASH        10      //maximun crash file within drop zone
#define COREDELAY        5      //number of second for core delay

va_list args;
char *crashdir;
int doabort;
int phase;
int proceed;

va_start(args,fmt);
crashdir=(char *)0;
doabort=false;
phase=0;
proceed=true;
while (proceed==true) {
  switch (phase) {
    case 0      :               //find out if crashdir is existing
      if ((crashdir=get_dropzone())==(char *)0)
        phase=999;              //no need to go further.
      break;
    case 1      :               //can we access the crash dir
      if (chdir(crashdir)<0) {    //Unable to access crashdir
        (void) free(crashdir);
        phase=999;              //no need to go further.
        }
      break;
    case 2      :               //no too much crash so fare?
      DIR *dirp;

      (void) rou_alert(0,"Trying to store core-dump within directory <%s>",
                         crashdir);
      if ((dirp=opendir(crashdir))!=(DIR *)0) {
        int nument;
        struct dirent *entry;

        nument=0;
        while ((entry=readdir(dirp))!=(struct dirent *)0) {
          if (entry->d_type==DT_REG)
            nument++;
          }
        (void) closedir(dirp);
        if (nument>MAXCRASH) {
          (void) rou_alert(0,"Aborting Coredump file generation!");
          (void) rou_alert(0,"Too many crash files already within <%s>",
                              crashdir);
          phase=999;          //No allowed to do abort
          }
        }
      (void) free(crashdir);
      break;
    case 3      :               //Ok we can call abort
      doabort=true;
      break;
    default     :
      proceed=false;
      break;
    }
  phase++;
  }
(void) sleep(COREDELAY);	/*to avoid crash avalanche	*/
(void) rou_valert(LOG_INFO,fmt,args);
va_end(args);
if (doabort==true) 
  (void) abort();	/*doing to do the abort		*/
(void) rou_alert(0,"Unable to dump core memory");
(void) exit(-1);	//Theoriticaly unreachabe if
                        //abort was allowed
#undef  MAXCRASH
#undef  COREDELAY
}
/*
^L
*/
/********************************************************/
/*                                                      */
/*	procedure to check if a process is	        */
/*	still up and running.				*/
/*                                                      */
/********************************************************/
int prc_checkprocess(pid_t pidnum)

{
#define	SIGCHECK 0	//signal to check if process
			//is existing.
			//
int status;

status=false;
switch(pidnum) {
  case (pid_t)0	:	/*0 means no process	*/
    status=false;
    break;
  case (pid_t)1	:	/*init process always OK*/
    status=true;
    break;
  default	:	/*standard process	*/
    if (kill(pidnum,SIGCHECK)==0)
      status=true;
    break;
  }
return status;
#undef	SIGCHECK
}
/*
^L
*/
/********************************************************/
/*                                                      */
/*	Procedure to set/unset a lock 		        */
/*	return true if successful,		        */
/*	false otherwise.			        */
/*                                                      */
/********************************************************/
int prc_locking(const char *lockname,int lock,int tentative)

{
#define	OPEP	"uniprc.c:lck_locking,"

int done;
char *fullname;
struct stat bufstat;
int phase;
int proceed;

done=false;
fullname=(char *)0;
phase=0;
proceed=true;
while (proceed==true) {
  switch (phase) {
    case 0	:	//setting lock filename
      if (lockname==(const char *)0) {
	(void) rou_alert(9,"%s lockname is missing (bug?)",OPEP);
	phase=999;	//big trouble, No need to go further
        }
      break;
    case 1	:	//creating the lock directory if needed
      int status;
      char cmd[100];

      fullname=rou_apppath(DIRLOCK);
      (void) snprintf(cmd,sizeof(cmd),"mkdir -p %s",fullname);
      if ((status=system(cmd))!=0) {
	(void) rou_alert(9,"Unable to create <%s> directory (system?/bug?)",
                            fullname);
	phase=999;	//big trouble, No need to go further
        }
      (void) free(fullname);
      break;
    case 2	:	//setting lock filename
      const char *fname;
      char *name;

      if ((fname=strrchr(lockname,'/'))==(char *)0)
        fname=lockname;
      else
        fname++; 
      name=(char *)calloc(sizeof(DIRLOCK)+strlen(fname)+10,sizeof(char));
      (void) sprintf(name,"%s/%s.lock",DIRLOCK,fname);
      fullname=rou_apppath(name);
      (void) free(name);
      break;
    case 3	:	//checking if link already exist
      if (stat(fullname,&bufstat)<0) {
        phase++;        //no need to check lock contents
        }
      break;
    case 4	:	//making lockname
      if (S_ISREG(bufstat.st_mode)!=0) {
        FILE *fichier;

        if ((fichier=fopen(fullname,"r"))!=(FILE *)0) {
          pid_t pid;
          char strloc[80];

	  (void) fgets(strloc,sizeof(strloc)-1,fichier);
	  (void) fclose(fichier);
          if (sscanf(strloc,"%lu",(u_long *)(&pid))==1) {
            (void) rou_alert(2,"Locking, check %d process active",pid);
	    if (prc_checkprocess(pid)==false) {
              (void) rou_alert(2,"Locking, removing pid=%d unactive lock",pid);
	      (void) unlink(fullname);
              }
	    else {
              if (lock==LCK_LOCK) {
                (void) rou_alert(0,"lock check, found %d process still active",pid);
	        phase=999;	//no need to go further
                }
	      }
	    }
	  }
	}
      break;
    case 5	:	//do we need to unlock ?
      if (lock==LCK_UNLOCK) {
	(void) rou_alert(9,"%s Request unlocking <%s>",OPEP,fullname);
	(void) unlink(fullname);
	done=true;
	phase=999;	//No need to go further
	}
      break;
    case 6	:	//making lockname
      (void) rou_alert(6,"%s Request locking <%s>",OPEP,fullname);
      while (tentative>0) {
	int handle;

        tentative--;
        if ((handle=open(fullname,O_RDWR|O_EXCL|O_CREAT,0640))>=0) {
          char numid[30];

          (void) snprintf(numid,sizeof(numid),"%d\n",getpid());
          (void) write(handle,numid,strlen(numid));
          (void) close(handle);
	  done=true;
	  break;	//breaking "tentative" loop
	  }
	else {
          (void) rou_alert(3,"Trying one more second to lock <%s> (error=<%s>)",
			      fullname,strerror(errno));
          (void) sleep(1);
	  }
	}
      break;
    default	:	//SAFE Guard
      if (done==false)
        (void) rou_alert(2,"Unable to set <%s> lock (config?)",lockname);
      proceed=false;
      break;
    }
  phase++;
  }
if (fullname!=(char *)0)
  (void) free(fullname);
return done;
#undef	OPEP
}
/*
^L
*/
/********************************************************/
/*                                                      */
/*						        */
/*	Procedure to put a process in background        */
/*	mode.					        */
/*	Return the child process id.		        */
/*                                                      */
/********************************************************/
pid_t prc_divedivedive()

{
#define	OPEP	"uniprc:rou_divedivedive,"
pid_t childpid;

childpid=(pid_t)0;
switch (childpid=fork()) {
  case -1	:
    (void) fprintf(stderr,"%s, Unable to dive! (error=<%s>)",
			   OPEP,strerror(errno));
    break;
  case  0	:
    //	we are now in background mode
    (void) setsid();
    break;
  default	:
    //waiting for ballast to fill up :-}}
    (void) sleep(1);
    break;
  }
return childpid;
#undef	OPEP
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
int prc_openuniprc()

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
int prc_closeuniprc()

{
int status;

status=0;
if (modopen==true) {
  (void) rou_closesubrou();
  modopen=false;
  }
return status;
}
