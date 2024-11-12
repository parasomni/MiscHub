import socket
import threading
import os
import sys

def handle_client(client):
    try:
        while True:
            http_request = client.recv(1024).decode('utf-8')
            if not http_request:
                break

            request_line = http_request.split('\r\n')[0]
            http_method, path, http_version = request_line.split()
            print(f'Received {http_method} request for {path} -- version: {http_version}')

            if http_method == 'GET':
                file_path = path.lstrip('/')
                if os.path.exists(file_path):
                    if file_path.endswith('.html'):
                        content_type = 'text/html'
                    elif file_path.endswith('.jpeg') or file_path.endswith('.jpg'):
                        content_type = 'image/jpeg'
                    else:
                        content_type = 'application/octet-stream'  # Default content type

                    with open(file_path, 'rb') as file:
                        file_data = file.read()
                    
                    content_length = len(file_data)

                    server_response = (
                        f'HTTP/1.1 200 OK\r\n'
                        f'Content-Type: {content_type}\r\n'
                        f'Content-Length: {content_length}\r\n\r\n'
                    )

                    client.sendall(server_response.encode('utf-8'))
                    client.sendall(file_data)
                    print(f'Sent {file_path} to client')

                else:
                    server_response = (
                        'HTTP/1.1 404 Not Found\r\n'
                        'Content-Length: 0\r\n\r\n'
                    )
                    client.sendall(server_response.encode('utf-8'))
    finally:
        client.close()

def main():
    host = '127.0.0.1'
    port = 8080
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    server.bind((host, port))
    server.listen()
    print(f'Server started on http://{host}:{port}')
    while True:
        try:
            client, address = server.accept()
            print(f'Received request from {address}')
            threading.Thread(target=handle_client, args=(client,)).start()
        except KeyboardInterrupt:
            print('Shutting down server. Waiting for all threads to finish...')
            server.close()
            sys.exit()
        except Exception as error:
            print(f'Error: {error}')
            server.close()
            sys.exit()

if __name__ == '__main__':
    main()


