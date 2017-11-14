CC:= gcc
SRC:= main.cpp
DEBUG:=

cluster-profile: $(SRC)
	$(CC) $(SRC) $(DEBUG) -o $@ -lelf

outline: outline.cpp
	$(CC) $< $(DEBUG) -o $@ -lpthread

clean:
	-@rm cluster-profile
