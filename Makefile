CC = gcc
CFLAGS = -Wall -Ofast -lre2 -lWN -lglib-2.0 -lgflags \
         `icu-config  --cppflags --ldflags` `mecab-config --libs`

all: string-splitter

string-splitter : string-splitter.cpp
	$(CC) string-splitter.cpp -o string-splitter $(CFLAGS)

clean:
	rm -rf string-splitter

