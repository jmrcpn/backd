#This is an example if backup file, the name of that file
#is the backup name.
#You can have more than one backup definition with differente
#backup frequency and included file.
#Every backup definition must be stored in the backlist 
#directory (usualy /var/lib/backd/conf)
#---------------------------------------------------------
#Frequency is at which that peticulare back-up need to be
#done, we suggest to put backup at the same time in the day, 
#such you can put as many backup as requested to fill up
#the tape
#format is (a la crontab) hours, day of the month, month, day of the week
#in this example that backup will done at 3 AM, every sunday for
#January, Marsh, May
#
frequency=3 * * *
#
#---------------------------------------------------------
#media specify on which media type you want to put that
#backup (you need to have define at least on tape with that media in tapelist).
#
media=USB-64
#
#---------------------------------------------------------
#kept, specify (in days) how long you want to keep that backup
#
keep=5
#
#---------------------------------------------------------
#now define what are the file level you want to include
#in the backup
#level name are search within the bcklist file
#and are NOT keywork
levels=critical,systems
#---------------------------------------------------------
#
