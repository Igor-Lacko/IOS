#Makefile, IOS project 2, author: Igor Lacko (xlackoi00)

CC = gcc 
CFLAGS = -std=gnu99 -Wall -Wextra -Werror -pedantic
LDFLAGS = -pthread

all: proj2


proj2: proj2.c proj2.h
	$(CC) $< $(CFLAGS) -o $@ $(LDFLAGS) 


clean: 
	rm -f proj2 proj2.out