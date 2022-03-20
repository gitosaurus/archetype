// Driver that downloads an ongoing ACX file from the cloud, updates
// it with a single string, and writes back the change.

#include <iostream>
#include <iterator>
#include <algorithm>

#include "google/cloud/storage/client.h"
#include "archetype/update_universe.hh"

// #define SHOW(expr) std::cerr << #expr << " == " << (expr) << std::endl
#define SHOW(expr)

using namespace archetype;
using namespace std;

int main(int argc, char* argv[]) {
  const string bucket_name = "archetype_web_cards";
  if (argc != 3) {
    cerr << "Usage: " << argv[0] << " <ACX binary> <input string>" << endl;
    return 1;
  }
  const string game_identifier = argv[1];
  const string adventure_input = argv[2];
  SHOW(bucket_name);
  SHOW(game_identifier);
  SHOW(adventure_input);

  // Create aliases to make the code easier to read.
  namespace gcs = ::google::cloud::storage;

  // Create a client to communicate with Google Cloud Storage. This client
  // uses the default configuration for authentication and project id.
  auto client = gcs::Client();
  cerr << "Client created" << endl;

  auto reader = client.ReadObject(bucket_name, game_identifier);
  if (!reader) {
    cerr << "Error reading object: " << reader.status() << "\n";
    return 1;
  }

  int width = 40;
  MemoryStorage in_mem, out_mem;
  copy(istreambuf_iterator<char>{reader}, {}, back_inserter(in_mem.bytes()));

  update_universe(in_mem, out_mem, adventure_input, width);

  auto writer = client.WriteObject(bucket_name, game_identifier);
  copy(out_mem.bytes().begin(), out_mem.bytes().end(), ostreambuf_iterator<char>{writer});
  writer.Close();
  if (writer.metadata()) {
    SHOW(*writer.metadata());
  } else {
    cerr << "Error creating object: " << writer.metadata().status() << endl;
    return 1;
  }

  return 0;
}
