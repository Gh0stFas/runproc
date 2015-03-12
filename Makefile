CC = g++
INC = ./inc
LIB = ./lib
BIN = ./bin
OPT = -O2
LIBFLAGS = ${OPT} -c -I${INC} -D_GNU_SOURCE
# Starting with GCC 4.6, I've had to place the -Wl,--no-as-needed option for
# the linker so it can link the math library properly. Without this option you
# would have to put the math library in an order dependent manner....bleh
CFLAGS = ${opt} -lm -L${LIB} -I${INC} -D_GNU_SOURCE
OBJ = libpopen2.a
RUNPROC = runproc
RUNPROC2 = runproc2
TESTTP = testtp

all : ${RUNPROC} ${RUNPROC2} ${TESTTP}

${TESTTP} : ${OBJ} testtp.cc
	${CC} -o ${TESTTP} testtp.cc ${CFLAGS}

${RUNPROC} : ${OBJ} runproc.cc ${INC}/popen2.h ${LIB}/popen2.cc
	${CC} -o ${RUNPROC} runproc.cc ${CFLAGS} -lpopen2

${RUNPROC2} : ${OBJ} runproc2.cc ${INC}/popen2.h ${LIB}/popen2.cc
	${CC} -o ${RUNPROC2} runproc2.cc ${CFLAGS} -lpopen2 -lpthread

libpopen2.a : ${LIB}/popen2.cc ${INC}/popen2.h
	${CC} ${LIBFLAGS} -o ${LIB}/popen2.o ${LIB}/popen2.cc
	ar -cvq ${LIB}/libpopen2.a ${LIB}/popen2.o
	
clean :
	/bin/rm -f ${RUNPROC} ${RUNPROC2} ${TESTTP} ${LIB}/*.a ${LIB}/*/*.o ${BIN}/*

install :
	/bin/mkdir -p ${BIN}
	/bin/mv ${RUNPROC} ${RUNPROC2} ${TESTTP} ${BIN}
