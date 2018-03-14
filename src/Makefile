CC:= g++
SRC:= main.cpp
DEBUG:=

cluster-profile: $(SRC)
	$(CC) -Ofast -std=c++11 $(SRC) $(DEBUG) -o $@ -lelf -ludis86 -L/usr/local/lib -I. -Irapidjson/include

clean:
	-@rm cluster-profile
