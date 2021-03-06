CXX     = g++
CFLAGS  = -march=corei7-avx -O3 -fopenmp -flto -pg -std=c++0x
LD	= $(CXX)
LDFLAGS	= -fopenmp -flto -fwhole-program -pg 
SOURCES = solar_system.cpp model.cpp constants.cpp
OBJS    = ${SOURCES:.cpp=.o}
BINARY  = solar_system
RM      = rm
# clear out all suffixes
.SUFFIXES:
# list only those we use
.SUFFIXES: .o .cpp

# define a suffix rule for .c -> .o
.cpp.o :
	$(CXX) $(CFLAGS) -c $<

all : solar_system

solar_system : $(OBJS)
	$(CXX) -o $(BINARY) $(LDFLAGS) $(OBJS)

assembly :
	$(CXX) $(CFLAGS) -fverbose-asm -S $(SOURCES)

clean :
	$(RM) -f $(BINARY) $(OBJS)

