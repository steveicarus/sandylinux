

# Select the install directory for the SIMBUS files. This is normally
# /usr/local, but may be other places. To build against specific instalations,
# use the command:
#
#  make SIMBUS_ROOT=/what/ever all
#

SIMBUS_ROOT = /usr/local

SIMBUS_LIBDIR = $(SIMBUS_ROOT)/lib
SIMBUS_INCDIR = $(SIMBUS_ROOT)/include

LIBS = -L$(SIMBUS_LIBDIR) -lsimbus

CXXFLAGS = -I$(SIMBUS_INCDIR) -g -O

all: slf_master

O = slf_main.o

slf_master: $O
	$(CXX) -o slf_master $O $(LIBS)

slf_main.o: slf_main.cc
