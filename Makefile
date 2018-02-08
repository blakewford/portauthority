CC:= gcc
SRC:= main.cpp
DEBUG:=

outline: outline.cpp
	g++ -std=c++11 $< $(DEBUG) -o $@ -lpthread
#	g++ -std=c++11 $< $(DEBUG) -o $@ -lpthread -DSIMAVR
