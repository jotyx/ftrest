# Makefile for client/server
SERVER          = ftrestd
CLIENT          = ftrest
SERVER_SOURCES  = ftrestd.cpp
CLIENT_SOURCES  = ftrest.cpp

STD             = -std=c++11 
CFLAGS	    = -Wall -Werror -pedantic

CC              = g++
###########################################

all:		$(SERVER) $(CLIENT)

rebuild:	clean all run

run: 		
		./$(SERVER)

$(SERVER):
		$(CC) $(STD) $(CFLAGS) -c $(SERVER_SOURCES)
		$(CC) $(STD) $(CFLAGS) -o $(SERVER) ftrestd.o

$(CLIENT):
		$(CC) $(STD) $(CFLAGS) -c $(CLIENT_SOURCES)
		$(CC) $(STD) $(CFLAGS) -o $(CLIENT) ftrest.o

###########################################

clean:
	rm $(SERVER) $(CLIENT) *.o
