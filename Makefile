#--------------------------------------------------------------------
#Makefile to build the package
#--------------------------------------------------------------------
#default make
default	:  clean prod
#--------------------------------------------------------------------
#Executable generation area
#--------------------------------------------------------------------
prod								\
debug								\
	:
	   @ for i in $(SUBDIR) ;				\
	       do						\
	       $(MAKE) -s -C $$i $@ ;				\
	       done

clean	:
	   @ for i in $(SUBDIR) ;				\
	       do						\
	       $(MAKE) -s -C $$i $@ ;				\
	       done
	   @ - rm -fr $(APPNAME)-*

#--------------------------------------------------------------------
#test procedure
#--------------------------------------------------------------------
test	:  prod
	   ./shell/dotest.sh test-area
		
#--------------------------------------------------------------------
#Installation procedure
#--------------------------------------------------------------------
install	:
	   @ # preparing /var/lib/backd directory
	   @ install -d $(DESTDIR)/usr/share/man/man8
	   @ cp -a bin $(DESTDIR)/usr
	   @ install -d $(DESTDIR)/etc/$(APPNAME)/backd.d
	   @ install -m644 conf/system.bck			\
	   		$(DESTDIR)/etc/$(APPNAME)/backd.d/
	   @ install -m644 conf/standard.sch			\
			$(DESTDIR)/etc/$(APPNAME)
	   @ install -m644 conf/tapedev.sample			\
			$(DESTDIR)/etc/$(APPNAME)/tapedev
	   @ install -m644 conf/tapelist.sample			\
			$(DESTDIR)/etc/$(APPNAME)/tapelist
	   @ install -d $(DESTDIR)/usr/lib/$(APPNAME)/shell
	   @ install shell/sendmsg.sh $(DESTDIR)/usr/lib/$(APPNAME)/shell
	   @ install -d $(DESTDIR)/etc/rc.d/init.d/
	   @ install -m754					\
			contrib/systemv/backd.sh		\
			$(DESTDIR)/etc/rc.d/init.d/backd
	   @ install -d $(DESTDIR)/etc/sysconfig
	   @ install -m644 conf/backd.conf $(DESTDIR)/etc/sysconfig/backd
	   @ install -m444 man/backd.8 $(DESTDIR)/usr/share/man/man8/
	   @ install -d $(DESTDIR)/var/spool/$(APPNAME)
	   @ install -d $(DESTDIR)/var/run/$(APPNAME)
#--------------------------------------------------------------------
#Equivalences
#--------------------------------------------------------------------
SUBDIR	= 								\
	   lib								\
	   app								\
	   conf								\
	   man								\

#--------------------------------------------------------------------
#definitions
APPNAME	=  backd3
#--------------------------------------------------------------------
