// vim: smarttab tabstop=8 shiftwidth=2 expandtab
/********************************************************/
/*							*/
/*	Low level subroutine declaration	        */
/*	handle backup device                            */
/*							*/
/********************************************************/
#ifndef	UNIDEV
#define UNIDEV

#include        <fcntl.h>


typedef long    THANDLE;        //internal refrence to backup device

typedef enum    {
        mov_begin,              //move tape at begining
        mov_end,                //move tape at end of media
        mov_next,               //move tape at start of next file
        mov_unknown             //unkown tape move
        }MOVENU;

//device definition structure
typedef struct  {
        char    *media;         //device type
        char    *devname;       //system device name
        u_int   blksize;        //device block size minimal access
        char    mode;           //'C'|'B' character|block device
        }DEVTYP;

//freeing memory used by a device list
extern DEVTYP *dev_freedev(DEVTYP *dev);

//to duplicate a dev structure
extern DEVTYP *dev_dupdev(DEVTYP *dev);

//reading tapedev file
extern DEVTYP **dev_readdevfile(const char *filename);

//reading tapedev file and select the one meaningfulle
extern DEVTYP **dev_selectdev(const char *filename,const char *media);

//read tapedev structure and return the number of devtype+devname entries
extern const DEVTYP *dev_finddev(DEVTYP **devs,char *media,char *devname);

//Opening the backup device
extern THANDLE *dev_open(char *devname,u_int blksize,int mode);

//closing the backup device
extern THANDLE *dev_close(THANDLE *handle,int eject);

//read buffer from backup device
extern long dev_read(THANDLE *handle,const char *buff,u_int count);

//write buffer to backup device
extern long dev_write(THANDLE *handle,const char *buff,u_int count);

//procedure to know the current "tape" position on the device
extern off_t dev_getcurpos(THANDLE *handle);

//procedure to move reading/writing tape position
extern int dev_move(THANDLE *handle,MOVENU action,int disp);

//procedure to send on FILE the contents of one tape segment
extern int dev_getonesegment(FILE *file,THANDLE *handle,int segnum);

//procedure to write a specific marker on
//a pseudo tape (file, usb)
extern int dev_setmark(THANDLE *handle);

//procedure to get/know the device block size
extern u_int dev_getblksize(THANDLE *handle);

//procedure to get/know the device name
extern const char *dev_getdevname(THANDLE *handle);

//return the current file number with the device
extern int dev_getfilenum(THANDLE *handle);

//homework done by module before starting to use it
extern int dev_openunidev();

//homework dto be done by module before exiting
extern int dev_closeunidev();

#endif
