# Assignment 2 – Multi-Client Chat/Query Application (TCP & UDP)

## Files
| File | Purpose |
|---|---|
| `students.csv` | Sample data; first line is a header and is skipped |
| `student_db.h` | `find_student()` – CSV search shared by both servers |
| `crc32.h` | CRC-32 helper used by the UDP programs (`crc_pack` / `crc_unpack`) |
| `a2_tcp_server.c`, `a2_tcp_client.c` | 1(a) TCP, port 8080, one pthread per client |
| `a2_udp_server.c`, `a2_udp_client.c` | 1(b) UDP, port 9090, 3 s receive timeout |
| `Makefile` | `make` builds all four programs |

## Run
```
make
./a2_tcp_server            # terminal 1
./a2_tcp_client            # terminal 2, 3, ... (several clients at once)
./a2_udp_server            # terminal 1
./a2_udp_client            # terminal 2
```
Remote machine: `./a2_tcp_client <server_ip>`. Concurrency test:
`for i in $(seq 20); do echo 2310001 | ./a2_tcp_client & done`

## How it works
**TCP server:** `socket → bind → listen → accept` in a loop. Each accepted fd is malloc'd and passed to `pthread_create` (malloc avoids a race on a stack variable); the thread does `recv → CSV search → send → close`, detached so no `join` is needed. Found → full details; not found → `Student Record Not Found`.
**TCP client:** `connect → send roll → recv until 0 (server closed) → close`; prints connect time and RTT.
**UDP server:** `recvfrom` gives the client address, `sendto` replies to it. Only Roll Number + Name are returned.
**UDP client:** `SO_RCVTIMEO` stops it blocking forever if the datagram is lost; CRC verifies the reply.

## 1(c) Comparison (answer)
| Aspect | TCP | UDP |
|---|---|---|
| Connection | Connection-oriented: 3-way handshake, FIN/ACK close | Connectionless: just `sendto`/`recvfrom` |
| Reliability | ACKs, retransmission, checksum → guaranteed delivery | Best effort; may be lost/duplicated/corrupted |
| Ordering | In-order byte stream (sequence numbers) | No ordering; each datagram independent |
| Error recovery | Automatic (retransmit, flow & congestion control) | None; application must handle (timeout, retry, CRC) |
| Communication | Stream, `connect/accept`, one thread per connection, 20 B header | Message based, one socket serves all clients, 8 B header, lower latency |
| Best for | Complete profile retrieval | Quick small verification queries |

## What you can add / change
- **CRC:** already on UDP. Not needed for TCP (TCP checksums already).
- **Latency/RTT:** printed by both clients; loop N queries for avg/min/max.
- **Throughput:** bytes ÷ time (TCP client prints bytes and RTT).
- **PDR:** in the UDP client loop N queries, `PDR = replies/sent × 100` (use `tc qdisc add dev lo root netem loss 20%` to create loss).
- Thread-safe logging (`pthread_mutex_t`), thread pool, `fork()` or `select()` server, load CSV once into an array, validate roll number input.
- Search by name: compare the 2nd CSV field instead of the 1st in `find_student()`.
- Change port: edit `#define PORT`.
- Sequential server: call `handle_client` directly instead of `pthread_create`.
- Wireshark on `lo`: filters `tcp.port==8080`, `udp.port==9090`.

## Viva questions
`SOCK_STREAM` vs `SOCK_DGRAM`? `bind` vs `connect`? Why `SO_REUSEADDR`? Why malloc the fd for threads? Why can UDP lose data? When would you choose UDP over TCP?
