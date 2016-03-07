# Makefile for the DE for players.  This doesn't include the "God" commands/info
#  including itemvalues and hunting for OP equipment for zone.

MALLOC_DEBUG = n
SHELL = /bin/sh

CC	= g++
DEBUG	= -ggdb3

ifdef USE_SHARED
	LDFLAGS += -rdynamic
endif

OPTS     = -O2
CCFLAGS  = -D__UNIX__ -Wno-deprecated -D_DE_
CFLAGS   = -D__UNIX__ -D_DE_
INCLUDES = -I.
LIBS     = -lncurses -lstdc++

# files that would be compiled as dynamic libs if USE_SHARED was defined
#    make sure that they are NOT included in ANY form in OBJS!!!
# The makefile will deal with compiling SH_LIBS as normal files 
# if USE_SHARED isn't defined
SH_LIBS  =

SOURCES  = $(shell find -name "*.cpp")
OBJS     = $(SOURCES:./%.cpp=../ofiles/%.o)

# those below are .c files shared with dms
C_OBJS = ../ofiles/common.o ../ofiles/skills.o

#******************************************************************************#
#******************************************************************************#
#                                                                              #
# Don't mess around below this point.  Everything past here are makefile rules,#
# and NOT dependency lists that need to be changed                             #
#                                                                              #
#*******************************************************************************
#*******************************************************************************

DMS_OBJS = 
PROGS = de

ifndef USE_SHARED
	DMS_OBJS += $(SH_LIBS)
else
	PROGS += $(SH_LIBS:.o=.so)
endif

all: $(PROGS)

de : $(OBJS) message $(C_OBJS)
	$(CC) $(LDFLAGS) -o de $(OBJS) $(C_OBJS) $(LIBS)

-include ../depend/*.d ../depend/*/*.d

../ofiles/%.o: %.cpp
	@mkdir -p ../ofiles/`echo $* | grep -o '.*/'`
	$(CC) -c $(CCFLAGS) $(INCLUDES) $(OPTS) $< -o $@
	@mkdir -p ../depend/`echo $* | grep -o '.*/'`
	@$(CC) -MM $(CCFLAGS) $(INCLUDES) $(OPTS) $*.cpp > $*.dep
	@sed -e 's|.*:|../ofiles/$*.o:|' < $*.dep > ../depend/$*.d
	@rm -f $*.dep

$(C_OBJS): ../ofiles/%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) $(OPTS) -c $< -o $@

ifeq ($(MALLOC_DEBUG),y)

$(MALLOC_OBJS): %.o: %.cpp
	$(CC) $(CFLAGS) $(INCLUDES) -DDEBUG -c $< -o $@
else

$(MALLOC_OBJS): %.o: %.cpp
	$(CC) $(CFLAGS) $(OPTS) $(INCLUDES) -c $< -o $@
endif

$(SH_LIBS:.o=.so): %.so: %.cpp
	$(CC) $(CFLAGS) -fpic -c $< -o $*.o
	ld -shared -o $@ /usr/lib/crtbeginS.o $*.o /usr/lib/crtendS.o

depend .depend:
	@/bin/echo -e "\e[4mDependencies now created during the creation of .o files.\e[0m"
	@echo $(OBJS)

clean:
	rm -f *.o */*.o
	rm -f de
	rm -f ../depend/*.d ../depend/*/*.d
	rm -f ../ofiles/*.o ../ofiles/*/*.o

message:
	@if [ ! -f ../ofiles/skills.o ]; then \
	  /bin/echo -e "\e[1mCompiling skills.o may take a few seconds...\e[0m"; \
	fi

# DO NOT DELETE THIS LINE -- make depend depends on it.
