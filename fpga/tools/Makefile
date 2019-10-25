
CPPFLAGS += -I../sys

CXX = $(CROSS_COMPILE)g++

all: slf_tests watch_buttons

slf_tests: slf_tests.o
	$(CXX) -static -o slf_tests slf_tests.o


watch_buttons: watch_buttons.o
	$(CXX) -static -o watch_buttons watch_buttons.o
