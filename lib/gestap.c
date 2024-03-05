// vim: smarttab tabstop=8 shiftwidth=2 expandtab
/********************************************************/
/*							*/
/*	Module to manage tap device access              */
/*							*/
/********************************************************/
#include        <fcntl.h>
#include        <stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include        <unistd.h>

#include	"uniprc.h"
#include	"gestap.h"

static  int modopen;            //boolean module open/close
/*
^L
*/
/********************************************************/
/*							*/
/*      Procedure to check if the unit (device) is      */
/*      existing.                                       */
/*      Return zero if unit available                   */
/*							*/
/********************************************************/
static int deviceok(char *device)

{
#define OPEP    "gestap.c:deviceok,"

int status;
FILE *ftape;
char *tapename;
int phase;
int proceed;

status=-1;
ftape=(FILE *)0;
tapename=(char *)0;
phase=0;
proceed=true;
while (proceed==true) {
  switch (phase) {
    case 0      :       //do we have a unit
      if (device==(char *)0) {
        (void) rou_alert(2,"%s Unitname (tape device) is unknown!",OPEP);
        phase=999;      //No need to go further
        }
      break;
    case 1      :       //is the unit accessible
      tapename=rou_apppath(device);
      if ((ftape=fopen(tapename,"r"))==(FILE *)0) {
        (void) rou_alert(2,"%s Unable to open unit <%s> (error=<%s)",
                           OPEP,tapename,strerror(errno));
        (void) free(tapename);
        phase=999;      //No need to go further
        }
      break;
    case 2      :       //do we have at least the tape header block
      char *header;

      header=calloc(1,sizeof(TAPTYP));
      if (fread(header,sizeof(TAPTYP),1,ftape)!=1) {
        (void) rou_alert(1,"%s unable to access tape <%s> header space "
                          "(error=<%s>)", OPEP,tapename,strerror(errno));
        (void) fclose(ftape);
        phase=999;              //no need to go further
        }
      (void) free(header);
      (void) free(tapename);
      break;
    case 3      :       //everything is fine, closing file      
      (void) fclose(ftape);
      status=0;
      break;
    default     :
      proceed=false;
      break;
    }
  phase++;
  }
return status;
#undef  OPEP
}
/*
^L
*/
/********************************************************/
/*							*/
/*      Procedure to read the device header releated    */
/*      to a device.                                    */
/*      Return a tape structure or NULL                 */
/*							*/
/********************************************************/
TAPTYP *readheader(char *devname,u_int blksize)

{
#define OPEP    "gestap.c:readheader"
TAPTYP *tape;
THANDLE *handle;
int phase;
int proceed;

tape=(TAPTYP *)0;
handle=(THANDLE *)0;
phase=0;
proceed=true;
while (proceed==true) {
  switch (phase) {
    case 0      :       //checking if devref exist
      if (devname==(char *)0) {
        (void) rou_alert(0,"%s device name missing! (bug?)",OPEP);
        phase=999;      //trouble trouble
        }
      break;
    case 1      :       //opening the device
      if ((handle=dev_open(devname,blksize,O_RDONLY|O_EXCL))==(THANDLE *)0) {
        phase=999;      //trouble trouble
        }
      break;
    case 2      :       //reading tape
      tape=tap_readheader(handle,blksize);
      break;
    case 3      :       //closing device
      handle=dev_close(handle,false);
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

*/
/********************************************************/
/*							*/
/*	Subroutine to generate a tape definition entry, */
/*      discriminate comment part from the datapart     */
/*      within the line                                 */
/*							*/
/********************************************************/
static LSTTYP *gettapeinfo(char *str)

{
LSTTYP *entry;
register char *comment;

entry=(LSTTYP *)calloc(1,sizeof(LSTTYP));
if ((comment=strchr(str,'#'))!=(char *)0) {
  *comment='\000';
  comment++;
  }
if (strlen(str)>0)
  entry->tapedata=tap_strtotape(str);
if (comment!=(char *)0)
  entry->comment=strdup(comment);
return entry;
}
/*
^L
*/
/********************************************************/
/*							*/
/*      Procedure to scan a tapelist file keeping       */
/*      comments and parsing tape description if needed */
/*      return a tape structure                         */
/*							*/
/********************************************************/
static LSTTYP **scanliste(FILE *fichier)

{
LSTTYP **list;
char line[3000];

list=(LSTTYP **)0;
while (rou_getstr(fichier,line,sizeof(line)-1,true,'#')!=(char *)0) {
  LSTTYP *entry;

  if ((entry=gettapeinfo(line))!=(LSTTYP *)0) {
    list=(LSTTYP **)rou_addlist((void **)list,(void *)entry);
    }
  }
return list;
}
/*
^L
*/
/********************************************************/
/*                                                      */
/*	Procedure to display tape ID set on specific    */
/*      device                                          */
/*                                                      */
/********************************************************/
static void online(FILE *fileout,DEVTYP *dev)

{
#define OPEP    "gestap.c:online"

THANDLE *handle;
TAPTYP *tape;
int phase;
int proceed;

handle=(THANDLE *)0;
tape=(TAPTYP *)0;
phase=0;
proceed=true;
while (proceed==true) {
  switch(phase) {
    case 0      :       //Check if device
      if (dev==(DEVTYP *)0) {
        (void) rou_alert(0,"%s No device pointer, aborting listing (Bug?)",OPEP);
        phase=999;
        }
      break;
    case 1      :       //opening device
      if ((handle=dev_open(dev->devname,dev->blksize,O_RDONLY))==(THANDLE *)0) {
        (void) rou_alert(1,"%s Unable to open device <%s>",OPEP,dev->devname);
        phase=999;
        }
      break;
    case 2      :       //reading tape header
      if ((tape=tap_readheader(handle,dev->blksize))==(TAPTYP *)0) {
        phase=999;      //no tape inserted?
        }
      break;
    case 3      :       //display tape number
      (void) fprintf(fileout,"%s\t%s\t%s\n",dev->media,dev->devname,tape->id[0]);
      tape=tap_freetape(tape);
      break;
    default     :       //task completed
      (void) dev_close(handle,false);
      proceed=false;
      break;
    }
  phase++;
  }
#undef  OPEP
}
/*
^L
*/
/********************************************************/
/*							*/
/*      Procedure to free all memory used by one tape   */
/*      list entry.                                     */
/*							*/
/********************************************************/
LSTTYP *tap_freeentry(LSTTYP *entry)

{
if (entry!=(LSTTYP *)0) {
  entry->tapedata=tap_freetape(entry->tapedata);
  if (entry->comment!=(char *)0)
    (void) free(entry->comment);
  (void) free(entry);
  entry=(LSTTYP *)0;
  }
return entry;
}
/*
^L
*/
/********************************************************/
/*							*/
/*      Procedure to lock exclusive access to tape list.*/
/*      return lock status.                             */
/*							*/
/********************************************************/
int tap_locktapelist(const char *filename,int lock,int tentative)

{
if (filename==(char *)0) 
  filename=TAPES"_lock";
return prc_locking(filename,lock,tentative);
}
/*
^L
*/
/********************************************************/
/*							*/
/*      Procedure to lock exclusive access to a tape    */
/*      device (to allow a secure write to tape).       */
/*      return lock status.                             */
/*							*/
/********************************************************/
int tap_lockdevice(DEVTYP *device,int lock)

{
int done;
done=false;
if ((device!=(DEVTYP *)0)&&(device->devname!=(char *)0)) {
  done=prc_locking(device->devname,lock,10);
  }
return done;
}
/*
^L
*/
/********************************************************/
/*							*/
/*      Procedure to add a tape definition to a         */
/*      tapeliste.                                      */
/*							*/
/********************************************************/
LSTTYP **tap_addtolist(LSTTYP **list,TAPTYP *tape)

{
if (tape!=(TAPTYP *)0) {
  LSTTYP *toadd;
  char comment[50];

  toadd=(LSTTYP *)calloc(1,sizeof(LSTTYP));
  (void) sprintf(comment,"Label Stamp: %s",rou_getstrfulldate(time((time_t *)0)));
  toadd->comment=strdup(comment);
  toadd->tapedata=(TAPTYP *)calloc(1,sizeof(TAPTYP));
  (void) memcpy(toadd->tapedata,tape,sizeof(TAPTYP));
  list=(LSTTYP **)rou_addlist((void **)list,(void *)toadd);
  }
return list;
}
/*
^L
*/
/********************************************************/
/*							*/
/*	procedure to initiat a clean tap structure.     */
/*							*/
/********************************************************/
TAPTYP *tap_inittape(char *label,ARGTYP *params)

{
TAPTYP *tape;

tape=tap_newtape();
if (params->device!=(char *)0)
  (void) strncpy(tape->device,params->device,sizeof(tape->device)-1);
if (params->media!=(char *)0) 
  (void) strncpy(tape->media,params->media,sizeof(tape->media)-1);
if (params->mediasize>0)
  tape->mediasize=params->mediasize;
return tape;
}
/*
^L
*/
/********************************************************/
/*							*/
/*      Procedure to write an header on a "new" tape    */
/*							*/
/********************************************************/
TAPENUM tap_initheader(TAPTYP *tape,u_int blksize)

{
#define OPEP    "gestap.c:tap_initheader,"
TAPENUM status;
char *lockname;
THANDLE *handle;
int phase;
int proceed;

status=tap_trouble;
lockname=(char *)0;
handle=(THANDLE *)0;
phase=0;
proceed=true;
while (proceed==true) {
  switch (phase) {
    case 0      :
      if ((tape==(TAPTYP *)0)||(deviceok(tape->device)!=0)) {
        status=tap_nodevice;
        phase=999;              //no need to go further
        }
      break;
    case 1      :               //setting local lockname
      if ((lockname=strrchr(tape->device,'/'))==(char *)0)
        lockname=tape->device;
      else 
        lockname++;
      break;
    case 2      :               //locking device
      if (prc_locking(lockname,LCK_LOCK,10)==false) {
        (void) rou_alert(1,"%s unable to lock device <%s> in due time, ",
                           "another process still running?",OPEP,tape->device);
        status=tap_nolock;
        phase=999;              //no need to further
        }
      break;
    case 3      :               //opening the tape device
      if ((handle=dev_open(tape->device,blksize,O_RDWR|O_EXCL))==(THANDLE *)0) {
        (void) prc_locking(lockname,LCK_UNLOCK,1);
        status=tap_nodevice;
        phase=999;              //no need to go further
        }
      break;
    case 4      :               //writing|updating the tape device
      if (tap_writeheader(handle,tape,blksize)>0)
        status=tap_ok;
      (void) dev_close(handle,false);
      break;
    case 5      :               //unlocking tape device
      (void) prc_locking(lockname,LCK_UNLOCK,1);
      break;
    default     :
      proceed=false;
      break;
    }
  phase++;
  }
return status;
#undef  OPEP
}
/*
^L
*/
/********************************************************/
/*							*/
/*      Procedure to write/update an header on tape     */
/*							*/
/********************************************************/
int tap_writeheader(THANDLE *handle,TAPTYP *tape,u_int blksize)

{
#define OPEP    "gestap.c:tap_writeheader,"

int status;
char *header;
int phase;
int proceed;

status=0;
header=(char *)0;
phase=0;
proceed=true;
while (proceed==true) {
  switch (phase) {
    case 0      :
      if ((tape==(TAPTYP *)0)||(handle)==(THANDLE *)0) {
        status=tap_nodevice;
        phase=999;              //no need to go further
        }
      break;
    case 1      :               //converting header to block
      if ((header=tap_tapetoheader(tape,blksize))==(char *)0) {
        (void) rou_alert(1,"%s Unable to convert tape header (bug?)",OPEP);
        status=-1;
        phase=999;              //no need to go further
        }
      break;
    case 2      :               //writing header to tape device
      char *block;

      block=(char *)calloc(1,blksize);
      (void) memcpy(block,header,strlen(header));
      (void) dev_move(handle,mov_begin,0);
      status=strlen(header);
      if (dev_write(handle,block,blksize)<0) {
        (void) rou_alert(1,"%s Unable to write tape device (error=<%s>)",
                            OPEP,strerror(errno));
        status=-1;
        phase=999;              //no need to go further
        }
      (void) free(block);
      break;
    default     :
      proceed=false;
      break;
    }
  phase++;
  }
if (header!=(void *)0)
  (void) free(header);
return status;
#undef  OPEP
}
/*
^L
*/
/********************************************************/
/*							*/
/*      Procedure to read tape header currently on      */
/*      device                                          */
/*							*/
/********************************************************/
TAPTYP *tap_readheader(THANDLE *handle,u_int blksize)

{
#define OPEP    "gestap.c:tap_readheader,"
TAPTYP *tape;
void *header;
int phase;
int proceed;

tape=(TAPTYP *)0;
header=(void *)0;
phase=0;
proceed=true;
while (proceed==true) {
  switch (phase) {
    case 0      :               //device ready
      if (handle==(THANDLE *)0) {
        (void) rou_alert(1,"%s tape device not open (bug?)",OPEP);
        phase=999;              //no need to go further
        }
      break;
    case 1      :               //reading tape header
      u_int received;
      const char *devname;

      header=calloc(1,blksize);
      devname=dev_getdevname(handle);
      (void) dev_move(handle,mov_begin,0);
      received=dev_read(handle,(const char *)header,blksize);
      if (received!=blksize) {
        (void) rou_alert(1,"%s Unable to fully read tape header on device <%s> "
                            "(received=%lu versus blksize=%lu)",
                            OPEP,devname,received,blksize);
        phase=999;              //no need to go further
        }
      break;
    case 2      :               //converting header to tape
      if ((tape=tap_headertotape(header))==(TAPTYP *)0) {
        (void) rou_alert(1,"%s tape header wrong format",OPEP);
        phase=999;              //no need to go further
        }
      (void) free(header);
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
/*      Procedure to build a tape list extracted from   */
/*      file configuration list                         */
/*							*/
/********************************************************/
LSTTYP **tap_readtapefile(const char *filename)

{
#define OPEP    "gestap.c:tap_readtapefile,"

LSTTYP **list;
char *fname;
FILE *fichier;
int phase;
int proceed;

list=(LSTTYP **)0;
fname=(char *)0;
fichier=(FILE *)0;
phase=0;
proceed=true;
while (proceed==true) {
  switch (phase) {
    case 0      :               //building filename
      if (filename==(char *)0)
        filename=TAPES;
      if ((fname=rou_apppath(filename))==(char *)0) {
        (void) rou_alert(1,"%s Unable to get the tapelist filename (bug?)",OPEP);
        phase=999;              //no need to go further
        }
      break;
    case 1      :               //Is the file existing
      if ((fichier=fopen(fname,"r"))==(FILE *)0) {
        (void) rou_alert(0,"%s Unable to read tape list <%s> (error=<%s>)",
                            OPEP,fname,strerror(errno));
        phase=999;              //no need to go further
        }
      break;
    case 2      :               //building list
      if ((list=scanliste(fichier))==(LSTTYP **)0)
        (void) rou_alert(1,"%s Unable to scan tape list <%s> (config format?)",
                            OPEP,fname);
      break;
    case 3      :               //closing file
      (void) fclose(fichier);
      (void) free(fname);
      break;
    default     :               //exiting
      proceed=false;
      break;
    }
  phase++;
  }
return list;
#undef  OPEP
}
/*
^L
*/
/********************************************************/
/*                                                      */
/*	Procedure to read a device and check if the      */
/*      tape header match the requested tapeid.         */
/*                                                      */
/********************************************************/
int tap_in_device(DEVTYP *devref,TAPTYP *used)

{
#define OPEP    "gestap.c:tap_in_device"
int present;
TAPTYP *tape;
int phase;
int proceed;

present=false;
tape=(TAPTYP *)0;
phase=0;
proceed=true;
while (proceed==true) {
  switch (phase) {
    case 0      :       //do we have parameters
      if ((devref==(DEVTYP *)0)||(used==(TAPTYP *)0)) {
        (void) rou_alert(0,"%s missing devname or tapeid (Bug!)",OPEP);
        phase=999;      //no purpose to go further
        }
      break;
    case 1      :       //do we have parameters
      if ((tape=readheader(devref->devname,devref->blksize))==(TAPTYP *)0) 
        phase=999;      //no purpose to go further
      break;
    case 2      :       //reading the block device
      if (strcmp(tape->id[0],used->id[0])==0) {
        used->numsegment=tape->numsegment;
        used->mediasize=tape->mediasize;
        present=true;
        }
      tape=tap_freetape(tape);
      break;
    default     :
      proceed=false;
      break;
    }
  phase++;
  }
return present;
#undef  OPEP
}
/*
^L
*/
/********************************************************/
/*							*/
/*      Procedure to write of update a tapelist file    */
/*      with list of tape contents                      */
/*							*/
/********************************************************/
int tap_writetapefile(const char *filename,LSTTYP **list)

{
#define OPEP    "gestap.c:tap_witetapefile,"

int status;
char *fname;
FILE *fichier;
int phase;
int proceed;

status=-1;
phase=0;
proceed=true;
while (proceed==true) {
  switch (phase) {
    case 0      :               //building filename
      if (filename==(char *)0)
        filename=TAPES;
      if ((fname=rou_apppath(filename))==(char *)0) {
        (void) rou_alert(1,"%s Unable to get the tapelist filename (bug?)",OPEP);
        phase=999;              //no need to go further
        }
      break;
    case 1      :               //removing previous list
      if (fname!=(char *)0) {   //always
        char *old;

        old=(char *)calloc(strlen(fname)+10,sizeof(char));
        (void) strcat(old,fname);
        (void) strcat(old,".prv");
        (void) unlink(old);     //remove previous tapelist
        if (link(fname,old)<0) {
          (void) rou_alert(0,"%s Unable to link file <%s> and <%s> "
                             "(bug?, error=<%s>)",
                              OPEP,fname,old,strerror(errno));
          phase=999;            //no need to go further
          }
        else
          (void) unlink(fname);
        (void) free(old);
        }
      break;
    case 2      :               //opening the file in writing mode
      if ((fichier=fopen(fname,"w"))<0) {
        (void) rou_alert(0,"%s Unable to open file <%s> (error=<%s>)",
                              OPEP,fname,strerror(errno));
        phase=999;              //no need to go further
        }
      break;
    case 3      :               //dumping the list content on file
      if (list!=(LSTTYP **)0) {
        register int num;

        num=0;
        while (list[num]!=(LSTTYP *)0) {
          char *data;

          if ((data=tap_tapetostr(list[num]->tapedata))!=(char *)0) {
            (void) fprintf(fichier,"%s\t",data);
            (void) free(data);
            }
          if (list[num]->comment!=(char *)0) 
            (void) fprintf(fichier,"#%s",list[num]->comment);
          (void) fprintf(fichier,"\n");
          num++;
          }
        }
      break;
    case 4      :               //closing opened tapelist file
      (void) fclose(fichier);
      status=0;
      break;
    default     :
      if (fname!=(char *)0)
        (void) free(fname);
      proceed=false;
      break;
    }
  phase++;
  }
return status;
#undef  OPEP
}
/*
^L
*/
/********************************************************/
/*							*/
/*      Procedure to return a tape reference extracted  */
/*      from a list of tape reference, selection is done*/
/*      according tape label.                           */
/*							*/
/********************************************************/
const TAPTYP *tap_findtape(LSTTYP **list,const char *label)

{
const TAPTYP *tape;

tape=(const TAPTYP *)0;
if ((list!=(LSTTYP **)0)&&(label!=(const char *)0)) {
  while (*list!=(LSTTYP *)0) {
    if ((*list)->tapedata!=(TAPTYP *)0) {
      if (strcmp((*list)->tapedata->id[0],label)==0) {
        tape=(*list)->tapedata;
        break;
        }
      }
    list++;
    }
  }
return tape;
}
/*
^L
*/
/********************************************************/
/*                                                      */
/*	Procedure to update record within the tapelist  */
/*      file.                                           */
/*      Return 0 if update went fine, -1 otherwise      */
/*                                                      */
/********************************************************/
int tap_updatetape(const char *filename,TAPTYP *used)

{
int status;
LSTTYP **tapelist;
int phase;
int proceed;

status=-1;
tapelist=(LSTTYP **)0;
phase=0;
proceed=true;
while (proceed==true) {
  switch (phase) {
    case 0      :       //check used status
      if (used==(TAPTYP *)0) {
        (void) rou_alert(0,"Cannot do backup; tape data missing (BAD bug!)");
        phase=999;      //trouble trouble
        }
      break;
    case 1      :       //check used status
      if (tap_locktapelist(filename,LCK_LOCK,3)==false) {
        (void) rou_alert(0,"Unable to lock config 'tapelist' (other daemon\?\?)");
        phase=999;      //no need to go further 
        }
      break;
    case 2      :       //check used status
      if ((tapelist=tap_readtapefile(filename))==(LSTTYP **)0) {
        (void) rou_alert(0,"Unable to read config 'tapelist' (config\?\?)");
        phase=999;      //no need to go further
        }
      break;
    case 3      :       //locate tape recorde to update
      LSTTYP **ptr;

      ptr=tapelist;
      while (*ptr!=(LSTTYP *)0) {
        register TAPTYP *data;

        data=(*ptr)->tapedata;
        if ((data!=(TAPTYP *)0)&&(strcmp(data->id[0],used->id[0])==0)) {
          (void) memcpy(data,used,sizeof(TAPTYP)); 
          status=0;
          break;
          }
        ptr++;
        }
      break;
    case 4      :       //Update tapelist file
      if (status==0) {
        if (tap_writetapefile(filename,tapelist)<0) {
          (void) rou_alert(0,"Unable to update 'tapelist' (Timing\?\?)");
          status=-1;
          }
        }
      break;
    case 5      :       //free the local tapelist
      tapelist=(LSTTYP **)rou_freelist((void **)tapelist,
                                       (freehandler_t)tap_freeentry);
      break;
    default     :
      (void) tap_locktapelist(filename,LCK_UNLOCK,1);
      proceed=false;
      break;
    }
  phase++;
  }
return status;
}
/*
^L
*/
/********************************************************/
/*                                                      */
/*	Procedure to find out which tape is available   */
/*      for backup.                                     */
/*                                                      */
/********************************************************/
TAPTYP *tap_isatapeready(SCHTYP *sch)


{
TAPTYP *tape;
LSTTYP **tapelist;
int phase;
int proceed;

tape=(TAPTYP *)0;
tapelist=(LSTTYP **)0;
phase=0;
proceed=true;
while (proceed==true) {
  switch (phase) {
    case 0      :       //locking tape
      if (tap_locktapelist((const char *)0,LCK_LOCK,3)==false) {
        (void) rou_alert(0,"Unable to lock config 'tapelist' (other daemon\?\?)");
        phase=999;      //no need to go further 
        }
      break;
    case 1      :       //loading tape list
      if ((tapelist=tap_readtapefile((const char *)0))==(LSTTYP **)0) {
        (void) rou_alert(0,"Unable to read config 'tapelist' (config\?\?)");
        phase=999;      //no need to go further
        }
      break;
    case 2      :       //selecting tape
      LSTTYP **ptr;

      ptr=tapelist;
      while (*ptr!=(LSTTYP *)0) {
        register TAPTYP *data;

        data=(*ptr)->tapedata;
        if (data!=(TAPTYP *)0) {
          if ((data->pidlock!=(pid_t)0)&&(prc_checkprocess(data->pidlock)==false))
            data->pidlock=(pid_t)0; //cleaning old lock
          tape=tap_findavail(sch->media,tape,data);
          }
        ptr++;
        }
      break;
    case 3      :       //set the tape entry as locked and update tapelist
      if (tape!=(TAPTYP *)0) {
        tape->pidlock=getpid();
        if (tap_writetapefile((const char *)0,tapelist)<0) {
          (void) rou_alert(0,"Unable to lock tape <%s> within tapelist (Bug?)",
                              tape->id[0]);
          tape=(TAPTYP *)0;
          phase=999;    //No need to go further
          }
        }
      break;
    case 4      :       //duplicate tape information to drive backup
      tape=tap_duplicate(tape);
      break;
    default     :
      (void) tap_locktapelist((const char *)0,LCK_UNLOCK,1);
      tapelist=(LSTTYP **)rou_freelist((void **)tapelist,
                                       (freehandler_t)tap_freeentry);
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
/*	Procedure to show (on line) tape footer         */
/*                                                      */
/********************************************************/
void tap_showdata(FILE *outfile,TAPTYP *tape,int segnum)

{
#define OPEP    "gestap.c:tap_showdata"

DEVTYP **devlist;
DEVTYP *found;
THANDLE *handle;
int phase;
int proceed;

devlist=(DEVTYP **)0;
found=(DEVTYP *)0;
handle=(THANDLE *)0;
phase=0;
proceed=true;
while (proceed==true) {
  switch (phase) {
    case 0        :       //find all device associated with a media
      if ((devlist=dev_selectdev((char *)0,tape->media))==(DEVTYP **)0) {
        (void) rou_alert(0,"Found no device for media <%s> (config?)",tape->media);
        phase=999;        //No need to go further
        }
      break;
    case 1        :       //we have a list of device
      DEVTYP **ptr;

      ptr=devlist;
      while (*ptr!=(DEVTYP *)0) {
        if (tap_in_device(*ptr,tape)==true) {
          found=*ptr;
          break;
          }
        ptr++;
        }
      if (found==(DEVTYP *)0) {
        (void) rou_alert(0,"tape=<%s> found NOT available",tape->id[0]);
        phase=999;        //No need to go further
        }
      break;
    case 2        :       //we have a list of device
      handle=dev_open(found->devname,found->blksize,O_RDONLY|O_EXCL);
      if (handle==(THANDLE *)0) {
        (void) rou_alert(0,"tape=<%s>, unable to open device <%s>?",
                            OPEP,tape->id[0],found->devname);
        phase=999;        //No need to go further
        }
      break;
    case 3        :       //rewind tape
      if (dev_move(handle,mov_begin,0)<0) {
        (void) rou_alert(0,"tape=<%s>, unable to rewind tape (tape damaged?)",
                            OPEP,tape->id[0]);
        phase=999;
        }
      break;
    case 4        :     //making sure we are reading a segment
      if (segnum==0)    //if segment=0 -> read tape footer
        segnum=tape->numsegment;
      if (segnum>tape->numsegment)      //no going beyon footer segment
        segnum=tape->numsegment;
      break;
    case 5        :     //reading tape and sending it to outfile
      (void) dev_getonesegment(outfile,handle,segnum);
      break;
    case 6        :     //reading tape and closing tape
      (void) dev_close(handle,false);
      break;
    default     :
      devlist=(DEVTYP **)rou_freelist((void **)devlist,(freehandler_t)dev_freedev);
      proceed=false;
      break;
    }
  phase++;
  }
#undef  OPEP
}
/*
^L
*/
/********************************************************/
/*                                                      */
/*	Procedure to display tape footer contents       */
/*      (used for recovery purpose if tape information  */
/*       are not available anymore)                     */
/*                                                      */
/********************************************************/
void tap_showtapefooter(FILE *outfile,const char *tapelist,const char *tapename)

{
#define OPEP    "gestap.c:tap_showtapefooter"

LSTTYP **tlist;
int phase;
int proceed;

tlist=(LSTTYP **)0;
phase=0;
proceed=true;
while (proceed==true) {
  switch (phase) {
    case 0      :               //do we have tape name
      if (tapename==(char *)0) {
        (void) rou_alert(0,"%s tape name missing! (bug?)",OPEP);
        phase=999;      //no need to go further
        }
      break;
    case 1      :       //locking access to tape list
      if (tap_locktapelist(tapelist,LCK_LOCK,3)==false) {
        (void) rou_alert(0,"Unable to lock config 'tapelist' (other daemon\?\?)");
        phase=999;      //no need to go further 
        }
      break;
    case 2      :       //check used status
      if ((tlist=tap_readtapefile(tapelist))==(LSTTYP **)0) {
        (void) rou_alert(0,"Unable to read config 'tapelist' (config\?\?)");
        phase=999;      //no need to go further
        }
      break;
    case 3      :       //check used status
      LSTTYP **ptr;

      ptr=tlist;
      while (*ptr!=(LSTTYP *)0) {
        register TAPTYP *loc;
        
        loc=(*ptr)->tapedata;
        if (loc!=(TAPTYP *)0) {
          if ((strcmp(loc->id[0],tapename)==0)&&(loc->media!=(char *)0)) {
            (void) tap_showdata(outfile,loc,0);
            break;
            }
          }
        ptr++;
        }
      break;
    default     :
      (void) tap_locktapelist(tapelist,LCK_UNLOCK,1);
      tlist=(LSTTYP **)rou_freelist((void **)tlist,(freehandler_t)tap_freeentry);
      proceed=false;
      break;
    }
  phase++;
  }
#undef  OPEP
}
/*
^L
*/
/********************************************************/
/*                                                      */
/*	Procedure to list tape inseted on devices       */
/*                                                      */
/********************************************************/
void tap_listtapeonline(FILE *fileout,const char *media)

{
if (media!=(const char *)0) {
  DEVTYP **devlist;

  if ((devlist=dev_selectdev((char *)0,media))!=(DEVTYP **)0) {
    DEVTYP **ptr;

    ptr=devlist;
    while (*ptr!=(DEVTYP *)0) {
      (void) online(fileout,*ptr);
      ptr++;
      }
    devlist=(DEVTYP **)rou_freelist((void **)devlist,(freehandler_t)dev_freedev);
    }
  }
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
int tap_opengestap()

{
int status;

status=0;
if (modopen==false) {
  (void) rou_opensubrou();
  (void) par_openunipar();
  (void) dev_openunidev();
  (void) tap_openunitap();
  (void) sch_openunisch();
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
int tap_closegestap()

{
int status;

status=0;
if (modopen==true) {
  (void) sch_closeunisch();
  (void) tap_closeunitap();
  (void) dev_closeunidev();
  (void) par_closeunipar();
  (void) rou_closesubrou();
  modopen=false;
  }
return status;
}
