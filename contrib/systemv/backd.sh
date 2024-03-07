#!/bin/sh
#
# backd     This shell script takes care of starting and stopping
#             backd daemon.
#
# chkconfig: 2345 95 05
#
# description: backd is a daemon to do backup process automaticaly
# 	       (or almost)
# Source function library.
#
### BEGIN INIT INFO
# Provides: backd
# Required-Start: 
# Required-Stop:
# Default-Start: 3 4 5
# Default-Stop: 0 1 2 6
# Short-Description: starting container
### END INIT INFO


OPTIONS=""
. /lib/lsb/init-functions

PROG=backd3
PID_FILE=/var/run/$PROG/$PROG.lock
CMD=/usr/bin/$PROG

if [ -f /etc/sysconfig/backd ]; then
 . /etc/sysconfig/backd
fi

# See how we were called.
case "$1" in

  start)
	# Start daemons.
	log_info_msg "Starting backd daemon..."
	$CMD $OPTIONS > /dev/null
	evaluate_retval
	;;

  stop)
	# Stop daemons.
	if [ -f $PID_FILE ] ; then
	  log_info_msg "Stopping backd daemon..."
	  killproc -p $PID_FILE $CMD -TERM
	  evaluate_retval
	  rm -f $PID_FILE
	  fi
	;;

  atonce)
	log_info_msg "Sending ALRM signal to do backup atonce..."
	if [ -f $PID_FILE ] ; then
	  pid=`cat $PID_FILE 2>/dev/null`
          if [ -n "${pid}"  -a -d /proc/$pid ]; then
	    kill -ALRM ${pid}
	    fi
	  fi
	evaluate_retval
	;;

  restart)
	$0 stop
	sleep 1 
	$0 start
	;;

  reload)
  	log_info_msg "Reloading backd..."
	if [ -f $PID_FILE ] ; then
	  pid=`cat $PID_FILE 2>/dev/null`
          if [ -n "${pid}"  -a -d /proc/$pid ]; then
	    kill -HUP ${pid}
	    fi
          fi
        evaluate_retval
	;;

  status)
	statusproc $CMD
	;;

  condrestart)
	if [ -f $PID_FILE ] ; then
	  pid=`cat $PID_FILE 2>/dev/null`
          if [ -n "${pid}"  -a -d /proc/$pid ]; then
            $0 restart
	    fi
          fi
      ;;

  *)
	echo "Usage: backd {start|stop|restart|atonce|reload|status|condrestart}"
	exit 1
  esac

exit 0

