CC           = cc
MAKE         = make

FUSE_LIBS    = `pkg-config --libs fuse`
CURL_LIBS    = -lcurl
PTHREAD_LIBS = -pthread
CFLAGS       = `pkg-config --cflags fuse`
CFLAGS      += -Iinclude
CFLAGS      += -Ilibs/jsmn
CFLAGS      += -Wall -Wextra -pedantic
CFLAGS      += -g

.PHONY: all clean

all: hnfs

libs/jsmn/jsmn.o:
	$(MAKE) -C libs/jsmn

post.o: post.c include/hnfs/post.h
	$(CC) -c $(CFLAGS) post.c -o post.o

aldn.o: aldn.c include/aldn/aldn.h
	$(CC) -c $(CFLAGS) aldn.c -o aldn.o

hnfs: hnfs.c include/hnfs/post.h post.o libs/jsmn/jsmn.o aldn.o
	$(CC) $(CFLAGS) $(FUSE_LIBS) $(CURL_LIBS) $(PTHREAD_LIBS) post.o \
	  libs/jsmn/jsmn.o aldn.o hnfs.c -o hnfs

clean:
	rm -f hnfs
	rm -f post.o
	$(MAKE) -C libs/jsmn clean
