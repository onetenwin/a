CC=gcc
CFLAGS=-Wall -O2
all: a2_tcp_server a2_tcp_client a2_udp_server a2_udp_client
a2_tcp_server: a2_tcp_server.c student_db.h
	$(CC) $(CFLAGS) $< -o $@ -pthread
a2_udp_server: a2_udp_server.c student_db.h crc32.h
	$(CC) $(CFLAGS) $< -o $@
a2_udp_client: a2_udp_client.c crc32.h
	$(CC) $(CFLAGS) $< -o $@
a2_tcp_client: a2_tcp_client.c
	$(CC) $(CFLAGS) $< -o $@
clean:
	rm -f a2_tcp_server a2_tcp_client a2_udp_server a2_udp_client
