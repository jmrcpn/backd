// vim: smarttab tabstop=8 shiftwidth=2 expandtab
/********************************************************/
/*							*/
/*	Low level subroutine declaration	        */
/*	to hanle an argv list and extract	        */
/*	parameters.				        */
/*							*/
/********************************************************/
#include	<stdlib.h>
#include	<string.h>
#include	<unistd.h>

#include	"subrou.h"
#include	"unipar.h"

typedef struct {
	const char *id;		//application name
	WHATTYPE what;		//what
	}LSTTYP;

const LSTTYP app_name[]={
	  {APPNAME,wha_backd},
	  {"backd-marker",wha_marker},
	  {"backd-reader",wha_reader},
	  {"backd-scanner",wha_scanner},
	  {(const char *)0,wha_unknown}
	  };

static  int modopen;            //boolean module open/close
/*
^L
*/
/********************************************************/
/*							*/
/*						        */
/*	Procedure to init the argument list	        */
/*							*/
/********************************************************/
static ARGTYP *initparams()

{
ARGTYP *params;

params=(ARGTYP *)calloc(1,sizeof(ARGTYP));
params->argv=(char **)0;
params->device=(char *)0;
params->media=(char *)0;
params->delay=act_standard;
return params;
}
/*

*/
/********************************************************/
/*							*/
/*	Display a list of parameters available according*/
/*	the name used to call the backd program		*/
/*							*/
/********************************************************/
static int par_usage(char *name)

{
int status;
WHATTYPE called;

status=0;
(void) fprintf(stderr,"%s Version <%s>\n",name,rou_getversion());
(void) fprintf(stderr,"usage:\n  ");
called=par_whatami(name);
switch (called) {
  case wha_backd	:
    (void) fprintf(stderr,"%s\t"
		          "[-d debug] "
		          "[-f] "
		          "[-n] "
		          "[-h] "
                          "\n",app_name[(int)called].id);
    break;
  case wha_marker	:
    (void) fprintf(stderr,"%s\t"
		          "[-d debug] "
		          "[-h] "
		          "[-p poolname] "
		          "[-r root] "
		          "[-s mediasize] "
                          "device,tapename"
                          "\n",app_name[(int)called].id);
    break;
  case wha_reader	:
    (void) fprintf(stderr,"%s\t"
		          "[-d debug] "
		          "[-h] "
		          "[-r root] "
                          "tapename [seg]"
                          "\n",app_name[(int)called].id);
    break;
  case wha_scanner	:
    (void) fprintf(stderr,"%s\t"
		          "[-d debug] "
		          "[-h] "
		          "[-r root] "
		          "[-x tapename] "
                          "[media ...] "
                          "\n",app_name[(int)called].id);
    break;
  default		:
    status=-1;
    (void) rou_alert(0,"'%d' unexpected called name (bug?)",(int)called);
    break;
  }
//common parameters list
if (status==0) {
  (void) fprintf(stderr,"\twhere:\n");
  switch (called) {
    case wha_marker	:
      (void) fprintf(stderr,"\t\t-d level\t: debug level [1-10]\n");
      (void) fprintf(stderr,"\t\t-h\t\t: print this help message\n");
      (void) fprintf(stderr,"\t\t-m media\t: tape media type\n");
      (void) fprintf(stderr,"\t\t-r root\t\t: root working directory\n");
      (void) fprintf(stderr,"\t\t-s size\t\t: tape media size in megabytes\n");
      (void) fprintf(stderr,"\t\tdevice,label\t: 'Label' to write on "
                            "the tape available in 'device'\n");
      break;
    case wha_backd	:
      (void) fprintf(stderr,"\t\t-d level\t: debug level [1-10]\n");
      (void) fprintf(stderr,"\t\t-f\t\t: daemon started in foreground mode\n");
      (void) fprintf(stderr,"\t\t-h\t\t: print this help message\n");
      (void) fprintf(stderr,"\t\t-n\t\t: start backup quickly (10 min delay)\n");
      (void) fprintf(stderr,"\t\t-n -n\t\t: start backup at once "
                            "(foregound mode + no delay)\n");
      (void) fprintf(stderr,"\t\t-r root\t\t: root working directory\n");
      break;
    case wha_scanner	:
      (void) fprintf(stderr,"\t\t-d level\t: debug level [1-10]\n");
      (void) fprintf(stderr,"\t\t-h\t\t: print this help message\n");
      (void) fprintf(stderr,"\t\t-r root\t\t: root working directory\n");
      (void) fprintf(stderr,"\t\t-x tapename\t: tape extended report\n");
      (void) fprintf(stderr,"\t\t[media ...]\t: list of media to scan\n");
      break;
    case wha_reader	:
      (void) fprintf(stderr,"\t\t-d level\t: debug level [1-10]\n");
      (void) fprintf(stderr,"\t\t-h\t\t: print this help message\n");
      (void) fprintf(stderr,"\t\t-r root\t\t: root working directory\n");
      (void) fprintf(stderr,"\t\ttapename\t: tape label to extract data from\n");
      (void) fprintf(stderr,"\t\t[seg]\t\t: Tape segment to extract data from\n");
      break;
    default		:
      break;
    }
  }
return status;
}
/*
^L
*/
/********************************************************/
/*							*/
/*	Procedure to free memory used by an arg         */
/*	list					        */
/*							*/
/********************************************************/
ARGTYP *par_freeparams(ARGTYP *params)

{
if (params!=(ARGTYP *)0) {
  if (params->argv!=(char **)0) {
    char **ptr;

    ptr=params->argv;
    while (*ptr!=(char *)0) {
      (void) free(*ptr);
      ptr++;
      }
    (void) free(params->argv);
    }  
  if (params->media!=(char *)0)
    (void) free(params->media);
  if (params->device!=(char *)0)
    (void) free(params->device);
  (void) free(params);
  params=(ARGTYP *)0;
  }
return params;
}
/*
^L
*/
/********************************************************/
/*							*/
/*	Procedure to extract argument list	        */
/*	but cherry-pick only the one allowed by         */
/*	sequence optstring.			        */
/*	on error return an null argtype		        */
/*							*/
/********************************************************/
ARGTYP *par_getparams(int argc,char * const argv[],const char *optstring)

{
ARGTYP *params;
int c;

params=initparams();
opterr=0;               //no error message from getopt library routine
while (((c=getopt(argc,argv,optstring))!=EOF)&&(params!=(ARGTYP *)0)) {
  switch(c) {
    case 'd'	:       //debug level
      debug=atoi(optarg);
      (void) rou_alert(1,"debug level is now '%d'",debug);
      break;
    case 'f'	:       //foreground mode
      foreground=true;
      break;
    case 'h'	:       //requestion program help
      (void) par_usage(argv[0]);
      params=par_freeparams(params);
      break;
    case 'n'	:               //start backup quickly (now()+10 minutes)
      switch (params->delay) {
        case act_standard       :
          params->delay=act_quick;
          break;
        case act_quick          :
          params->delay=act_atonce;
          foreground=true;      //atonce is alway in foreground
          break;
        default                 :
          break;
        }
      break;
    case 'm'	:       //media type
      if (params->media!=(char *)0)
        (void) free(params->media);
      params->media=strdup(optarg);
      break;
    case 'r'	:
      if (rootdir!=(char *)0)
        (void) free(rootdir);
      rootdir=strdup(optarg);
      break;
    case 's'	:
      params->mediasize=atoll(optarg);
      break;
    case 'v'	:
      (void) fprintf(stderr,"%s Version <%s>\n",argv[0],rou_getversion());
      (void) exit(0);           //just display version
      break;
    case 'x'	:
      params->extended=true;
      break;
    default	:
      (void) par_usage(argv[0]);
      (void) rou_alert(0,"\"%s\" unexpected argument designator",argv[optind-1]);
      params=par_freeparams(params);
      break;
    }
  }
if ((params!=(ARGTYP *)0)&&(argc>optind)) {
  int i;

  params->argc=argc-optind;
  params->argv=(char **)calloc(params->argc+1,sizeof(char *));
  for (i=0;i<params->argc;i++) {
    params->argv[i]=strdup(argv[optind+i]);
    }
  }
return params;
}
/*

*/
/********************************************************/
/*							*/
/*	lets find out what is the backd impersonation 	*/
/*	according backd called name, function is	*/
/*	sligthly different.				*/
/*							*/
/********************************************************/
WHATTYPE par_whatami(char *myname)

{
WHATTYPE result;
const LSTTYP *ptr;
char *name;

result=wha_unknown;
ptr=app_name;
if ((name=strrchr(myname,'/'))!=(char *)0)
  name++;
else
  name=myname;
while (ptr->id!=(char *)0) {
  if (strcmp(name,ptr->id)==0) {
    result=ptr->what;
    break;
    }
  ptr++;
  }
return result;
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
int par_openunipar()

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
int par_closeunipar()

{
int status;

status=0;
if (modopen==true) {
  (void) rou_closesubrou();
  modopen=false;
  }
return status;
}
