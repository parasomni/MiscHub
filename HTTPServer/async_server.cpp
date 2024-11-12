#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/filesystem.hpp>
#include <fstream>
#include <iostream>

/*
An asynchronous server handles requests in a non-blocking way so the program can
continue executing other tasks while waiting for the actual response.
This design is efficient as it doesn't require threads to handle multiple
connections and can handle a large number of connections at the same time.

In general, an asynchronous operation returns immediately allowing the program
to continue executing other tasks. When a certain event occurs, the asynchronous
operation calls a callback function to handle the event.
All asynchronous operations are managed by an execution context which provides
the necessary environment to run asynchronous operations and is responsible for
their execution.

Compiling: g++ -std=c++17 -o async_server async_server.cpp -lboost_system
-lboost_filesystem -Wall -Wextra
*/

class HTTPServer {
public:
  HTTPServer(boost::asio::io_context &io_context, int port)
      : server_acceptor(io_context, boost::asio::ip::tcp::endpoint(
                                        boost::asio::ip::tcp::v4(), port)) {
    std::cout << "Server started on http://" << server_acceptor.local_endpoint()
              << std::endl;
    start_accept();
  }

private:
  // the acceptor listens for incoming connections and is initialized with the
  // io_context, port and address
  boost::asio::ip::tcp::acceptor server_acceptor;
  // the start_accept method sets up an asynchronous operation to accept a
  // connection
  void start_accept() {
    // creates a shared pointer to a socket and initializes it with the same
    // executor as the acceptor
    // the executor executes asynchronous operations and is part of the
    // execution context(io_context)
    auto socket = std::make_shared<boost::asio::ip::tcp::socket>(
        server_acceptor.get_executor());

    // the acceptor sets up an asynchronous operation to accept a new connection
    // and calls a lambda function when the event(a new connection) occurs
    // the lambda function itself performs another asynchronous operation inside
    // the handle_request method to read the request from the client
    // this design immediately calls the start_accept method again to
    // accept another connection while the current connection is still being
    // processed
    server_acceptor.async_accept(
        *socket, [this, socket](boost::system::error_code error_code) {
          std::cout << "New connection from: "
                    << socket->remote_endpoint().address() << std::endl;
          if (!error_code) {
            handle_request(socket);
          }
          start_accept();
        });
  }

  void handle_request(std::shared_ptr<boost::asio::ip::tcp::socket> socket) {
    // creates a shared pointer to a buffer and a request object
    auto buffer = std::make_shared<boost::beast::flat_buffer>();
    auto request = std::make_shared<
        boost::beast::http::request<boost::beast::http::string_body>>();

    // the async_read method reads the request from the client and calls the
    // lambda function when the event(request received) occurs
    // the lambda function itself calls the process_request method to process
    // the request
    // async_read is performed asynchronously so the method returns immediately
    boost::beast::http::async_read(
        *socket, *buffer, *request,
        [this, socket, request, buffer](boost::system::error_code error_code,
                                        std::size_t) {
          if (!error_code) {
            process_request(socket, request);
          }
        });
  }

  void process_request(
      std::shared_ptr<boost::asio::ip::tcp::socket> socket,
      std::shared_ptr<
          boost::beast::http::request<boost::beast::http::string_body>>
          request) {
    // checks if the request method is GET
    if (request->method() == boost::beast::http::verb::get) {
      std::string target(request->target());
      std::string file_path = "." + target;
      std::cout << "GET request " << request->target()
                << " from: " << socket->remote_endpoint().address()
                << std::endl;

      // checks if the file exists and is a regular file
      if (boost::filesystem::exists(file_path) &&
          boost::filesystem::is_regular_file(file_path)) {
        std::string content_type = get_content_type(file_path);
        send_file_response(socket, file_path, content_type);
      } else {
        send_not_found_response(socket);
      }
    } else {
      send_method_not_allowed_response(socket);
    }
  }

  std::string get_content_type(const std::string &path) {
    if (path.rfind(".html") == path.length() - 5) {
      return "text/html";
    } else if (path.rfind(".jpg") == path.length() - 4 ||
               path.rfind(".jpeg") == path.length() - 5) {
      return "image/jpeg";
    }
    // used as default content type for unknown file types
    return "application/octet-stream";
  }

  void send_file_response(std::shared_ptr<boost::asio::ip::tcp::socket> socket,
                          const std::string &file_path,
                          const std::string &content_type) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file) {
      send_not_found_response(socket);
      return;
    }

    std::ostringstream string_stream;
    string_stream << file.rdbuf();
    std::string body = string_stream.str();

    std::cout << "Sending content " << file_path
              << " to: " << socket->remote_endpoint().address() << std::endl;
    auto response = std::make_shared<
        boost::beast::http::response<boost::beast::http::string_body>>(
        boost::beast::http::status::ok, 11);
    response->set(boost::beast::http::field::server, "Boost.Beast/1.0");
    response->set(boost::beast::http::field::content_type, content_type);
    response->body() = body;
    response->prepare_payload();

    // the async_write method sends the response to the client and closes the
    // socket after sending the response
    boost::beast::http::async_write(
        *socket, *response,
        [socket, response](boost::system::error_code error_code, std::size_t) {
          if (error_code) {
            std::cerr << "Error during asnyc write: " << error_code.message()
                      << std::endl;
          }
          socket->shutdown(boost::asio::ip::tcp::socket::shutdown_send);
        });
  }

  void send_not_found_response(
      std::shared_ptr<boost::asio::ip::tcp::socket> socket) {
    std::cout << "Content not found for: "
              << socket->remote_endpoint().address() << std::endl;
    auto response = std::make_shared<
        boost::beast::http::response<boost::beast::http::string_body>>(
        boost::beast::http::status::not_found, 11);
    response->set(boost::beast::http::field::content_type, "text/html");
    response->body() = "404 Not Found";
    response->prepare_payload();

    boost::beast::http::async_write(
        *socket, *response,
        [socket, response](boost::system::error_code error_code, std::size_t) {
          if (error_code) {
            std::cerr << "Error during asnyc write: " << error_code.message()
                      << std::endl;
          }
          socket->shutdown(boost::asio::ip::tcp::socket::shutdown_send);
        });
  }

  void send_method_not_allowed_response(
      std::shared_ptr<boost::asio::ip::tcp::socket> socket) {
    std::cout << "Method not allowed for: "
              << socket->remote_endpoint().address() << std::endl;
    auto response = std::make_shared<
        boost::beast::http::response<boost::beast::http::string_body>>(
        boost::beast::http::status::method_not_allowed, 11);
    response->set(boost::beast::http::field::content_type, "text/html");
    response->body() = "405 Method Not Allowed";
    response->prepare_payload();

    boost::beast::http::async_write(
        *socket, *response,
        [socket, response](boost::system::error_code error_code, std::size_t) {
          if (error_code) {
            std::cerr << "Error during asnyc write: " << error_code.message()
                      << std::endl;
          }
          socket->shutdown(boost::asio::ip::tcp::socket::shutdown_send);
        });
  }
};

int main() {
  try {
    // the io_context creates a new execution context
    boost::asio::io_context io_context;
    HTTPServer server(io_context, 8080);
    // the run method blocks until all asynchronous operations have finished
    // it starts an event loop that waits for events to occur (like http
    // requests)
    io_context.run();

  } catch (std::exception &error) {
    std::cerr << "Exception: " << error.what() << "\n";
  }

  return 0;
}
