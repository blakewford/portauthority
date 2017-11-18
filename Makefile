CC:= gcc
SRC:= main.cpp
DEBUG:=

cluster-profile: $(SRC)
	$(CC) $(SRC) $(DEBUG) -o $@ -lelf

outline: outline.cpp
	g++ -std=c++11 $< $(DEBUG) -o $@ -lpthread
#	g++ -std=c++11 $< $(DEBUG) -o $@ -lpthread -DSIMAVR

clean:
	-@rm cluster-profile
