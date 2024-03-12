#! /bin/sh
#--------------------------------------------------------------------------
# $1= BASE=the root directory (for test purpose only)
# $2= the message code
# $2,$3.... value according the message code
#
#	Message code:
#			M_OK		;scheduled backup done successfully
#			M_TAPEOK	;found the tape in device
#			M_NOTAPE	;Tape not found
#--------------------------------------------------------------------------
if [ $# -lt 2 ] ; then
  echo "sendmsg.sh, Fatal error missing argument within <"$@">"
  exit -1
  fi
#setting the argument
BASE=$1
MSGCOD=$2

#setting local variables
#email originator
ORIGMNG=backd@`dnsdomainname`

#defining the spool directory
SPOOLDIR=$BASE/var/spool/backd3/
#--------------------------------------------------------------------------
cd `dirname $0`
#check about local configuration
if [ -f $BASE/etc/default/backd ]; then
 . $BASE/etc/default/backd
fi

#defining tape footer
FOOTER=$SPOOLDIR/$3/$3-footer
#--------------------------------------------------------------------------
#to check if BCKMNG is already defined
if [ ! $BCKMNG ] ; then
  export BCKMNG=root
fi
ATTACH=""
FROM="Your own Backup daemon <backd>";
TO="$BCKMNG"
#--------------------------------------------------------------------------
#--------------------------------------------------------------------------
#sending the actual message.
case "$2" in
  "M_OK"	)	# $3 tape reference, $4 backup time
    SUBJECT="Backup successful on tape '$3'"
    MSG="\nYou can now remove Tape '$3' from device '$4'.\n"
    MSG=$MSG"\n"
    MSG=$MSG"Please Find the summary of the tape '$3' content as attachment.\n"
    MSG=$MSG"\n"
    ATTACH="-a $FOOTER"
    if [ -f $FOOTER ] ; then
      cat $FOOTER		 	|			\
		/usr/bin/a2ps					\
			-1					\
			--medium=Letterdj 			\
			-P$PRINTER 				\
			--margin=30  				\
			--center-title=$3			\
			--left-footer				\
			--right-footer				\
			--pro=color				\
			--font-size=9				\
			--landscape				\
			--stdin=' '				\
			-b					\
			-q					\
			> /dev/null
      fi
    ;;
  "M_TAPEOK"	)	# $3 tape reference, $4 backup time $5 device
    SUBJECT="Ready: Tape '$3' found in tape reader"
    MSG="Tape '$3' is now available in device '$5'\n"
    MSG=$MSG"\n"
    MSG=$MSG"Please keep this tape in device until the backup time.\n"
    MSG=$MSG"Backup is scheduled to start at $4\n"
    MSG=$MSG"\n"
    ;;
  "M_NOTAPE"	)	# $3 tape reference, $4 backup time
    SUBJECT="Caution: Tape '$3' not yet in tape reader"
    MSG="Tape '$3' not yet available for backup.\n"
    MSG=$MSG"Please insert designated tape in tape reader.\n"
    MSG=$MSG"\n"
    MSG=$MSG"Backup starting is scheduled at $4\n"
    MSG=$MSG"\n"
    ;;
  "M_NOAVAIL"	)	# $3 backup time
    SUBJECT="Warning: Found no tape available to proceed with backup"
    MSG="Backup scheduled at '$3', But found no tape available\n"
    MSG=$MSG"to proceed.\n"
    MSG=$MSG"\n"
    MSG=$MSG"Please mark new tapes.\n"
    MSG=$MSG"\n"
    ;;

  *		)
    SUBJECT="Unknow error about tape (Bug?)"
    MSG="There is something really bad going on with backup process.\n"
    MSG=$MSG"\n"
    MSG=$MSG"Please check daemon log about it.\n"
    MSG=$MSG"\n"
    MSG=$MSG"Parameter received by sendmsg.sh script:\n"
    MSG=$MSG"\t<$*>"
    MSG=$MSG"\n\n"
    ;;
  esac
(
echo 
echo -e $MSG
echo 
) | s-nail $ATTACH -r $ORIGMNG -s"$SUBJECT" $BCKMNG ;

SYSLOG="Sent via Email, Subject: "$SUBJECT
echo $SYSLOG |  logger --id -p daemon.info 

#--------------------------------------------------------------------------
