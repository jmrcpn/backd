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
#include	"unibck.h"

static  int modopen;            //boolean module open/close

//definition of backup available entry
typedef enum    {
        cmd_included,           //included file
        cmd_excluded,           //excluded pattern
        cmd_level,              //backup level
        cmd_unknown
        }CMDCOD;

/*
^L
*/
/********************************************************/
/*                                                      */
/*	Procedure to parse backup command information   */
/*                                                      */
/********************************************************/
static  CMDCOD findcmd(char *command)

{
typedef struct  {
        const char *id;         //command string
        CMDCOD code;            //command code
        }CMDTYP;

//list of backup entry.
const CMDTYP cmd_list[]={
        {"included",cmd_included},
        {"excluded",cmd_excluded},
        {"level",cmd_level},
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

*/
/************************************************/
/*						*/
/*	procedure to display "Byte size" in unit*/
/*	as By, KB, MB, GB.			*/
/*						*/
/************************************************/
char *bck_bytesformat(char *strnumber,int maxsize,ul64 taille)


{
#define	K1	1024

static char units[]={'G','M','K',' '};
long park;
int proceed;
int phase;

park=K1*K1*K1;
phase=0;
proceed=true;
while (proceed==true) {
  switch (phase) {
    case 0	:	/*do we have GigaBytes	*/
    case 1	:	/*do we have MegaByte	*/
    case 2	:	/*do we have Kilobyte	*/
      if (taille>park) {
	float value;

	value=taille;
	value/=park;
        (void) snprintf(strnumber,maxsize,"% 10.2f %cB",value,units[phase]);
	phase=999;	/*No need to go further	*/
        }
      break;
    case 3	:	/*only Few Bytes	*/
      (void) snprintf(strnumber,maxsize,"%10llu %cB",taille,units[phase]);
      break;
    default	:	/*SAFE Guard		*/
      proceed=false;
      break;
    }
  park/=K1;
  phase++;
  }
return strnumber;
}
/*
^L
*/
/********************************************************/
/*                                                      */
/*	Procedure to scan a line, looking for file      */
/*      to be added to backup.                          */
/*      all needed  information                         */
/*                                                      */
/********************************************************/
BCKTYP *scanline(BCKTYP *bck,char *line)

{
#define OPEP    "unibck.c:scanline"

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
                            OPEP,line,bck->bckfile);
        phase=999;
        }
      break;
    case 1      :       //check scanning
      if (sscanf(line,"%[^=\n\r]=%[^\n\r]",command,info)!=2) {
        (void) rou_alert(0,"%s, unable to parse line <%s> in config file <%s> "
                           "(check config!)",
                           OPEP,line,bck->bckfile);
        phase=999;      //no need to go further
        }
      break;
    case 2      :       //what command du we have
      switch (findcmd(command)) {
        case cmd_included       :
          bck->included=(char **)rou_addlist((void **)bck->included,
                                             (void *)strdup(info));
          break;
        case cmd_excluded       :
          bck->excluded=(char **)rou_addlist((void **)bck->excluded,
                                             (void *)strdup(info));
          break;
        case cmd_level          :
          (void) free(bck->level);
          bck->level=strdup(info);
          break;
        default                 :
          (void) rou_alert(0,"Unable to scan line <%s> "
                             "within backup config file <%s> (format?)",
                             line,bck->bckfile);
          break;
        }
      break;
    default     :
      proceed=false;
      break;
    }
  phase++;
  }
return bck;
#undef  OPEP
}
/*
^L
*/
/********************************************************/
/*                                                      */
/*	Procedure to scan a file contens and extract    */
/*      all needed  information                         */
/*                                                      */
/********************************************************/
BCKTYP *scanbck(const char *filename)

{
#define OPEP    "unibck.c:scanbck"
BCKTYP *bck;
FILE *fichier;
int phase;
int proceed;

bck=(BCKTYP *)0;
fichier=(FILE *)0;
phase=0;
proceed=true;
while (proceed==true) {
  switch (phase) {
    case 0      :       //lets open the file
      if ((fichier=fopen(filename,"r"))==(FILE *)0) {
        (void) rou_alert(0,"%s, Unable to open file <%s> (error=<%s>)",
                            OPEP,filename,strerror(errno));
        phase=999;      //no need to go further
        }
      break;
    case 1      :       //reading lines
      char line[100];
   
      bck=(BCKTYP *)calloc(1,sizeof(BCKTYP)); 
      bck->bckfile=strdup(filename);
      bck->level=strdup("systems");
      while (rou_getstr(fichier,line,sizeof(line)-1,false,'#')!=(char *)0) {
        bck=scanline(bck,line);
        }
      break;
    case 2      :       //closing file
      (void) fclose(fichier);
      break;
    default     :
      proceed=false;
      break;
    }
  phase++;
  }
return bck;
#undef  OPEP
}
/*
^L
*/
/********************************************************/
/*                                                      */
/*	Procedure to free memory used by a backyp data  */
/*      structure.                                      */
/*      found                                           */
/*                                                      */
/********************************************************/
BCKTYP *bck_freebck(BCKTYP *bck)

{
if (bck!=(BCKTYP *)0) {
  if (bck->tapename!=(char *)0)
    (void) free(bck->tapename);
  if (bck->bckfile!=(char *)0)
    (void) free(bck->bckfile);
  if (bck->level!=(char *)0)
    (void) free(bck->level);
  bck->excluded=(char **)rou_freelist((void **)bck->excluded,
                                      (freehandler_t)free);
  bck->included=(char **)rou_freelist((void **)bck->included,
                                      (freehandler_t)free);
  (void) free(bck);
  bck=(BCKTYP *)0;
  }
return bck;
}
/*
^L
*/
/********************************************************/
/*                                                      */
/*	Procedure to collect the list of directory or   */
/*      file to be uncluded in backup.                  */
/*      return a null pointer if nothing to collect is  */
/*      found                                           */
/*                                                      */
/********************************************************/
BCKTYP *bck_getbcklist(const char *filename,char **levels)

{
BCKTYP *bck;

bck=(BCKTYP *)0;
if ((bck=scanbck(filename))!=(BCKTYP *)0) {
  int found;

  found=false;
  if (levels!=(char **)0) {
    char **ptr;

    ptr=levels;
    while ((*ptr)!=(char *)0) {
      if (strcmp(*ptr,bck->level)==0) {
        found=true;
        }
      ptr++;
      }
    }
  if (found==false)
    bck=bck_freebck(bck);
  }
return bck;
}
/*
^L
*/
/********************************************************/
/*                                                      */
/*	Procedure to write data after a (slot) tape     */
/*      marker within tape device.                      */
/*      return true if everything right, false otherwise*/
/*                                                      */
/********************************************************/
int bck_todevice(char *search,char *devname,int slot)

{
int status;

status=false;
return status;
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
int bck_openunibck()

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
int bck_closeunibck()

{
int status;

status=0;
if (modopen==true) {
  (void) rou_closesubrou();
  modopen=false;
  }
return status;
}
