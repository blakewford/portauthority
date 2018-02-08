CC:= g++
SRC:= main.cpp
DEBUG:=

cluster-profile: $(SRC)
	$(CC) $(SRC) $(DEBUG) -o $@ -lelf

clean:
	-@rm cluster-profile
