CC           = clang
MAKE         = make

FUSE_LIBS    = `pkg-config --libs fuse`
CURL_LIBS    = -lcurl
CFLAGS       = `pkg-config --cflags fuse`
CFLAGS      += -Iinclude
CFLAGS      += -Ilibs/jsmn
CFLAGS      += -pthread
CFLAGS      += -Wall -Wextra -pedantic
CFLAGS      += -g
CFLAGS      += -std=c99
CFLAGS      += -v
CFLAGS      += -fshow-source-location

.PHONY: all clean

all: hnfs

libs/jsmn/jsmn.o:
	$(MAKE) -C libs/jsmn

post.o: post.c include/hnfs/post.h
	$(CC) -c $(CFLAGS) post.c -o post.o

aldn.o: aldn.c include/aldn/aldn.h
	$(CC) -c $(CFLAGS) aldn.c -o aldn.o

hnfs.o: hnfs.c
	$(CC) -c $(CFLAGS)  hnfs.c -o hnfs.o

hnfs: hnfs.o post.o libs/jsmn/jsmn.o aldn.o
	$(CC) $(CFLAGS) $(FUSE_LIBS) $(CURL_LIBS) post.o \
	  libs/jsmn/jsmn.o aldn.o hnfs.o -o hnfs

jsmn_test: jsmn_test.c libs/jsmn/jsmn.o
	$(CC) $(CFLAGS) libs/jsmn/jsmn.o jsmn_test.c -o jsmn_test

clean:
	rm -f hnfs
	rm -f post.o
	rm -f aldn.o
	rm -f hnfs.o
	$(MAKE) -C libs/jsmn clean
