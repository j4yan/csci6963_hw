
EXEC = index
all : $(EXEC)

CSRC = comm.c index.c ngrams.c inverted_index.c
CXXSRC = 
FSRC = 

CPPFLAGS =
CFLAGS = -Wall
CXXFLAGS = -Wall
FFLAGS = 
#LDFLAGS = -lstdc++ -lefence
# LDFLAGS = -lstdc++
LDFLAGS =
# CC = clang
# CXX = clang++
CC = gcc
CXX = g++
FXX = gfortran

DEBUG ?= 1
ifeq ($(DEBUG), 1)
	FFLAGS += -g
	CFLAGS += -g
	CXXFLAGS += -g
	CPPFLAGS += -DDEBUG
else
	FFLAGS += -O2
	CFLAGS += -O2
	CXXFLAGS += -O2
	CPPFLAGS += -DNDEBUG
endif


SRCS = $(CSRC) $(CXXSRC) $(FSRC)
OBJECTS = $(CSRC:.c=.o) \
		  $(CXXSRC:.cpp=.o) \
		  $(CXXSRC:.cc=.o) \
		  $(FSRC:.f90=.o) \
		  $(FSRC:.f=.o) \
		  $(FSRC:.F=.o)


$(EXEC): $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(LDFLAGS)

clean:
	-rm -f *.o
	-rm -f a.out
.cc.o:
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $<
.cpp.o:
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $<
.c.o:
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $<
.F.o:
	$(FXX) $(FFLAGS) -c $<
.f.o:
	$(FXX) $(FFLAGS) -c $<
.f90.o:
	$(FXX) $(FFLAGS) -c $<
   
