// vim: smarttab tabstop=8 shiftwidth=2 expandtab
/********************************************************/
/*							*/
/*	Module to manage backup on tape device          */
/*							*/
/********************************************************/
#include        <sys/stat.h>
#include        <unistd.h>
#include        <stdlib.h>
#include        <string.h>
#include        <time.h>

#include	"subrou.h"
#include	"uniprc.h"
#include	"gesbck.h"

//sequence to backup and extract data on tape
#define TOSTORE "cpio -L -H newc -o --quiet 2>/dev/null | gzip -cf"
#define TOLIST  "gunzip -cf | cpio -ivt --quiet"
#define SPLDIR  "/var/spool/"APPNAME

static  int modopen=false;      //boolean module open/close
/*

*/
/********************************************************/
/*							*/
/*	Procedure to build the spool directory          */
/*							*/
/********************************************************/
static char *dospool(char *tapename)

{
#define SPSIZE  100

char *spool;
char cmd[SPSIZE + 50];

spool=(char *)calloc(1,SPSIZE);
(void) snprintf(spool,SPSIZE,"%s%s/%s",rootdir,SPLDIR,tapename);
(void) snprintf(cmd,sizeof(cmd),"mkdir -p %s",spool);
if (system(cmd)!=0) 
  (void) rou_alert(0,"Unable to create <%s> dir (System!?)",spool);
return spool;
#undef  SPSIZE
}
/*

*/
/********************************************************/
/*							*/
/*	Subroutine to return a string with the   	*/
/*	find sequencefor included/exclude file		*/
/*							*/
/********************************************************/
static char *formatcmd(const char *findtmpl,char *included,char *excluded)

{
char *inlist;
char *outlist;
char *selection;

inlist=(char *)calloc(1,20);
outlist=(char *)calloc(1,20);
selection=(char *)calloc(1,20);
if (included!=(char *)0) {
  char *ptr;

  ptr=included;
  while (*ptr!='\000') {
    char *nxtptr;

    if (*ptr=='/')
      ptr++;
    nxtptr=strchr(ptr,' ');
    if (nxtptr!=(char *)0)
      *nxtptr='\000';
    inlist=(char *)realloc(inlist,strlen(inlist)+strlen(ptr)+10);
    (void) strcat(inlist," ");
    (void) strcat(inlist,ptr);
    if (nxtptr==(char *)0)
      break;
    nxtptr++;
    ptr=nxtptr;
    }
  }
if (excluded!=(char *)0) {
  char *ptr;

  ptr=excluded;
  while (*ptr!='\000') {
    char *nxtptr;

    if (*ptr=='/')
      ptr++;
    nxtptr=strchr(ptr,' ');
    if (nxtptr!=(char *)0)
      *nxtptr='\000';
    outlist=(char *)realloc(outlist,strlen(outlist)+strlen(ptr)+40);
    if (strlen(outlist)>0) 	
      (void) strcat(outlist," -o ");
    (void) strcat(outlist,"-path \"");
    (void) strcat(outlist,ptr);
    (void) strcat(outlist,"\"");
    if (nxtptr==(char *)0)
      break;
    nxtptr++;
    ptr=nxtptr;
    }
  if (strlen(outlist)>0)
    (void) strcat(outlist," -prune -o");
  }
selection=(char *)realloc(selection,strlen(inlist)+strlen(outlist)+strlen(findtmpl)+20);
(void) sprintf(selection,findtmpl,inlist,outlist);
(void) free(inlist);
(void) free(outlist);
return selection;
}
/*
^L
*/
/********************************************************/
/*                                                      */
/*	Procedure to merge 2 strings sequence           */
/*      seq1=seq1+seq2;                                 */
/*                                                      */
/********************************************************/
static char *mergeseq(char *seq1,char **seq2,const char *start)

{
if ((seq1!=(char *)0)&&(seq2!=(char **)0)) {
  while (*seq2!=(char *)0) {
    int taille;

    taille=strlen(start)+strlen(seq1)+strlen(*seq2);
    if (taille>0) {
      char *newinc; 

      newinc=(char *)calloc(taille+3,sizeof(char));
      if (strlen(seq1)==0)
        (void) sprintf(newinc,"%s%s",start,*seq2);
      else
        (void) sprintf(newinc,"%s %s%s",seq1,start,*seq2);
      (void) free(seq1);
      seq1=newinc;
      }
    seq2++;
    }
  }
return seq1;
}
/*
^L
*/
/********************************************************/
/*                                                      */
/*	Procedure prepare the search sequence           */
/*      used to collect list of file to be within the   */
/*      backup.                                         */
/*                                                      */
/********************************************************/
static char *getsellist(const char *cmd,BCKTYP *tobackup)

{
#define NUMADD  3

char *search;
char *toadd[NUMADD];
int phase;
int proceed;

search=(char *)0;
(void) memset(toadd,'\000',sizeof(toadd));
phase=0;
proceed=true;
while (proceed==true) {
  switch (phase) {
    case 0      :       //is tobackup empty???
      if (tobackup==(BCKTYP *)0) 
        phase=999;      //no need to go further
      break;
    case 1      :       //collecting sequence
      toadd[0]=mergeseq(strdup(""),tobackup->included,rootdir);
      toadd[1]=mergeseq(strdup(""),tobackup->excluded,"");
      break;
    case 2      :       //merge included and excluded sequence
      search=formatcmd(cmd,toadd[0],toadd[1]);
      (void) free(toadd[1]);
      (void) free(toadd[0]);
      break;
    default     :       //task completed
      proceed=false;
      break;
    }
  phase++;
  }
return search;
#undef  NUMADD
}
/*
^L
*/
/********************************************************/
/*                                                      */
/*	Procedure to proceed with backup search         */
/*      and collect resulting data.                     */
/*                                                      */
/********************************************************/
static int backuppipe(THANDLE *handle,BCKTYP *bck,char *search)

{
#define OPEP    "gesbck.c:backuppipe"
int status;
FILE *fin;
FILE *fout;
char *inseq;
char *outseq;
int phase;
int proceed;

status=false;
fin=(FILE *)0;
fout=(FILE *)0;
inseq=(char *)0;
outseq=(char *)0;
phase=0;
proceed=true;
while (proceed==true) {
  switch (phase) {
    case 0      :       //make sure no trouble
      if ((search==(char *)0)||(strlen(search)==0)) {
        (void) rou_alert(0,"%s, search sequence is missing (bug!!!)",OPEP);
        phase=999;      //trouble trouble
        }
      break;
    case 1      :       //set the input pipe channel
      inseq=(char *)calloc(1,strlen(search)+strlen(TOSTORE)+20);
      (void) sprintf(inseq,"(%s)|%s",search,TOSTORE);
      if ((fin=prc_openpipe(inseq,"r"))==(FILE *)0) {
        (void) rou_alert(0,"input Pipe Trouble, Aborting backup");
        phase=999;      //trouble trouble
        }
      break;
    case 2      :       //set the output/result  pipe channel 
      char *spoolfile;

      spoolfile=dospool(bck->tapename);
      outseq=(char *)calloc(1,strlen(TOLIST)+strlen(spoolfile)+50);
      (void) sprintf(outseq,"%s > %s/%s-%03d",TOLIST,spoolfile,
                                              bck->tapename,bck->slot);
      if ((fout=prc_openpipe(outseq,"w"))==(FILE *)0) {
        (void) rou_alert(0,"output Pipe Trouble, Aborting backup");
        inseq=prc_closepipe(fin,inseq);
        phase=999;      //trouble trouble
        }
      (void) free(spoolfile);
      break;
    case 3      :       //collecting data
      int taken;
      TIMESPEC start;
      char *buffer;
      register u_int blksize;

      status=true;
      taken=0;
      (void) rou_getchrono(&start,(TIMESPEC *)0);
      bck->blkstart=dev_getcurpos(handle);
      blksize=dev_getblksize(handle);
      buffer=calloc(sizeof(char),blksize);
      while ((taken=fread(buffer,1,blksize,fin))>0) {
        int got;

        if ((got=dev_write(handle,buffer,taken))!=taken) {
          (void) rou_alert(0,"%s (got=%d) Unable to store data on tape "
                             "(error=<%s> Aborting backup",
                             OPEP,got,strerror(errno));
          break;
          }
        bck->cnumber+=(ul64)taken;
        bck->blknum++;
        if (fwrite(buffer,1,taken,fout)<0) {
          (void) rou_alert(0,"%s Unable to store data on tape index"
                            "(error=<%s> Aborting backup",OPEP,strerror(errno));
          status=false;
          break;
          }
        (void) memset(buffer,'\000',blksize);
        }
      (void) free(buffer);
      (void) rou_getchrono(&(bck->duration),&start);
      break;
    case 4      :       //closing pipe
      inseq=prc_closepipe(fout,inseq);
      outseq=prc_closepipe(fin,outseq);
      break;
    default     :       //task terminated
      proceed=false;
      break;
    }
  phase++;
  }
if (outseq!=(char *)0)
  (void) free(outseq);
if (inseq!=(char *)0)
  (void) free(inseq);
return status;
#undef  OPEP
}
/*
^L
*/
/********************************************************/
/*                                                      */
/*	Procedure to to add the footer header to the    */
/*      backup device.                                  */
/*                                                      */
/********************************************************/
static BCKTYP *addonefile(THANDLE *handle,TAPTYP *tape,char *filename)

{
#define OPEP    "gesbck.c:addonefile"

BCKTYP *resbck;
TIMESPEC chrono;
FILE *fichier;
int phase;
int proceed;

resbck=(BCKTYP *)0;
chrono.tv_sec=(time_t)0;
chrono.tv_nsec=(long)0;
fichier=(FILE *)0;
phase=0;
proceed=true;
while (proceed==true) {
  switch (phase) {
    case 0      :       //do we have a device open.
      if (handle==(THANDLE *)0)
        phase=999;      //no need to add listing file to backup
      break;
    case 1      :       //opening file to be add to backup
      if ((fichier=fopen(filename,"r"))==(FILE *)0) {
        (void) rou_alert(0,"%s Unable to open <%s> file (error=<%s> Bug System!?)",
                           OPEP,filename,strerror(errno));
        phase=999;      //trouble trouble
        }
      break;
    case 2      :       //prepare bck info
      char included[100];

      (void) rou_getchrono(&chrono,(TIMESPEC *)0);
      (void) memset(included,'\000',sizeof(included));
      (void) snprintf(included,sizeof(included)-1,"%s.ftr",tape->id[0]);
      resbck=(BCKTYP *)calloc(1,sizeof(BCKTYP));
      resbck->report=rpt_footer;
      resbck->included=(char **)rou_addlist((void **)0,(void *)strdup(included));
      break;
    case 3      :       //prepare bck info
      int blksize;
      int lu;
      char *memblk;

      blksize=dev_getblksize(handle);
      resbck->blkstart=dev_getcurpos(handle);
      resbck->blknum=1;
      memblk=(char *)calloc(1,blksize);
      while ((lu=fread(memblk,1,blksize,fichier))>0) {
        int stored;

        resbck->cnumber+=lu;
        if ((stored=dev_write(handle,memblk,lu))!=lu) {
          (void) rou_alert(0,"%s, stored partial footer, "
                             "stored only '%lu', wanted='%lu'",
                             OPEP,stored,lu);
          break;
          }
        (void) memset(memblk,'\000',blksize);
        resbck->blknum++;
        }
      (void) free(memblk);
      (void) fclose(fichier);
      break;
    case 4      :       //set EOF marker on file
      resbck->slot=dev_getfilenum(handle);
      (void) dev_setmark(handle);
      break;
    default     :
      proceed=false;
      break;
    }
  phase++;
  }
return resbck;
#undef  OPEP
}
/*
^L
*/
/********************************************************/
/*                                                      */
/*	Procedure to generate the tape information      */
/*      within the footer file.                         */
/*                                                      */
/********************************************************/
static void footer_head(FILE *fichier,TAPTYP *tape,ul64 blksize)

{
u_long releasedate;

releasedate=rou_caldate(rou_normdate(tape->stamp),0,tape->frozen);
(void) fprintf(fichier,"TapeID:\t\t\t\t%33s\n",tape->id[0]);
(void) fprintf(fichier,"Lastused:\t\t\t%33s\n",rou_getstrdate(tape->lastused));
(void) fprintf(fichier,"\n");
(void) fprintf(fichier,"Previous TapeID:\t\t%33s\n",tape->id[1]);
(void) fprintf(fichier,"Next TapeID:\t\t\t%33s\n",tape->id[2]);
(void) fprintf(fichier,"\n");
(void) fprintf(fichier,"Tape Stamp:\t\t\t%33s\n",rou_getstrfulldate(tape->stamp));
(void) fprintf(fichier,"Keep up to:\t\t\t%33s\n",rou_getstrdate(releasedate));
(void) fprintf(fichier,"\n");
(void) fprintf(fichier,"Tape size:\t\t\t% 26d MBytes\n",tape->mediasize);
(void) fprintf(fichier,"tape block size:\t\t% 26d Kbytes\n",(unsigned int)(blksize/1024));
(void) fprintf(fichier,"Cycled time:\t\t\t% 26d  Times\n",tape->cycled);
(void) fprintf(fichier,"Backd version used:\t\t%33s\n",tape->vers);
(void) fprintf(fichier,"\n");
(void) fprintf(fichier,"Storing sequence used:\n\t%57s\n",TOSTORE);
(void) fprintf(fichier,"Listing sequence to use:\n\t%57s\n",TOLIST);
(void) fprintf(fichier,"\n");
(void) fprintf(fichier,"Tape contents\n");
}
/*
^L
*/
/********************************************************/
/*                                                      */
/*	Procedure store a file with the list of all     */
/*      backup done on tape.                            */
/*      return true if this successfull, false otherwise */
/*                                                      */
/********************************************************/
BCKTYP *bck_dolist(int toadd,THANDLE *handle,TAPTYP *tape,BCKTYP **bcklist)

{
#define OPEP            "gesbck.c:bck_dolist"

#define STRFORMAT       "%3s%9s%12s%14s%15s    %s\n"
#define ENTFORMAT       "%3d%9s%12llu%14s%15s    %s\n"


BCKTYP *resbck;
FILE *fichier;
TIMESPEC chrono;
char *filename;
ul64 total;
ul64 blocks;
ul64 blksize;
int slot;
int phase;
int proceed;

resbck=(BCKTYP *)0;
fichier=(FILE *)0;
total=(ul64)0;
blocks=(ul64)0;
blksize=dev_getblksize(handle);
chrono.tv_sec=(time_t)0;
chrono.tv_nsec=(long)0;
filename=(char *)0;
slot=0;
phase=0;
proceed=true;
while (proceed==true) {
  switch (phase) {
    case 0      :       //check if everything fine
      if ((tape==(TAPTYP *)0)||(bcklist==(BCKTYP **)0)) {
        (void) rou_alert(0,"%s, tape or backup list missing (bug!!!)",OPEP);
        phase=999;      //trouble trouble.
        }
      break;
    case 1      :       //opening file
      char *spoolfile;

      spoolfile=dospool(tape->id[0]);
      filename=(char *)calloc(1,strlen(spoolfile)+50);
      (void) sprintf(filename,"%s/%s-footer",spoolfile,tape->id[0]);
      if ((fichier=fopen(filename,"w"))==(FILE *)0) {
        (void) rou_alert(0,"%s, Unable to open file <%s> (error=<%s>) "
                           "((system trouble?))",OPEP,strerror(errno));
        phase=999;      //trouble trouble.
        }
      (void) free(spoolfile);
      break;
    case 2      :       //store header
      (void) footer_head(fichier,tape,blksize);
      (void) fprintf(fichier,STRFORMAT,"asf","Type","Block pos",
                                       "Duration","Byte size","Included");
      break;
    case 3      :       //store block information
      while (*bcklist!=(BCKTYP *)0) { 
        static  char *ttype[]={"header","data","footer"};
        char strnumber[20];
        char **ptr;

        (void) rou_addchrono(&chrono,&((*bcklist)->duration));
        total+=(*bcklist)->cnumber;
        blocks+=(*bcklist)->blknum;
        (void) bck_bytesformat(strnumber,sizeof(strnumber),(*bcklist)->cnumber);
        (void) fprintf(fichier,ENTFORMAT,(*bcklist)->slot,
                                         ttype[(*bcklist)->report], 
                                         ((*bcklist)->blkstart)/blksize,
                                         rou_getstrchrono(&((*bcklist)->duration)),
                                         strnumber,
                                         *((*bcklist)->included));
        ptr=(*bcklist)->included+1;
        while (*ptr!=(char *)0) {
          (void) fprintf(fichier,"%56s %s\n","+",*ptr);
          ptr++;
          }
        bcklist++;
        slot++;
        }
      break;
    case 4      :       //store footer
      char strnumber[20];
      ul64  availsize;

      (void) bck_bytesformat(strnumber,sizeof(strnumber),total);
      //mediasize is in Mbytes Unit while blksize is in Unit
      //set noth mediasize and blksize in Kbytes Unit
      availsize=(tape->mediasize*1024)/(blksize/1024);
      (void) fprintf(fichier,"\n");
      (void) fprintf(fichier,"\tTotal process %16s%15s\n",
                              rou_getstrchrono(&chrono),strnumber);
      (void) fprintf(fichier,"\tBlock Used\t %6llu/%6llu\t% 10.2f %%\n",
                              blocks,availsize,((float)blocks/availsize)*100);
      break;
    case 5      :       //close file and (if needed) store list.
      (void) fclose(fichier);
      if (toadd==true)
        resbck=addonefile(handle,tape,filename);
      break;
    default     :
      proceed=false;
      break;
    }
  phase++;
  }
if (filename!=(char *)0) 
  (void) free(filename);
return resbck;;
#undef  ENTFORMAT
#undef  STRFORMAT
#undef  OPEP
}
/*
^L
*/
/********************************************************/
/*                                                      */
/*	Procedure store a set of file within a specifice*/
/*      (tape) device.                                  */
/*      return true if all successfull, false otherwise */
/*                                                      */
/********************************************************/
int bck_dobackup(TAPTYP *tape,THANDLE *handle,BCKTYP **tobackup)

{
#define OPEP    "gesbck.c:bck_tobackup"
#define FINDSEQ "find %s -follow -mount %s -print 2>/dev/null | sort -d"

int status;
int slot;
BCKTYP **todo;
char *search;
int phase;
int proceed;

status=false;           //lets say something wrong
todo=tobackup;
search=(char *)0;
slot=0;
phase=1;
proceed=true;
while (proceed==true) {
  switch (phase) {
    case 0      :       //checking if everything fine
      if ((tape==(TAPTYP *)0)||(handle==(THANDLE *)0)||(todo==(BCKTYP **)0)) {
        (void) rou_alert(0,"%s Aborting backup, tape, device or "
                           "backup list undefined (Bug\?\?\?)",OPEP);
        phase=999;      //no need to go further
        }
      break;
    case 1      :       //a backup still to do?
      if (*todo==(BCKTYP *)0) 
        phase=999;      //backup terminated
      break;
    case 2      :       //*checking if to is data backup segment
      if ((*todo)->report!=rpt_data) {
        todo++;         //next backup
        phase=0;        //check next one
        }
      break;
    case 3      :       //*setting backup information
      slot++;
      (*todo)->tapename=strdup(tape->id[0]);
      (*todo)->blkstart=0;
      (*todo)->blknum=1;        //MEOF marker is one block
      (*todo)->slot=slot;
      (*todo)->cnumber=(ul64)0;
      break;
    case 4      :       //*doing one backup
      if ((search=getsellist(FINDSEQ,*todo))==(char *)0) 
        phase=999;      //no data to collect???
      break;
    case 5      :       //lets do backup one slot at the time
      if ((status=backuppipe(handle,*todo,search))==false) 
        phase=999;      //proble storing backup
      (void) free(search);
      break;
    case 6      :       //next backup?
      (void) dev_setmark(handle);
      todo++;
      phase=0;          //another backup todo
      break;
    default     :       //all backup done
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
/*                                                      */
/*	Procedure to "open" usager to module.           */
/*      homework purpose                                */
/*      return zero if everything right                 */
/*                                                      */
/********************************************************/
int bck_opengesbck()

{
int status;

status=0;
if (modopen==false) {
  (void) rou_opensubrou();
  (void) dev_openunidev();
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
int bck_closegesbck()

{
int status;

status=0;
if (modopen==true) {
  (void) bck_closeunibck();
  (void) dev_closeunidev();
  (void) rou_closesubrou();
  modopen=false;
  }
return status;
}
