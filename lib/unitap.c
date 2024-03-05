// vim: smarttab tabstop=8 shiftwidth=2 expandtab
/********************************************************/
/*							*/
/*	Low level subroutine implementation	        */
/*	handle tape contents                            */
/*							*/
/********************************************************/
#include        <stdio.h>
#include        <stdlib.h>
#include        <string.h>

#include	"subrou.h"
#include	"unitap.h"

const struct    {
        const   char *id;       //header entry name
        u_int   num;            //header number
        }hdrlst[]=
          {
          {"TapeId",1},
          {"PreviousID",2},
          {"NextId",3},
          {"Media_size",4},
          {"Num_segment",5},
          {(const char *)0,0}
          };

static  int modopen;            //boolean module open/close
/*
^L
*/
/********************************************************/
/*							*/
/*	procedure to add data to the tape structure     */
/*							*/
/********************************************************/
static TAPTYP *dispatchid(TAPTYP *tape,char *id,char *data)

{
#define OPEP    "unitap.c:dispatchid,"

int i;

for (i=0;(hdrlst[i].id!=(const char *)0)&&(tape!=(TAPTYP *)0);i++) {
  if (strcmp(hdrlst[i].id,id)==0) {
    char *ptr;

    ptr=(char *)0;
    switch (hdrlst[i].num) {
      case 1    :
      case 2    :
      case 3    :
        ptr=tape->id[i];
        (void) memset(ptr,'\000',sizeof(tape->id[i]));
        (void) strncpy(ptr,data,sizeof(tape->id[i])-1);
        break;
      case 4    :
        tape->mediasize=strtoul(data,(char **)0,10);
        break;
      case 5    :
        tape->numsegment=strtoul(data,(char **)0,10);
        break;
      default   :
        (void) rou_alert(3,"%s Unable to add id <%s> within "
                           "tape structure (header format?)",OPEP,id);
        break;
      }
    break;
    }
  }
return tape;
#undef  OPEP
}
/*
^L
*/
/********************************************************/
/*							*/
/*	procedure to free tap structure.                */
/*							*/
/********************************************************/
TAPTYP *tap_freetape(TAPTYP *data)

{
if (data!=(TAPTYP *)0) 
  (void) free(data);
data=(TAPTYP *)0;
return data;
}
/*
^L
*/
/********************************************************/
/*							*/
/*	procedure to initialize a tape structure        */
/*							*/
/********************************************************/
TAPTYP *tap_newtape()

{
#define BLOCKSIZE      32768    //default tape block size in Bytes
#define MEDIASIZE      16384    //default media size in megabytes (16G)
register const char *none="none";

TAPTYP *tape;

tape=(TAPTYP *)calloc(1,sizeof(TAPTYP));
(void) strcpy(tape->id[0],none);
(void) strcpy(tape->id[1],none); 
(void) strcpy(tape->id[2],none);
(void) strcpy(tape->media,"DAT72");
(void) strcpy(tape->vers,rou_getversion());
tape->mediasize=MEDIASIZE;
return tape;
}

/*
^L
*/
/********************************************************/
/*							*/
/*	procedure to duplicate tape contents            */
/*							*/
/********************************************************/
TAPTYP *tap_duplicate(TAPTYP *tape)

{
TAPTYP *dup;

dup=(TAPTYP *)0;
if (tape!=(TAPTYP *)0) {
  dup=(TAPTYP *)calloc(1,sizeof(TAPTYP));
  (void) memcpy(dup,tape,sizeof(TAPTYP));
  }
return dup;
}
/*
^L
*/
/********************************************************/
/*							*/
/*      Convert a tape structure to a string (to be used*/
/*      within tapeliste).                              */
/*							*/
/********************************************************/
char *tap_tapetostr(TAPTYP *tape)

{
char *line;

line=(char *)0;
if (tape!=(TAPTYP *)0) {
  char data[300];

  (void) snprintf(data,sizeof(data)-5,"%-16s%-7s"
                                      "%6u% 12ld% 13d% 6d% 10d",
                                      tape->id[0],
                                      tape->media,
                                      tape->mediasize,
                                      tape->lastused,
                                      tape->frozen,
                                      tape->cycled,
                                      tape->pidlock);
  line=strdup(data);
  }
return line;
}
/*
^L
*/
/********************************************************/
/*							*/
/*      Convert a string (from tapeliste) to a tape     */
/*      structure.                                      */
/*							*/
/********************************************************/
TAPTYP *tap_strtotape(const char *data)

{
TAPTYP *tape;

tape=(TAPTYP *)0;
if (data!=(const char *)0) {
  u_int mediasize;
  u_int lastused;
  u_int frozen;
  u_int cycled;
  u_int pidlock;
  char *id;
  char *media;

  mediasize=0;
  lastused=0;
  frozen=0;
  cycled=0;
  pidlock=0;
  id=(char *)0;
  media=(char *)0;
  if (sscanf(data,"%m[a-z,A-Z-,0-9]"    //tapeid
                  " %m[a-z,A-Z-,0-9]"   //poolid
                  " %u"                 //blocksize
                  " %u"                 //lastused
                  " %u"                 //frozen
                  " %u"                 //cycled
                  " %u"                 //pidlock
                 ,&id,&media,&mediasize,//minimal information
                 &lastused,&frozen,&cycled,&pidlock)>=3) {
    tape=tap_newtape();
    if (id!=(char *)0) {
      (void) strncpy(tape->id[0],id,sizeof(tape->id[0])-1);
      (void) free(id);
      }
    if (media!=(char *)0) {
      (void) strncpy(tape->media,media,sizeof(tape->media)-1);
      (void) free(media);
      }
    tape->mediasize=mediasize;
    tape->lastused=lastused;
    tape->frozen=frozen;
    tape->cycled=cycled;
    tape->pidlock=pidlock;
    }
  }
return tape;
}
/*
^L
*/
/********************************************************/
/*							*/
/*      Procedure to convert a tape header to a TAPTYP  */
/*      structure.                                      */
/*							*/
/********************************************************/
TAPTYP *tap_headertotape(void *header)

{
#define OPEP    "unitape.c:tap_headertotape,"

TAPTYP *tape;
int phase;
int proceed;

tape=(TAPTYP *)0;
phase=0;
proceed=true;
while (proceed==true) {
  switch (phase) {
    case 0      :       //do we have an header?
      if (header==(void *)0) {
        (void) rou_alert(2,"%s tape header missing <%s> (Bug?)",OPEP);
        phase=999;      //no need to go further
        }
      break;
    case 1      :       //preparing the tape area
      tape=(TAPTYP *)calloc(1,sizeof(TAPTYP));
      break;
    case 2      :       //do we have an header?
      int num;
      char *ptr;
      char *data;
      char id[20];

      data=(char *)0;
      ptr=(char *)header;
      while (sscanf(ptr,"%[a-z,A-Z_]:"
                        "%m[a-z-,A-Z,0-9]\n"
                        "%n",id,&data,&num)==2) {
        ptr+=num;
        tape=dispatchid(tape,id,data);
        (void) free(data);
        data=(char *)0;
        if (tape==(TAPTYP *)0) {
          phase=999;            //no need to go further
          break;
          }
        }
      break;
    default     :
      proceed=false;
      break;
    }
  phase++;
  }
return tape;
#undef  OPEP
}
/*
^L
*/
/********************************************************/
/*							*/
/*      Procedure to convert a TAPTYP to a string       */
/*      within a area of blocksize                      */
/*							*/
/********************************************************/
char *tap_tapetoheader(TAPTYP *tape,ul64 blksize)

{
#define OPEP    "unitap.c:tapetoheader,"

void *zone;
int phase;
int proceed;

zone=(void *)0;
phase=0;
proceed=true;
while (proceed==true) {
  switch (phase) {
    case 0      :
      if (tape==(TAPTYP *)0) {
        (void) rou_alert(2,"%s tape structure missing (Bug?)",OPEP);
        phase=999;              //no need to go further
        }
      break;
    case 1      :               //converting header to string
      char *ptr;
      int i;

      zone=(void *)calloc(1,blksize);
      ptr=(char *)zone;
      for (i=0;hdrlst[i].id!=(const char *)0;i++) {
        char * data;

        data="???";
        switch (hdrlst[i].num) {
          case 1        :
          case 2        :
          case 3        :
            data=tape->id[i];
            ptr+=sprintf(ptr,"%s:%s\n",hdrlst[i].id,data);
            break;
          case 4        :
            ptr+=sprintf(ptr,"%s:%u\n",hdrlst[i].id,tape->mediasize);
            break;
          case 5        :
            ptr+=sprintf(ptr,"%s:%u\n",hdrlst[i].id,tape->numsegment);
            break;
          default       :
            (void) rou_alert(0,"%s Unexpected header filed <%s> (Bug?)",
                                   OPEP,hdrlst[i].id);
            break;
          }
        if (strlen(zone)>(blksize-100)) {
          (void) rou_alert(1,"%s unable to fully store header "
                             "within block (Bug?)",OPEP);
          break;
          }
        }
      break;
    default     :
      proceed=false;
      break;
    }
  phase++;
  }
return zone;
#undef  OPEP
}
/*
^L
*/
/********************************************************/
/*                                                      */
/*	Procedure find if the tape is available for a   */
/*      new backup.                                     */
/*                                                      */
/********************************************************/
TAPTYP *tap_findavail(const char *media,TAPTYP *nominee,TAPTYP *challenger)

{
TAPTYP *tape;
int phase;
int proceed;

tape=nominee;
phase=0;
proceed=true;
while (proceed==true) {
  switch (phase) {
    case 0      :       //no tape data  ????
      if ((challenger==(TAPTYP *)0)||(media==(char *)0)) {
        (void) rou_alert(6,"Looking for a tape, but unknow media type (config!)");
        phase=999;      //no need to go further
        }
      break;
    case 1      :       //check if there is tape media match
      (void) rou_alert(1,"looking for a tape; <%s> media compatible",media);
      if (strcmp(media,challenger->media)!=0) 
        phase=999;      //no need to go further
      break;
    case 2      :       //tape already locked ??
      if (challenger->pidlock!=(pid_t)0)
        phase=999;      //process in progress, no need to go further
      break;
    case 3      :       //is the challenger still net to be kept
      time_t curdate;

      curdate=rou_normdate(time((time_t *)0));
      if (rou_caldate(challenger->lastused,0,challenger->frozen)>=curdate)
        phase=999;      //challenger tape still with a valuable backup
      break;
    case 4      :       //still inside the to be kept?
      if (tape==(TAPTYP *)0) {
        tape=challenger;
        (void) rou_alert(2,"Tape <%s> is now selected as media",tape->id[0]);
        phase=999;      //challenger is the nominee
        }
      break;
    case 5      :       //ok Good tape
      if (nominee->cycled>challenger->cycled) {
        (void) rou_alert(2,"Tape <%s> is found better than tape <%s>",
                           challenger->id[0],tape->id[0]);
        tape=challenger;//less used tape selected
        phase=999;      //challenger is the nominee
        }
      break;
    default     :
      proceed=false;
      break;
    }
  phase++;
  }
return tape;
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
int tap_openunitap()

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
int tap_closeunitap()

{
int status;

status=0;
if (modopen==true) {
  (void) rou_closesubrou();
  modopen=false;
  }
return status;
}
