# Assignment – Simulation of Domain Name Resolution using UDP

## Files
| File | Purpose |
|---|---|
| `dns_server.c` | UDP DNS server with hostname→IP table, error handling, loss/delay/corruption simulation |
| `dns_client.c` | Client with cache (TTL), timeout + retry, CRC check, statistics |
| `crc32.h` | CRC-32 helper (`crc_pack` / `crc_unpack`) |
| `Makefile` | `make` builds both |

## Run
```
make
./dns_server                  # normal, UDP port 5353
./dns_server 30 100 10        # simulate 30% loss, 100 ms delay, 10% corrupted replies
./dns_client                  # or ./dns_client <server_ip>
```
Client commands: `<domain>`, `cache`, `stats`, `quit`. Try: `google.com` (resolved) → `google.com` again (CACHE HIT) → `foo.bar` (NXDOMAIN) → `bad_name` (invalid format) → `stats`.

## Protocol (all messages end with `|CRC32`)
- Query: `QUERY|<domain>`
- Replies: `OK|<ip>|<ttl>`, `NXDOMAIN`, `ERROR|INVALID_FORMAT`

## Requirement mapping
| Requirement | Where |
|---|---|
| DNS database (dictionary) | `table[]` in `dns_server.c` |
| Name resolution function | server lookup loop; client `resolve` logic in `main` |
| Caching (client side) | `cache[]`, `cache_find`, `cache_put` (8 entries, FIFO, TTL 30 s) |
| NXDOMAIN | server returns `NXDOMAIN` for unknown names |
| Timeout simulation | server drops packets (`loss %`) or delays (`delay ms` > 2 s); client `SO_RCVTIMEO` 2 s, 3 retries |
| Invalid format | server `valid_name()`: length ≤253, has a dot, only letters/digits/`-`/`.`; or message not `QUERY|…` |

## Analysis: suitability of UDP for DNS (answer)
- **Overhead:** 8 B UDP header vs 20 B TCP; no 3-way handshake or connection state.
- **Latency:** one request + one reply = 1 RTT (TCP needs ≥2–3 RTT). Caching removes even that.
- **Reliability:** UDP does not retransmit, but DNS messages are tiny so the application handles loss with timeout + retry (simulated here); CRC detects corruption.
- **Scalability:** the server keeps no per-client state, so it serves huge numbers of queries.
- **Limitations:** large replies (>512 B, DNSSEC) and zone transfers use TCP; UDP is open to spoofing and amplification attacks.

## Metrics (printed by `stats`)
Cache hit ratio, datagrams sent, replies OK, timeouts, corrupted replies, **PDR = replies ÷ sent × 100**, loss %, **RTT avg/min/max, jitter**, **throughput** (bytes ÷ total RTT).
Experiment table for the report: run the server with loss 0/10/30/50 %, query 20 domains, record PDR, retries and average RTT.

## What you can add / change
- **CRC:** already included; `./dns_server 0 0 20` corrupts 20% of replies → `[CRC ERROR]`.
- Transaction ID in each query so replies are matched to queries (as in real DNS).
- Negative caching of NXDOMAIN, LRU instead of FIFO, per-record TTL in the table.
- Multiple IPs per name (round robin), record types (A, MX, CNAME), reverse lookup.
- Server forwarding to a second "upstream" server (recursive resolution), multi-threaded server.
- Load the DNS table from a file; write a log file; exponential back-off between retries.
- Change port: `#define PORT` in both files. Change timeout/retries: `TIMEOUT_S`, `MAX_RETRY`.
- Wireshark filter `udp.port==5353`.

## Viva questions
Why UDP for DNS? When does DNS use TCP? What is TTL? What is NXDOMAIN? What is the difference between iterative and recursive queries? How does the client detect loss?
