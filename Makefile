CC   = gcc
OPTS = -Wall -g

all: server lib client


server: server.o udp.o lfs.o
	$(CC)  -o server server.o udp.o lfs.o

client: client.o udp.o
	$(CC)  -o client client.o udp.o 


lib: mfs.o udp.o
	$(CC) -Wall -Werror -shared -fPIC -g -o libmfs.so mfs.c udp.c

clean:

%.o: %.c 
	$(CC) $(OPTS) -c $< -o $@

clean:
	rm -f server.o client.o udp.o mfs.o lfs.o libmfs.so server client lib

