// vim: smarttab tabstop=8 shiftwidth=2 expandtab
/********************************************************/
/*							*/
/*	Low level subroutine implementation	        */
/*	handle tape contents                            */
/*							*/
/********************************************************/
#include        <sys/mtio.h>
#include        <sys/stat.h>
#include        <stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include        <unistd.h>

#include	"subrou.h"
#include	"unidev.h"

//configuration file tapetype and related device
#define TAPEDEV CONFDIR"tapedev"

#define	MEOM	   0xFFFFFFFF	/*end of media marker	*/
#define	MEOF	   0xA0A0B0B0	/*end of file in media	*/
#define	BACKMAGIC   "BACKDMRK"	/*string minus '`'	*/
#define	SMAG		    8	//BACK MAGIC identifier lenght
#define	SIDAG		   30	//BACK MAGIC sequence size

/*The record structure must be keept the same		*/
typedef	struct	{	//USB-key Marker
	char mgk[SIDAG];//marker Magic
	ul64  mrkval;	//marker value type
			//0XA0A0B0B0 = end of file
			//0XFFFFFFFF = end of media
        u_int fileno;   //marker file number
        off_t slotpos;  //marker slot position on device
        off_t slotnxt;  //next  marker position
	off_t fstart;	//file start position 
	u_long fsize;	//file real size
	u_long chksum;	//marker checksum
	}MRKTYP __attribute__ ((aligned (8)));

typedef	enum	{	//backup device type
	dev_tape,	//device is a tape
	dev_usb,	//device is an USB key
	dev_file,	//device is an simple file
	dev_ukn		//unknown device type
	}DEVSPEC;

typedef	struct	{	//defining device handle
	int handle;	//device handle
        char *devname;  //device name
	DEVSPEC device;	//backup device type
	ul64 blksize;   //used block size
        ul64 fstart;    //file oposition of the current file
        ul64 fsize;     //sizeof of the current file
        off_t prvmrk;   //Previous marker position
        u_int mt_fileno;//current file number
        u_int mt_blkno; //current block number
	}HDLTYP;	

static  int modopen;            //boolean module open/close
/*

*/
/********************************************************/
/*							*/
/*	Subroutine to set the TAPE (st0) options	*/
/*							*/
/********************************************************/
static int setopts(int handle,int cmd,int param)

{
struct mtop mt_cmd;

mt_cmd.mt_op=cmd;
mt_cmd.mt_count=param;
return ioctl(handle,MTIOCTOP,&mt_cmd);
}
/*

*/
/********************************************************/
/*							*/
/*	Procedure to compute marker checksum            */
/*      device hanlde.                                  */
/*							*/
/********************************************************/
static ul64 mrk_chksum(MRKTYP *mrk,register ul64 blksize)

{
register ul64 chksum;

chksum=(ul64)0;
mrk->chksum=(ul64)0;
for (int i=0;i<blksize;i++)
  chksum+=(ul64)(((unsigned char *)mrk)[i]);
return chksum;
}
/*

*/
/********************************************************/
/*							*/
/*	Subroutine to prepare a file marker on file type*/
/*	device.						*/
/*							*/
/********************************************************/
static MRKTYP *genmarker(HDLTYP *hdl,ul64 slotpos,ul64 mrkval)

{
MRKTYP *mrk;

mrk=(MRKTYP *)calloc(hdl->blksize,sizeof(char));
mrk->slotpos=(ul64)slotpos;
mrk->mrkval=mrkval;
mrk->fileno=0;
if (mrkval==MEOF)
  mrk->fileno=hdl->mt_fileno;
(void) snprintf(mrk->mgk,SIDAG,"%s-%08llx-%05d",BACKMAGIC,mrkval,mrk->fileno);
mrk->chksum=mrk_chksum(mrk,hdl->blksize);
return mrk;
}
/*

*/
/********************************************************/
/*							*/
/*	Procedure to read a marker at a specific        */
/*      position with the tape file (USEB).             */
/*	return true if marker, false otherwize		*/
/*							*/
/********************************************************/
static MRKTYP *usb_readmarker(HDLTYP *hdl,off_t slotpos)

{
#define OPEP    "unidev.c:usb_readmarker"
MRKTYP *mrk;
off_t current;
char *block;
int phase;
int proceed;

mrk=(MRKTYP *)0;
current=(off_t)0;
block=(char *)calloc(hdl->blksize,sizeof(char));
phase=0;
proceed=true;
while (proceed==true) {
  switch (phase) {
    case 0      :       //positionning cursor on the right spot
      if ((current=lseek(hdl->handle,slotpos,SEEK_SET))!=slotpos) {
        (void) rou_alert(0,"%s, unable to read a marker block (error=<%s>)",
			    OPEP,strerror(errno));
        phase=999;      //trouble trouble
        }
      break;
    case 1      :
      if (read(hdl->handle,block,hdl->blksize)<hdl->blksize) {
        (void) rou_alert(0,"%s, unable to read a complete block (error=<%s>)",
			    OPEP,strerror(errno));
	phase=999;	//not able to read marker?
        }
      break;
    case 2	:	/*find a marker magic number	*/
      if (strncmp(block,BACKMAGIC,SMAG)!=0) {
        (void) rou_alert(0,"%s, marker missing at position '%lu' on tape "
                           "device (bug?)", OPEP,slotpos);
	phase=999;	//no marker found within device file
        }
      break;
    case 3	:	/*marker at the rihg position?	*/
      __u64 chksum;
      __u64 targetsum;

      targetsum=((MRKTYP *)block)->chksum;
      ((MRKTYP *)block)->chksum=0;
      chksum=mrk_chksum((MRKTYP *)block,hdl->blksize);
      if (chksum!=targetsum) {
        (void) rou_alert(0,"%s Header checksum is not found right at '%lu' "
                           "(target=%08x checksum='%08x')",
			   OPEP,slotpos,targetsum,chksum);
	phase=999;	//not a valid marker found within device file
	}
      break;
    case 4	:	//bingo this is a marker, reporting
      mrk=(MRKTYP *)calloc(hdl->blksize,sizeof(char));
      (void) memcpy(mrk,block,hdl->blksize);
      break;
    default	:	//SAFE Guard
      proceed=false;
      break;
    }
  phase++;
  }
(void) free(block);
return mrk;
#undef  OPEP
}
/*

*/
/********************************************************/
/*							*/
/*	Procedure to write a marker specific marker     */
/*      at a specific location within the tape (USB)    */
/*      device.                                         */
/*	return true if marker was inserted.             */
/*							*/
/********************************************************/
static int usb_writemarker(HDLTYP *hdl,off_t slotpos,MRKTYP *mrk)

{
#define OPEP    "unidev.c:usb_writemarker"

int status;
off_t current;
char *block;
int phase;
int proceed;

status=-1;
current=(off_t)0;
block=(char *)calloc(hdl->blksize,sizeof(char));
phase=0;
proceed=true;
while (proceed==true) {
  switch (phase) {
    case 0      :       //positionning cursor on the right spot
      if ((current=lseek(hdl->handle,slotpos,SEEK_SET))!=slotpos) {
        (void) rou_alert(0,"%s, unable to read a marker block (error=<%s>)",
			    OPEP,strerror(errno));
        phase=999;      //trouble trouble
        }
      break;
    case 1      :       //computing current check sum
      (void) memcpy(block,mrk,sizeof(MRKTYP));
      ((MRKTYP *)block)->chksum=mrk_chksum((MRKTYP *)block,hdl->blksize);
      break;
    case 2      :       //writing marker
      if (write(hdl->handle,block,hdl->blksize)<hdl->blksize) {
        (void) rou_alert(0,"%s, unable to write a complete block (error=<%s>)",
			    OPEP,strerror(errno));
	phase=999;	//not able to read marker?
        }
      break;
    case 3      :       //everything fine
      if (mrk->mrkval==MEOF) {
        hdl->fsize=0;
        hdl->fstart=slotpos+hdl->blksize;
        hdl->prvmrk=slotpos;
        }
      else      //MEOM marker, resync on end of previous MEOF
        (void) lseek(hdl->handle,slotpos,SEEK_SET);
      status=0;
      break;
    default	:	//SAFE Guard
      proceed=false;
      break;
    }
  phase++;
  }
(void) free(block);
return status;
#undef  OPEP
}
/*

*/
/********************************************************/
/*							*/
/*	Procedure to add a marker at the current file   */
/*      (USB) device current position.                  */
/*	return true if marker was inserted.             */
/*							*/
/********************************************************/
static int usb_addmarker(HDLTYP *hdl,ul64 mrkval)

{
#define OPEP    "unidev.c:usb_addmarker"

int status;
off_t slotpos;
MRKTYP *mrk;
int phase;
int proceed;

status=-1;
slotpos=(off_t)0;
mrk=(MRKTYP *)0;
phase=0;
proceed=true;
while (proceed==true) {
  switch (phase) {
    case 0      :       //what th current slot/block position
      if ((slotpos=lseek(hdl->handle,0,SEEK_CUR))<0) {
        (void) rou_alert(0,"%s, unable to read a marker block (error=<%s>)",
                            OPEP,strerror(errno));
        phase=999;      //trouble trouble
        }
      break;
    case 1      :       //make sure ea are ath te block begining
      if ((slotpos%hdl->blksize)!=0) {
        slotpos-=(slotpos%hdl->blksize);
        slotpos+=hdl->blksize;
        (void) lseek(hdl->handle,slotpos,SEEK_SET);
        (void) rou_alert(0,"%s, Alert! resync cursor position to '%d' "
                             "(trouble? error=<%s>)",OPEP,slotpos,strerror(errno));
        }
      break;
    case 2      :       //is this the first marker added or last marker
      if ((mrkval==MEOM)||(hdl->prvmrk==(off_t)0))
        phase++;        //no need to update previous marker
      break;
    case 3      :       //updating previous marker
      if ((mrk=usb_readmarker(hdl,hdl->prvmrk))==(MRKTYP *)0) {
        (void) rou_alert(0,"%s, Alert! Unable to read previous marker at '%lu' "
                           "cursor position (bug?)",OPEP,slotpos);
        phase=999;      //no need to go further.
        }
      else {
        mrk->fstart=hdl->fstart;
        mrk->fsize=hdl->fsize;
        mrk->slotnxt=slotpos;
        (void) usb_writemarker(hdl,hdl->prvmrk,mrk);
        (void) free(mrk);
        }
      break;
    case 4      :       //adding a marker
      hdl->fsize=0;
      mrk=genmarker(hdl,slotpos,mrkval);
      if (usb_writemarker(hdl,slotpos,mrk)<0) {
        (void) rou_alert(0,"%s, Alert! Unable to write new  marker at '%lu' "
                           "cursor position (bug?)",OPEP,slotpos);
        phase=999;      //no need to go further
        }
      (void) free(mrk);
      break;
    case 5      :       //adjusting hdl parameter;
      hdl->fstart=slotpos+hdl->blksize;
      break;
    default     :       //SAFE guard
      proceed=false;
      break;
    }
  phase++;
  }
return status;
#undef  OPEP
}
/*

*/
/********************************************************/
/*							*/
/*	Procedure to read a marker			*/
/*	return true if marker, false otherwize		*/
/*							*/
/********************************************************/
static MRKTYP *readonemarker(HDLTYP *hdl,off_t slotpos)

{
#define	OPEP	"unidev.c:readonemarker"

MRKTYP *mrk;
char *block;
int phase;
int proceed;

mrk=(MRKTYP *)0;
block=(char *)calloc(hdl->blksize,sizeof(char));
phase=0;
proceed=true;
while (proceed==true) {
  switch (phase) {
    case 0	:	//syncing a good currsor position
      if ((slotpos%hdl->blksize)!=0) {
        slotpos-=(slotpos%hdl->blksize);
        (void) rou_alert(0,"%s, Alert! resync cursor position to '%d' "
                           "(trouble? error=<%s>)",OPEP,slotpos,strerror(errno));
        }
      (void) lseek(hdl->handle,slotpos,SEEK_SET);
      break;
    case 1      :
      if (read(hdl->handle,block,hdl->blksize)<hdl->blksize) {
        (void) rou_alert(0,"%s, unable to read a complete block (error=<%s>)",
			    OPEP,strerror(errno));
	phase=999;	//no marker found within device file
	}
      break;
    case 2	:	/*find a marker magic number	*/
      if (strncmp(block,BACKMAGIC,SMAG)!=0) {
        (void) rou_alert(0,"%s, Marker not found at position='%lu' (bug?)",
			    OPEP,slotpos);
	phase=999;	//no marker found at expected position
        }
      break;
    case 3	:	/*marker at the rihg position?	*/
      __u64 chksum;
      __u64 targetsum;

      targetsum=((MRKTYP *)block)->chksum;
      ((MRKTYP *)block)->chksum=0;
      chksum=mrk_chksum((MRKTYP *)block,hdl->blksize);
      if (chksum!=targetsum) {
        (void) rou_alert(0,"%s Header checksum doesn't comply "
                           "target=%08x checksum='%08x",
			   OPEP,targetsum,chksum);
	phase=999;	//no valid marker found at expected position
	}
      break;
    case 4	:	//bingo this is a marker, reporting
      mrk=(MRKTYP *)calloc(hdl->blksize,sizeof(char));
      (void) memcpy(mrk,block,hdl->blksize);
      break;
    default	:	/*SAFE Guard			*/
      proceed=false;
      break;
    }
  phase++;
  }
(void) free(block);
return mrk;
#undef	OPEP
}
/*

*/
/********************************************************/
/*							*/
/*	Procedure to look at specfic marker TYPE	*/
/*							*/
/********************************************************/
static MRKTYP *findmarker(HDLTYP *hdl,off_t slotpos,int num)

{
#define	OPEP	"unidev.c:findmarker"
MRKTYP *mrk;

while ((mrk=readonemarker(hdl,slotpos))!=(MRKTYP *)0) {
  int mrknum;
  off_t next;

  mrknum=mrk->fileno;
  if (mrknum==num)
    break;
  next=mrk->slotnxt;
  (void) free(mrk);
  if (mrknum==0) {
    (void) rou_alert(0,"%s, Premarture MEOM marker at postion '%lu' (bug?)",
			    OPEP,next);
    break;
    }
  slotpos=next;
  }
return mrk;
#undef OPEP
}
/*

*/
/********************************************************/
/*							*/
/*	Procedure to free all memory used by a tape     */
/*      device hanlde.                                  */
/*							*/
/********************************************************/
static HDLTYP *freehandle(HDLTYP *handle)

{

if (handle!=(HDLTYP *)0) {
  if (handle->devname!=(char *)0)
    (void) free(handle->devname);
  (void) free(handle);
  handle=(HDLTYP *)0;
  }
return handle;
}
/*
^L
*/
/********************************************************/
/*							*/
/*      Procedure to scan a tapedev file discarding     */
/*      comments and parsing device description         */
/*      return a tape structure                         */
/*							*/
/********************************************************/
static DEVTYP **scandevs(FILE *fichier)

{
#define OPEP    "unidev.c:scandevs,"
DEVTYP **devs;
char line[3000];

devs=(DEVTYP **)0;
while (rou_getstr(fichier,line,sizeof(line)-1,false,'#')!=(char *)0) {
  DEVTYP *dev;
  int found;

  dev=(DEVTYP *)calloc(1,sizeof(DEVTYP));
  if ((found=sscanf(line,"%m[a-z,A-Z-,0-9]"     //device type
                         " %m[a-z,A-Z-,0-9/.]"  //device name
                         " %u"                  //device block size
                         " %[B,C]c",            //device mode (C/B)
                         &(dev->media),
                         &(dev->devname),
                         &(dev->blksize),
                         &(dev->mode)))!=4) {
     dev=dev_freedev(dev);
     }
  if (dev==(DEVTYP *)0) {
    (void) rou_alert(0,"%s Unable to scan line <%s> within "
                       "tapedev file, found %d items)",
                       OPEP,line,found);
    continue;
    }
  devs=(DEVTYP **)rou_addlist((void **)devs,(void *)dev);
  }
return devs;
#undef  OPEP
}
/*
^L
*/
/********************************************************/
/*                                                      */
/*	Procedure to free all memory used by the one    */
/*      of tape device entry                            */
/*                                                      */
/********************************************************/
DEVTYP *dev_freedev(DEVTYP *dev)

{
if (dev!=(DEVTYP *)0) {
  if (dev->devname!=(char *)0)
    (void) free(dev->devname);
  if (dev->media!=(char *)0)
    (void) free(dev->media);
  (void) free(dev);
  dev=(DEVTYP *)0;
  }
return dev;
}
/*
^L
*/
/********************************************************/
/*                                                      */
/*	Procedure to clone a dev entry in a newly       */
/*      allocated structure.                            */
/*                                                      */
/********************************************************/
DEVTYP *dev_dupdev(DEVTYP *dev)

{
DEVTYP *newdev;

newdev=(DEVTYP *)0;
if (dev!=(DEVTYP *)0) {
  newdev=(DEVTYP *)calloc(1,sizeof(DEVTYP));
  if (dev->media!=(char *)0)
    newdev->media=strdup(dev->media);
  if (dev->devname!=(char *)0)
    newdev->devname=strdup(dev->devname);
  newdev->blksize=dev->blksize;
  newdev->mode=dev->mode;
  }
return newdev;
}
/*
^L
*/
/********************************************************/
/*                                                      */
/*	Procedure to read the tapedev file and build    */
/*      a list of tape type related to hardware device. */
/*                                                      */
/********************************************************/
DEVTYP **dev_readdevfile(const char *filename)

{
#define OPEP    "unidev.c:dev_readdevfile,"

DEVTYP **devs;
char *fname;
FILE *fichier;
int phase;
int proceed;

devs=(DEVTYP **)0;
fname=(char *)0;
fichier=(FILE *)0;
phase=0;
proceed=true;
while (proceed==true) {
  switch (phase) {
    case 0      :               //building filename
      if (filename==(char *)0)
        filename=TAPEDEV;
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
      if ((devs=scandevs(fichier))==(DEVTYP **)0)
        (void) rou_alert(1,"%s Unable to scan device list <%s> (config format?)",
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
return devs;
#undef  OPEP
}
/*
^L
*/
/********************************************************/
/*                                                      */
/*	Procedure to get a list of device according a   */
/*      media name.                                     */
/*                                                      */
/********************************************************/
DEVTYP **dev_selectdev(const char *filename,const char *media)

{
#define OPEP    "unidev.c:dev_selectdev,"

DEVTYP **devs;
DEVTYP **selected;
int phase;
int proceed;

devs=(DEVTYP **)0;
selected=(DEVTYP **)0;
phase=0;
proceed=true;
while (proceed==true) {
  switch (phase) {
    case 0      :               //building filename
      if ((devs=dev_readdevfile(filename))==(DEVTYP **)0) 
        phase=999;
      break;
    case 1      :               //building filename
      DEVTYP **ptr;
    
      ptr=devs;
      while (*ptr!=(DEVTYP *)0) {
        if (strcmp((*ptr)->media,media)==0) {
          selected=(DEVTYP **)rou_addlist((void **)selected,
                                          (void *)dev_dupdev(*ptr));
          }
        ptr++;
        } 
      devs=(DEVTYP **)rou_freelist((void **)devs,(freehandler_t)dev_freedev);
      break;
    default     :               //exiting
      proceed=false;
      break;
    }
  phase++;
  }
return selected;
#undef  OPEP
}
/*
^L
*/
/********************************************************/
/*                                                      */
/*	Procedure to scan a devtape list and return     */
/*      the number of entry matching devtype+devname    */
/*      occurence.                                      */
/*                                                      */
/********************************************************/
const DEVTYP *dev_finddev(DEVTYP **devs,char *media,char *devname)

{
const DEVTYP *found;

found=(const DEVTYP *)0;
if (devs!=(DEVTYP **)0) {
  while (*devs!=(DEVTYP *)0) {
    register DEVTYP *ptr;

    ptr=*devs;
    devs++;
    if (strcmp(ptr->media,media)!=0)
      continue;
    if (strcmp(ptr->devname,devname)!=0)
      continue;
    found=ptr;
    break;
    }
  }
return found;
}
/*

*/
/********************************************************/
/*							*/
/*	Subroutine to open a tape device		*/
/*							*/
/********************************************************/
THANDLE *dev_open(char *devname,u_int blksize,int mode)

{
#define OPEP    "unidev.c:dev_open,"

HDLTYP *hdl;
struct stat buf;
int phase;
int proceed;

hdl=(HDLTYP *)calloc(1,sizeof(HDLTYP));
(void) memset(&buf,'\000',sizeof(buf));
hdl->devname=rou_apppath(devname);
hdl->blksize=blksize;
hdl->handle=-1;
hdl->prvmrk=(off_t)0;
hdl->device=dev_ukn;
phase=0;
proceed=true;
while (proceed==true) {
  switch (phase) {
    case 0	:	/*opening device		*/
      if ((hdl->handle=open(hdl->devname,mode))<0) {
	(void) rou_alert(1,"%s Unable to open device <%s> in mode '%0x', "
                           "(error=<%s>)",
			   OPEP,hdl->devname,mode,strerror(errno));
        phase=999;
	}
      break;
    case 1	:       /*detecting device type		*/
      if (stat(hdl->devname,&buf)<0) {
	(void) rou_alert(1,"%s Unable to get device <%s> stat, (error=<%s>)",
			    OPEP,hdl->devname,strerror(errno));
        (void) close(hdl->handle);
	phase=999;
	}
      break;
    case 2	:	/*setting device type		*/
      switch (buf.st_mode & S_IFMT) {
	case S_IFBLK	:
	  hdl->device=dev_usb;
	  break;
	case S_IFCHR	:
	  hdl->device=dev_tape;
	  break;
	case S_IFREG	:
	  hdl->device=dev_file;
	  break;
	default		:
	  (void) rou_alert(1,"%s Unexpected '%08o' file type (aborting)",
                             OPEP,buf.st_mode & S_IFMT);
	  phase=999;
	  break;
	}
      break;
    case 3	:	/*moving device if needed	*/
      switch (hdl->device) {
	case dev_tape   :
	  (void) setopts(hdl->handle,MTREW,0);  //rewinding
	  (void) setopts(hdl->handle,MTCOMPRESSION,0);
	  if (setopts(hdl->handle,MTSETBLK,blksize)<0) {
	    (void) rou_alert(1,"%s Unable to set device <%s> blocksize ('%d') "
			       "option (errno=<%s>)",
				OPEP,hdl->devname,blksize,strerror(errno));
            (void) close(hdl->handle);
	    hdl->device=dev_ukn;
	    phase=999;
	    }
	  break;
	case dev_file   :       //to be ready to ready the tape header
	case dev_usb    :
          hdl->fstart=0;
          hdl->fsize=blksize; 
	  break;
	default		:
	  break;
	}
      break;
    default	:	/*SAFE Guard			*/
      if (hdl->device==dev_ukn) 
	hdl=freehandle(hdl);
      proceed=false;
      break;
    }
  phase++;
  }
return (THANDLE *)hdl;
#undef  OPEP
}
/*

*/
/********************************************************/
/*							*/
/*	Subroutine to close a tape device		*/
/*							*/
/********************************************************/
THANDLE *dev_close(THANDLE *handle,int eject)

{
#define	OPEP	"unidev.c:dev_close"

HDLTYP *hdl;

hdl=(HDLTYP *)handle;
if (hdl!=(HDLTYP *)0) {
  switch (hdl->device) {
    case dev_tape	:
      if ((eject==true)&&(setopts(hdl->handle,MTOFFL,0)<0))
        (void) rou_alert(1,"%s, Unable to rewind and eject tape (error=<%s>)",
			    OPEP,strerror(errno));
      break;
    case dev_file	:	/*Nothing to do		*/
    case dev_usb	:
      break;
    case dev_ukn	:
      (void) rou_alert(1,"%s, Trying to eject unknown device type "
                         "(TAPE,USB, Bug?)",OPEP);
      break;
    }
  if (close(hdl->handle)<0)
    (void) rou_alert(0,"%s, Unable to close device, (error=<%s>)",
                       OPEP,strerror(errno));
  hdl=freehandle(hdl);
  }
return (THANDLE *)hdl;
#undef	OPEP
}
/*

*/
/********************************************************/
/*							*/
/*	Subroutine to read from the tape device		*/
/*							*/
/********************************************************/
long dev_read(THANDLE *handle,const char *buff,u_int count)

{
#define	OPEP	"unidev.c:dev_read"
long lu;
HDLTYP *hdl;
int phase;
int proceed;

lu=(long)0;
hdl=(HDLTYP *)handle;
phase=0;
proceed=true;
while (proceed==true) {
  switch (phase) {
    case 0	:	/*is hdl set?			*/
      if (hdl==(HDLTYP *)0) {
	(void) rou_crash("%s, hdl pointer is null (bug?)",OPEP);
	(void) exit(-1);/*never reached			*/
	}
      break;
    case 1	:	/*checking count limit		*/
      if (count>hdl->blksize) {
	(void) rou_crash("%s, Trying to read '%lu' byte while block size is'%lu'",
		          OPEP,count,hdl->blksize);
	(void) exit(-1);/*never reached			*/
	}
      break;
    case 2	:	//making sure not reading too many char
      switch (hdl->device) {
	case dev_tape	:	//nothing to do(limited by st0 device
	  break;
	case dev_file	:	//
	case dev_usb	:
          if (count>hdl->fsize)
            count=hdl->fsize;
          if (count==0)
            phase=999;          //nothing remaining to read anymore
	  break;
	case dev_ukn	:	/*very unlikely		*/
	default		:
	  (void) rou_crash("%s, Very unexpected device <%s> type (big bug!)",
			   OPEP,hdl->devname);
	  (void) exit(-1);/*never reached			*/
          break;
        }
      break;
    case 3	:	/*reading memory		*/
      if ((lu=read(hdl->handle,(char *)buff,count))<0) {
	(void) rou_alert(0,"%s, Unable to fully read tape "
                           "device <%s> (error=<%s>)",
		     	   OPEP,hdl->devname,strerror(errno));
	phase=999;	/*no need to go further		*/
	}
      break;
    case 4	:	/*adjusting counting		*/
      switch (hdl->device) {
	case dev_tape	:	/*nothing to do		*/
	  break;
	case dev_file	:	/*Nothing to do		*/
	case dev_usb	:
          hdl->fsize-=lu;
	  break;
	case dev_ukn	:	//more than very unlikely
	default		:
          break;
	}
      break;
    default	:	/*SAFE Guard			*/
      proceed=false;
      break;
    }
  phase++;
  }
return lu;
#undef	OPEP
}
/*

*/
/********************************************************/
/*							*/
/*	Subroutine to write to the tape device		*/
/*	return the number of char written or -1 on	*/
/*	error.						*/
/*							*/
/********************************************************/
long dev_write(THANDLE *handle,const char *buff,u_int count)

{
#define	OPEP "unidev.c:dev_write"

HDLTYP *hdl;
long stored;
int phase;
int proceed;

hdl=(HDLTYP *)handle;
stored=-1;
phase=0;
proceed=true;
while (proceed==true) {
  switch (phase) {
    case 0	:	/*is hdl set?			*/
      if (hdl==(HDLTYP *)0) {
	(void) rou_crash("%s hdl pointer is null (big bug?)",OPEP);
	(void) exit(-1);/*never reached			*/
	}
      break;
    case 1	:	/*checking count limit		*/
      if (count>hdl->blksize) {
	(void) rou_crash("%s Trying to write '%lu' byte to device <%s>, "
                         "while block size is'%lu'",
		          OPEP,count,hdl->devname,hdl->blksize);
	(void) exit(-1);/*never reached			*/
	}
      break;
    case 2	:	/*writing memory		*/
      {
      char *bufloc;

      bufloc=(char *)calloc(hdl->blksize,sizeof(char));
      (void) memmove(bufloc,buff,count);
      stored=write(hdl->handle,bufloc,hdl->blksize);
      (void) free(bufloc);
      }
      break;
    case 3	:	/*all buffer was stored?	*/
      if ((stored<0)||(stored<hdl->blksize)) {
        (void) rou_alert(0,"%s, Unable to fully write device (error=<%s>)",
                            OPEP, strerror(errno));
	stored=-1;
	phase=999;	/*no need to go further		*/
	}
      break;
    case 4	:	/*updating storage information	*/
      hdl->fsize+=count;
      stored=count;
      break;
    default	:	/*SAFE Guard			*/
      proceed=false;
      break;
    }
  phase++;
  }
return stored;
#undef	OPEP
}
/*

*/
/********************************************************/
/*							*/
/*	Subroutine to move tape cursor			*/
/*	return  0, if cursor was successfully moved	*/
/*	return -1, if not able to move cursor		*/
/*							*/
/********************************************************/
int dev_move(THANDLE *handle,MOVENU action,int disp)

{
#define	OPEP	"unidev.c:dev_move"

int status;
HDLTYP *hdl;

status=-1;
hdl=(HDLTYP *)handle;
if (hdl==(HDLTYP *)0) {
  (void) rou_crash("%s invalid handle (bug?!)",OPEP);
  (void) exit(-1);        //never reached
  }
switch (action) {
  case mov_begin:
    switch (hdl->device) {
      case dev_tape	:
        status=setopts(hdl->handle,MTREW,0);
        break;
      case dev_usb	:
      case dev_file	:
        status=lseek(hdl->handle,0,SEEK_SET);
        break;
      default           :
        (void) rou_crash("%s invalid device '%d' (bug?!)",OPEP,hdl->device);
        break;
      }
    break;
  case mov_end  :
    switch (hdl->device) {
      case dev_tape	:
        status=setopts(hdl->handle,MTEOM,0);
        break;
      case dev_usb	:
      case dev_file	:
        MRKTYP *mrk;

        if ((mrk=findmarker(hdl,hdl->blksize,0))!=(MRKTYP *)0) {
          status=lseek(hdl->handle,mrk->slotpos,SEEK_SET);
          (void) free(mrk);
          }
        break;
      default           :
        (void) rou_crash("%s invalid device '%d' (bug?!)",OPEP,hdl->device);
        break;
      }
    break;
  case mov_next  :
    (void) dev_move(handle,mov_begin,0);
    switch (hdl->device) {
      case dev_tape	:
        status=setopts(hdl->handle,MTFSF,disp);
        break;
      case dev_usb	:
      case dev_file	:
        MRKTYP *mrk;

        if ((mrk=findmarker(hdl,hdl->blksize,disp))!=(MRKTYP *)0) {
          hdl->fsize=mrk->fsize;
          hdl->fstart=mrk->fstart;
          status=0;
          (void) free(mrk);
          }
        break;
      default           :
        (void) rou_crash("%s invalid device '%d' (bug?!)",OPEP,hdl->device);
        break;
      }
    break;
  default       :
    (void) rou_alert(1,"%s, Unexpected action '%d' on device <%s> type (Bug?)",
                         OPEP,(int)action,hdl->devname);
    break;
  }
return status;
#undef OPEP
}
/*

*/
/********************************************************/
/*							*/
/*	procedure to send on stdout the segment within  */
/*      a tape.                                         */
/*							*/
/********************************************************/
int dev_getonesegment(FILE *file,THANDLE *handle,int segnum)

{
int status;
int phase;
int proceed;

status=-1;
phase=0;
proceed=true;
while (proceed==true) {
  switch (phase) {
    case 0      :       //seek tape start
      if (dev_move(handle,mov_next,segnum)<0)
        phase=999;      //trouble trouble
      break;
    case 1      :       //seek tape start
      register u_int blksize;
      register char *block;
      register int got;

      blksize=dev_getblksize(handle);
      block=(char *)calloc(1,blksize); 
      while ((got=dev_read(handle,block,blksize))>0) {
        (void) fwrite(block,1,got,file);
        (void) memset(block,'\000',blksize);
        }
      (void) free(block);
      status=0;
      break;
    default     :
      proceed=false;
      break;
    }
  phase++;
  }
return status;
}
/*

*/
/********************************************************/
/*							*/
/*	Subroutine to know on which block the tape is	*/
/*      return -1 if trouble                            */
/*							*/
/********************************************************/
off_t dev_getcurpos(THANDLE *handle)

{
#define	OPEP	"unidev.c:dev_getcurpos"
HDLTYP *hdl;
off_t tell;
struct mtpos current;

hdl=(HDLTYP *)handle;
tell=-1;
switch (hdl->device) {
  case dev_tape	:
    if (ioctl(hdl->handle,MTIOCPOS,&current)<0) 
      (void) rou_alert(1,"%s, Unable to get tape current position, "
                         "on device <%s> (error=<%s>)",
			  OPEP,hdl->devname,strerror(errno));
    else
      tell=current.mt_blkno;
    break;
  case dev_usb	:
  case dev_file	:
    if ((tell=lseek(hdl->handle,0,SEEK_CUR))<0) {
      (void) rou_alert(0,"%s, Unable to get USB current position, "
                         "on device <%s> (error=<%s>)",
			  OPEP,hdl->devname,strerror(errno));
      }
    break;
  default	:
    (void) rou_alert(1,"%s, unexpected device type='%d' "
                       "for <%s>, (Bug?)",OPEP,hdl->device,hdl->devname);
    break;
  }
return tell;
#undef	OPEP
}
/*

*/
/********************************************************/
/*							*/
/*	Subroutine to set a number of EOF at the current*/
/*	tape position.					*/
/*							*/
/********************************************************/
int dev_setmark(THANDLE *handle)

{
#define	OPEP	"unidev.c:devsetmark"
int status;
HDLTYP *hdl;

status=0;
hdl=(HDLTYP *)handle;
switch (hdl->device) {
  case dev_tape	:
    if ((status=setopts(hdl->handle,MTWEOF,1))<0)
      (void) rou_alert(1,"%s, Unable to set EOF marker on tape (error=<%s>",
			 OPEP,strerror(errno));
    break;
  case dev_file	:
  case dev_usb	:
    hdl->mt_fileno++;
    (void) usb_addmarker(hdl,(__u64)MEOF);
    (void) usb_addmarker(hdl,(__u64)MEOM);
    break;
  default	:
    status=-1;
    (void) rou_alert(1,"%s unspected device type='%d', (Bug?)",OPEP,hdl->device);
    break;
  }
return status;
#undef	OPEP
}
/*

*/
/********************************************************/
/*							*/
/*	Subroutine to return the tape block size	*/
/*							*/
/********************************************************/
u_int dev_getblksize(THANDLE *handle)

{
u_int taille;

taille=(u_int)0;
if (handle!=(THANDLE *)0) {
  taille=((HDLTYP *)handle)->blksize;
  }
return taille;
}
/*

*/
/********************************************************/
/*							*/
/*	procedure to retrieve the handle device name    */
/*							*/
/********************************************************/
const char *dev_getdevname(THANDLE *handle)

{
const char *devname;

devname="bug_unknown_devname";
if (handle!=(THANDLE *)0) 
  devname=((DEVTYP *)handle)->devname;
return devname;
}
/*

*/
/********************************************************/
/*							*/
/*	Subroutine to return the current file number on	*/
/*	the tape.					*/
/*							*/
/********************************************************/
int dev_getfilenum(THANDLE *handle)

{
#define	OPEP	"unidev.c:dev_getfilenum,"

HDLTYP *hdl;
long number;
struct mtget gets;

hdl=(HDLTYP *)handle;
number=-1;
switch (hdl->device) {
  case dev_tape	:
    if ((ioctl(hdl->handle,MTIOCGET,&gets))<0) 
      (void) rou_alert(1,"%s Unable to get tape current filenum, (error=<%s>)",
			  OPEP,strerror(errno));
    else
      number=gets.mt_fileno;
    break;
  case dev_usb	:
  case dev_file	:
    number=hdl->mt_fileno;
    break;
  default	:
    (void) rou_alert(1,"%s unexpected device type='%d', (Bug?)",OPEP,hdl->device);
    break;
  }
return (int)number;
#undef OPEP
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
int dev_openunidev()

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
int dev_closeunidev()

{
int status;

status=0;
if (modopen==true) {
  (void) rou_closesubrou();
  modopen=false;
  }
return status;
}
