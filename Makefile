CXX     = g++
CFLAGS  = -march=corei7 -O3 -fopenmp
LD	= $(CXX)
LDFLAGS	= -fopenmp
OBJS    = solar_system.o model.o constants.o
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

clean :
	$(RM) -f $(BINARY) $(OBJS)

