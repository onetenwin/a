# HTTP Client in C — TCP Socket Lab

This project contains a small C HTTP client that connects to a local Python HTTP server, requests `/index.html`, saves the response body to `downloaded_index.html`, and reports response time, download time, total time, downloaded bytes, and approximate throughput.

## Files

- `client.c` — C TCP socket client.
- `index.html` — HTML page served by the Python server.
- `README.md` — these instructions.

## Requirements

- Linux or WSL Ubuntu
- GCC (`build-essential`)
- Python 3

> The C program uses POSIX/Linux socket headers, so run it in Linux/WSL rather than directly in Windows Command Prompt.

## 1. Extract the ZIP and open the project folder

In Ubuntu/WSL, navigate to the extracted folder. For example, if the ZIP is in your Windows Downloads folder:

```bash
cd /mnt/c/Users/YOUR_WINDOWS_USERNAME/Downloads/http_socket_lab
```

Replace `YOUR_WINDOWS_USERNAME` with your Windows username. If you extracted the ZIP somewhere else, use that path instead.

If the project folder is already in your WSL home directory, use:

```bash
cd ~/http_socket_lab
```

## 2. Start the HTTP server (Terminal 1)

From the project directory, run:

```bash
python3 -m http.server 8000
```

Keep this terminal open. It serves `index.html` on `127.0.0.1:8000`.

You can optionally verify the page in your browser at:

http://127.0.0.1:8000/index.html

## 3. Compile the C client (Terminal 2)

Open a second Ubuntu/WSL terminal and change to the same project directory.

If GCC is missing:

```bash
sudo apt update
sudo apt install build-essential
```

Compile:

```bash
gcc -Wall -Wextra -O2 client.c -o client
```

## 4. Run the client

```bash
./client
```

The client requests `/index.html` from the local server and writes the response body to `downloaded_index.html` in the current directory.

Example output (numbers will vary):

```text
HTTP response received
Status: HTTP/1.0 200 OK
Response time: 2.150 ms
Downloaded file: downloaded_index.html
Data size: 240 bytes
Download time: 0.001 seconds
Total time: 0.003 seconds
Throughput: 234.38 KB/s
```

The exact HTML byte count and timings will vary by machine.

## 5. Verify the downloaded file

```bash
cat downloaded_index.html
```

You should see the HTML from `index.html`.

## Troubleshooting

### `connect: Connection refused`
Make sure the Python server is running in Terminal 1 and is using port `8000`.

### `gcc: command not found`
Install GCC with `sudo apt install build-essential`.

### `404 File not found`
Run the Python server from the directory that contains `index.html`.

### Port 8000 is already in use
Stop the other server, or change `SERVER_PORT` in `client.c` and use the same port when starting Python's server.

## Notes on measurements

- **Response time** here means the elapsed time from just before sending the HTTP request until the first response bytes arrive. The first bytes may contain the HTTP headers, not the HTML body.
- **Download time** is measured from the first response bytes until the server closes the connection.
- **Throughput** is the HTML body size divided by download time. Small local files may complete too quickly for a stable measurement.
- This is a simple HTTP/1.0 lab client, not a general-purpose browser. It is intended for the supplied local HTML page.
