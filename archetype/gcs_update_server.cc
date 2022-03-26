#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include "google/cloud/storage/client.h"
#include "archetype/update_universe.hh"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <optional>
#include <thread>
#include <algorithm>

#define SHOW(expr) std::cerr << #expr << " == " << (expr) << std::endl
// #define SHOW(expr)

namespace be = boost::beast;
namespace asio = boost::asio;
using tcp = boost::asio::ip::tcp;

using namespace std;

string update(string bucket_name, string game_identifier, string adventure_input) {
  using namespace archetype;

  // Create aliases to make the code easier to read.
  namespace gcs = ::google::cloud::storage;

  // Create a client to communicate with Google Cloud Storage. This client
  // uses the default configuration for authentication and project id.
  auto client = gcs::Client();
  cerr << "Client created" << endl;

  auto reader = client.ReadObject(bucket_name, game_identifier);
  if (!reader) {
    cerr << "Error reading object: " << reader.status() << "\n";
    throw runtime_error("Error reading object");
  }

  const int width = 60;
  MemoryStorage in_mem, out_mem;
  copy(istreambuf_iterator<char>{reader}, {}, back_inserter(in_mem.bytes()));

  string str_out = update_universe(in_mem, out_mem, adventure_input, width);

  auto writer = client.WriteObject(bucket_name, game_identifier);
  copy(out_mem.bytes().begin(), out_mem.bytes().end(), ostreambuf_iterator<char>{writer});
  writer.Close();
  if (writer.metadata()) {
    SHOW(*writer.metadata());
  } else {
    cerr << "Error creating object: " << writer.metadata().status() << endl;
    throw runtime_error("Error creating object");
  }
  return str_out;
}

int main(int argc, char* argv[]) try {
  auto address = asio::ip::make_address("0.0.0.0");
  uint16_t port = 8080;
  cout << "Listening on " << address << ":" << port << endl;

  auto handle_session = [](tcp::socket socket) {
    auto report_error = [](be::error_code ec, char const* what) {
      cerr << what << ": " << ec.message() << "\n";
    };

    be::error_code ec;
    for (;;) {
      be::flat_buffer buffer;

      // Read a request
      be::http::request<be::http::string_body> request;
      be::http::read(socket, buffer, request, ec);
      if (ec == be::http::error::end_of_stream) break;
      if (ec) return report_error(ec, "read");

      const string bucket_name = "archetype_web_cards";
      // The request target will be the name of the adventure to update.
      if (request.target().empty() || request.target()[0] != '/') {
	throw invalid_argument("ill-formed adventure name");
      }
      string game_identifier{request.target().begin() + 1, request.target().end()};
      SHOW(game_identifier);
      // The body contains the adventure input.
      istringstream in(request.body());
      ostringstream out;
      string adventure_input;
      while (getline(in, adventure_input)) {
	SHOW(adventure_input);
	out << update(bucket_name, game_identifier, adventure_input);
      }
      string str_output = out.str();

      // Send the response
      // Respond to any request with a "Hello World" message.
      be::http::response<be::http::string_body> response{be::http::status::ok,
                                                         request.version()};
      response.set(be::http::field::server, BOOST_BEAST_VERSION_STRING);
      response.set(be::http::field::content_type, "text/plain");
      response.keep_alive(request.keep_alive());
      response.body() = move(str_output);
      response.prepare_payload();
      be::http::write(socket, response, ec);
      if (ec) return report_error(ec, "write");
    }
    socket.shutdown(tcp::socket::shutdown_send, ec);
  };

  asio::io_context ioc{/*concurrency_hint=*/1};
  tcp::acceptor acceptor{ioc, {address, port}};
  for (;;) {
    auto socket = acceptor.accept(ioc);
    if (!socket.is_open()) break;
    // Run a thread per-session, transferring ownership of the socket
    thread{handle_session, move(socket)}.detach();
  }

  return 0;
} catch (exception const& ex) {
  cerr << "Standard exception caught " << ex.what() << '\n';
  return 1;
}
