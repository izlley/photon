CFLAGS=-Wall -g 
DEBUG=-g
LDFLAGS=-levent -lpthread -lthrift
BINS=photon
GCC=g++
CC=c++
THRIFT_DIR=../thirdparty/thrift-0.9.0/build
GEN_DIR=./gencode/gen-cpp
INC=-I${GEN_DIR} -I${THRIFT_DIR}/include
LIB_DIR=-L${THRIFT_DIR}/lib

GEN_SRC = ./gencode/gen-cpp/Hbase.cpp \
	  ./gencode/gen-cpp/Hbase_types.cpp \
	  ./gencode/gen-cpp/Hbase_constants.cpp
GEN_OBJ = $(GEN_SRC:.cpp=.o)

FILES=photon workqueue hbase-op exec-get exec-post util-parser
OBJECTS = $(addsuffix .o,$(FILES))

all: ${BINS}

photon: $(GEN_OBJ) $(OBJECTS)
	${CC} ${CFLAGS} ${OBJECTS} ${GEN_OBJ} ${LIB_DIR} ${LDFLAGS} -o $@

$(OBJECTS): %.o : %.cc
	${GCC} ${DEBUG} ${INC} -c $< -o $@

$(GEN_OBJ): %.o : %.cpp
	${GCC} ${DEBUG} -DHAVE_NETINET_IN_H -DHAVE_INTTYPES_H ${INC} -c $< -o $@

clean:
	/bin/rm -rf ${BINS} *.o core *.core
