# Assignment 3 – HTTP Client using TCP Sockets

## Files
| File | Purpose |
|---|---|
| `a3_http_client.c` | Mandatory: raw-socket HTTP/1.1 GET client + performance metrics |
| `a3_https_client.c` | Optional/advanced: HTTPS client with OpenSSL |
| `Makefile` | `make` → HTTP client; `make https` → HTTPS client (needs `libssl-dev`) |

## Run
```
# 1. web server folder
mkdir CN_Web_Server && cd CN_Web_Server
#    put index.html, notes.txt, sample.pdf, image.jpg here
python3 -m http.server 8000

# 2. client (another terminal, in this folder)
make
./a3_http_client http://localhost:8000/sample.pdf     # or run with no args and type URL
./a3_http_client http://localhost:8000/image.jpg
./a3_http_client http://example.com/                  # part (a): any real web server

# optional HTTPS
sudo apt install libssl-dev && make https
./a3_https_client example.com /
```
Downloaded files are saved as `downloaded_<name>` (e.g. `downloaded_sample.pdf`). Verify with `cmp downloaded_sample.pdf CN_Web_Server/sample.pdf`.

## How the HTTP client works
1. Parse URL → host, port (default 80), path (default `/`).
2. `getaddrinfo` (DNS) → `socket` → `connect` (TCP).
3. Send `GET <path> HTTP/1.1`, `Host`, `Connection: close`.
4. `recv` in a loop until the server closes. Bytes are collected until `\r\n\r\n` (end of headers); status code is parsed; the rest is the body and is written with `fwrite` in binary mode (so PDF/JPG are not corrupted).
5. Metrics (using `CLOCK_MONOTONIC`):
   - DNS time, TCP connect time
   - **Response time** = request sent → first byte
   - **Download time** = first byte → connection close
   - **Data size** = body bytes (headers counted separately)
   - **Throughput** = bytes ÷ download time (KB/s and Mbps)

## How the HTTPS client works
TCP connect to port 443 → `SSL_CTX_new(TLS_client_method)` → load CA store → `SSL_VERIFY_PEER` → SNI + hostname check → `SSL_connect` (handshake) → verify result → send GET with `SSL_write` → `SSL_read` (decrypted data). Prints TLS version, cipher suite, certificate subject/issuer, **TCP connection time, SSL handshake time, RTT, data transfer rate**.

## Analysis points (for report)
- Compare metrics for html / txt / jpg / pdf in a table; small files are dominated by connection + response time, large files by throughput.
- Localhost throughput is unrealistically high; also test a remote site.
- HTTPS adds the handshake (extra 1–2 RTT) plus encryption overhead.

## What you can add / change
- **Latency/RTT:** run the request 10 times and average; add `ping`-style measurement of connect time.
- **Throughput:** test with a 5–50 MB file (`head -c 20M /dev/urandom > big.bin`).
- **PDR / loss:** use `tc qdisc add dev lo root netem loss 5%` and show throughput drop (TCP retransmits).
- Handle `301/302` redirects (parse `Location:`), `404` pages, chunked transfer encoding, `Content-Length` check.
- Add socket timeout (`SO_RCVTIMEO`), progress display, keep-alive comparison.
- HTTPS: print certificate expiry, force TLS 1.2 vs 1.3 (`SSL_CTX_set_max_proto_version`) and compare handshake time.
- CRC: not required here (TCP/TLS already guarantee integrity); you can add a CRC/`sha256sum` check of the downloaded file against the original.
- Wireshark filter `tcp.port==8000` / `http`, `tls.handshake` for screenshots.

## Viva questions
Why `Connection: close`? Why `\r\n\r\n`? Why binary-safe writing? What is `Content-Length`? HTTP/1.0 vs 1.1? TLS handshake steps? What does certificate verification check?
