from http.server import HTTPServer, SimpleHTTPRequestHandler

import os


os.chdir("server_files")
server = HTTPServer(("127.0.0.1", 8000),
                    SimpleHTTPRequestHandler)

print("Server running on port 8000")
server.serve_forever()


# server files path

# downloads/server_files/index.html

# command to run

# gcc -D_POSIX_C_SOURCE=200809L local_server_client.c -o local_server_client

# ./local_server_client

#  /index.html