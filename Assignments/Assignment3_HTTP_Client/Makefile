CC=gcc
CFLAGS=-Wall -O2
all: a3_http_client
a3_http_client: a3_http_client.c
	$(CC) $(CFLAGS) $< -o $@
# Optional (needs: sudo apt install libssl-dev)
https: a3_https_client.c
	$(CC) $(CFLAGS) a3_https_client.c -o a3_https_client -lssl -lcrypto
clean:
	rm -f a3_http_client a3_https_client downloaded_*
