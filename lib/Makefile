#--------------------------------------------------------------------
#Executable generation area
#--------------------------------------------------------------------
debug	:  
	   @ $(MAKE) OPTIME="-g" objs
	   @ echo "library compiled in '$@' mode, now ready"

prod	:  
	   @ $(MAKE) OPTIME="-O3" objs
	   @ echo "library compiled in '$@' mode, now ready"

clean	:
	   - rm -fr *.o *.a

#--------------------------------------------------------------------
#Equivalences
#--------------------------------------------------------------------
OBJS=						\
	modbck.o modget.o modsca.o modtap.o	\
	gesbck.o gestap.o gesfil.o gesmsg.o	\
	unibck.o unidev.o unipar.o uniprc.o	\
	unisch.o unisig.o unitap.o		\
	subrou.o 

objs	: 
	  @ $(MAKE) $(LIBBCK)

libbck.a					\
	: toremake $(OBJS)
	  @ ar -cr $(LIBBCK) $(OBJS)

#--------------------------------------------------------------------
#Dependances
#--------------------------------------------------------------------

modbck.o:  modbck.c modbck.h			\
	   gesbck.h gesfil.h gestap.h gesmsg.h	\
	   unibck.h unidev.h uniprc.h		\
	   unipar.h				\
	   subrou.h 

modget.o:  modget.c modget.h			\
	   gestap.h				\
	   unidev.h unipar.h uniprc.h		\
	   subrou.h

modsca.o:  modsca.c modsca.h			\
	   gestap.h				\
	   unidev.h unipar.h			\
	   subrou.h

modtap.o:  modtap.c modtap.h			\
	   gestap.h				\
	   subrou.h

gestap.o:  gestap.c gestap.h			\
	   uniprc.h				\

gesbck.o:  gesbck.c gesbck.h			\
	   unidev.h unitap.h			\
	   subrou.h

gesfil.o:  gesfil.c gesfil.h			\
	   subrou.h

gesmsg.o:  gesmsg.c gesmsg.h			\
	   unidev.h unitap.h

unibck.o:  unibck.c unibck.h			\
	   subrou.h 

unidev.o:  unidev.c unidev.h			\
	   subrou.h 

unipar.o:  unipar.c unipar.h			\
	   subrou.h 

unisig.o:  unisig.c unisig.h			\
	   subrou.h 

unisch.o:  unisch.c unisch.h			\
	   subrou.h 

unitap.o:  unitap.c unitap.h

uniprc.o:  uniprc.c uniprc.h

subrou.o:  subrou.c subrou.h


gesbck.h:  					\
	   unibck.h				\
	   unidev.h
	   touch $@

gestap.h:  					\
	   unipar.h				\
	   unitap.h				\
	   unisch.h				\
	   unidev.h				\
	   subrou.h
	   touch $@

gesfil.h:  					\
	   unibck.h unisch.h
	   touch $@

unibck.h					\
unidev.h					\
unipar.h					\
unitap.h					\
	:  subrou.h
	   touch $@

toremake:  Makefile
	   touch toremake
	   - rm -f $(OBJS) $(LIBBCK)
#--------------------------------------------------------------------
#definitions
#--------------------------------------------------------------------
CC=gcc
LD=gcc
CFLAGS= -Wall $(OPTIME)
LIBBCK=libbck.a
#--------------------------------------------------------------------
