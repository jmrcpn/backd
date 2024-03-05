#! /bin/sh
#--------------------------------------------------------------------------
#Script to test tape marking, writing and reading
# $1= DESTDIR	(mandatory)
#--------------------------------------------------------------------------
APPNAME=backd3
#==========================================================================
#To set few pseudo tape devices
#$1 is the  root directory
#--------------------------------------------------------------------------
setdevice ()

{
mkdir $1/dev
#preparing a 5 Megs tape device
dd if=/dev/zero of=$1/dev/tape0 bs=32K count=160 2> /dev/null
for t in 1 2 3 4
  do
  cp $1/dev/tape0 $1/dev/tape$t
  (
  echo -e -n "USB-64\t\t/dev/tape$t\t32768\t\tB\t"
  echo        "#device is a test device" 
  ) >> $1/etc/$APPNAME/tapedev
  done
}

#==========================================================================
#preparing data to backup pattern
#$1 is the  root directory
#--------------------------------------------------------------------------
setdata()

{
#setting data to be stored on tape
cp -a tobackup	$DESTDIR
#creating backup directive files
cat > $DESTDIR/etc/$APPNAME/backd.d/data.bck << EOF
level=critical
included=/tobackup/data
excluded=*~
excluded=*.previous
EOF

cat > $DESTDIR/etc/$APPNAME/backd.d/system1.bck << EOF
level=systems
included=/tobackup/sys1/
included=/tobackup/sys3/
excluded=*~
excluded=*.swp
EOF

cat > $DESTDIR/etc/$APPNAME/backd.d/system2.bck << EOF
level=systems
included=/tobackup/sys0/
included=/tobackup/sys2/
excluded=*~
excluded=*.nogood
EOF
}

#==========================================================================
#Marking all the tape devices
#$1 is the  root directory
#--------------------------------------------------------------------------
marks()

{
backd-marker			\
	-r $DESTDIR		\
	-d 0			\
	-m USB-64		\
	-s 5			\
	/dev/tape0,USB-0000	\
	/dev/tape1,USB-271828	\
	/dev/tape2,USB-314159 	\
	/dev/tape3,USB-355113	\
	/dev/tape4,USB-662607
}
#==========================================================================
#doing one backup on first available tape
#$1 is the  root directory
#--------------------------------------------------------------------------
dobackup ()

{
#doing one backup
$APPNAME			\
	-r $1			\
	-d 0			\
	-n			\
	-n			\
	
}
#--------------------------------------------------------------------------
#Main procedure
PATH=./bin:$PATH
if [ $# -ne 1 ] ; then
  echo "The installation directory is missing"
  exit -1
  fi

#creating needed diretory
DESTDIR=$1

#preparing distination directory
rm -fr $DESTDIR
make DESTDIR=$DESTDIR install

#creating data
setdata $DESTDIR

#creating tape device
setdevice $DESTDIR

#Marking the tape devices
marks $DESTDIR

#doing a first backup
dobackup $DESTDIR

#display tape num 4 segment
backd-reader			\
	-r $DESTDIR		\
	-d 0			\
	USB-271828		\
	4			\
		| gunzip -cf | cpio -ivt --quiet > /tmp/backd-$$

DIFF=$(diff backup-ref /tmp/backd-$$)
if [ "$DIFF" != "" ] ; then
  echo "Tape backup extraction not successfull, see file /tmp/backd-$$"
  echo "$DIFF"
  else
  echo "Tape backup extraction OK"
  rm /tmp/backd-$$
  fi

