

SIMBUS_LIBDIR = /usr/local/test/lib
SIMBUS_INCDIR = /usr/local/test/include

LIBS = -L$(SIMBUS_LIBDIR) -lsimbus

CXXFLAGS = -I$(SIMBUS_INCDIR) -g -O

O = slf_main.o

slf_master: $O
	$(CXX) -o slf_master $O $(LIBS)

slf_main.o: slf_main.cc
