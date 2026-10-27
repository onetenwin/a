CC=gcc
CFLAGS=-Wall -O2
all: dns_server dns_client
dns_server: dns_server.c crc32.h
	$(CC) $(CFLAGS) $< -o $@
dns_client: dns_client.c crc32.h
	$(CC) $(CFLAGS) $< -o $@
clean:
	rm -f dns_server dns_client
