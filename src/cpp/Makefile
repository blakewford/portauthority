CC:= g++
SRC:= main.cpp
DEBUG:=

authority: $(SRC)
	$(CC) -Ofast -std=c++11 $(SRC) $(DEBUG) -o $@ -lelf -ludis86 -L/usr/local/lib -I. -Irapidjson/include

clean:
	-@rm authority
