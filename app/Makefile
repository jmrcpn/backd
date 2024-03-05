#--------------------------------------------------------------------
#Executable generation area
#--------------------------------------------------------------------
debug	: 
	   @ $(MAKE) OPTIME="-g" DEBUG="-DDEBUG" exe 
	   @ echo "application compiled in '$@' mode now ready"

prod	:
	   @ $(MAKE) OPTIME="-g2" exe
	   @ echo "application compiled in '$@' mode now ready"

exe	:
	   @ $(MAKE) $(EXE)


clean	:
	   - rm -fr *.o $(EXE)
	   - rm -fr ../bin/*

#--------------------------------------------------------------------
#Equivalences
#--------------------------------------------------------------------
EXE=								\
	backd3							\
	marker							\
	reader							\
	scanner							\


SRC=								\
	backd.c							\
	Makefile

#--------------------------------------------------------------------
#definitions
#--------------------------------------------------------------------
CC	=  gcc
LD	=  gcc
CFLAGS	=  -I ../lib -Wall $(OPTIME)
LIBBCK	=  ../lib/libbck.a
LIBS	=  $(LIBBCK) -luuid -lm
#--------------------------------------------------------------------
#Dependances
#--------------------------------------------------------------------
backd3 :  toremake backd.o
	  @ mkdir -p ../bin
	  @ $(LD) $(LDFLAGS) -o ../bin/$@ backd.o $(LIBS)

marker								\
reader								\
scanner								\
	: backd3
	  @ ln -nsf ./backd3 ../bin/backd-$@

backd.o:  backd.c						\
	  ../lib/modbck.h					\
	  ../lib/modget.h					\
	  ../lib/modsca.h					\
	  ../lib/modtap.h					\
	  ../lib/unipar.h					\
	  ../lib/subrou.h					\
	  $(LIBBCK)

toremake:  Makefile $(LIBBCK)
	   touch toremake
	   - rm -f $(EXE) *.o

#--------------------------------------------------------------------
