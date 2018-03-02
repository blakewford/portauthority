CC:= g++
SRC:= main.cpp
DEBUG:=

cluster-profile: $(SRC)
	$(CC) $(SRC) $(DEBUG) -o $@ -lelf -ludis86 -L/usr/local/lib -I.

clean:
	-@rm cluster-profile
