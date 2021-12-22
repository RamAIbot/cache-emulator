CC = g++
OPT = -O3
OPT = -g
WARN = -Wall
CFLAGS = $(OPT) $(WARN) $(INC) $(LIB)

# List all your .cc/.cpp files here (source files, excluding header files)
SIM_SRC = sim_cache.cc cache_general.cc

# List corresponding compiled object files here (.o files)
SIM_OBJ = sim_cache.o cache_general.o
 
#################################

# default rule

all: sim_cache cache_general
	@echo "my work is done here..."


# rule for making sim_cache

sim_cache: $(SIM_OBJ)
	$(CC) -o sim_cache $(CFLAGS) $(SIM_OBJ) -lm
	@echo "-----------DONE WITH sim_cache-----------"


cache_general: $(SIM_OBJ)
	$(CC) -o cache_general $(CFLAGS) $(SIM_OBJ) -lm
	@echo "-----------DONE WITH cache_general-----------"


# generic rule for converting any .cpp file to any .o file
 
.cc.o:
	$(CC) $(CFLAGS)  -c $*.cc

.cpp.o:
	$(CC) $(CFLAGS)  -c $*.cpp


# type "make clean" to remove all .o files plus the sim_cache binary

clean:
	rm -f *.o sim_cache
	rm -f *.o cache_general


# type "make clobber" to remove all .o files (leaves sim_cache binary)

clobber:
	rm -f *.o


