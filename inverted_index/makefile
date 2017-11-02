
EXEC = index
all : $(EXEC)

CSRC = comm.c index.c ngrams.c inverted_index.c
CPPSRC = 
FSRC = 


#FFLAGS = -nus
#LDFLAGS = -lcxa -lunwind
#CXX = icc
#FXX = ifort

CFLAGS = -g -Wall
# CFLAGS = -O2
CPPFLAGS = -g -Wall
FFLAGS = -g 
#LDFLAGS = -lstdc++ -lefence
# LDFLAGS = -lstdc++
LDFLAGS =
# CC = gcc
CC = clang
# CXX = g++
CXX = clang++
FXX = gfortran

SRCS = $(CSRC) $(CPPSRC) $(FSRC)
OBJECTS = $(CSRC:.c=.o) $(CPPSRC:.cpp=.o) $(FSRC:.f=.o)

$(EXEC): $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(LDFLAGS)

clean:
	-rm -f *.o
	-rm -f a.out
.cpp.o:
	$(CXX) $(CPPFLAGS) -c $<
.c.o:
	$(CC) $(CFLAGS) -c $<
.F.o:
	$(FXX) $(FFLAGS) -c $<
.f.o:
	$(FXX) $(FFLAGS) -c $<
   
