CXX     = g++
CFLAGS  = -march=native -mtune=native -O3 -flto -ffast-math -funroll-loops -pg -std=c++11 -pthread
LD	= $(CXX)
LDFLAGS	= -flto -fwhole-program -pg -pthread 
SOURCES = solar_system.cpp model.cpp constants.cpp args.cpp simulation.cpp jpl_data.cpp jpl_bodies.cpp
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

