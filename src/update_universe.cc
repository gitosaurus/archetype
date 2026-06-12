#include "update_universe.hh"

#include "Object.hh"
#include "Serialization.hh"
#include "Universe.hh"
#include "WrappedOutput.hh"
#include "StringInput.hh"
#include "StringOutput.hh"
#include "SystemObject.hh"
#include "Value.hh"
#include "inspect_universe.hh"

#include <sstream>
#include <string_view>

namespace archetype {

  class EchoingInput : public IUserInput {

  public:
    EchoingInput(UserInput input, UserOutput output):
      input_(input),
      output_(output)
    { }

    virtual char getKey() override {
      std::string key_str(1, input_->getKey());
      output_->put(key_str);
      return key_str[0];
    }

    virtual std::string getLine() override {
      std::string line = input_->getLine();
      output_->put(line);
      output_->endLine();
      return line;
    }

    virtual bool atEOF() const override {
      return input_->atEOF();
    }

  private:
    UserInput input_;
    UserOutput output_;

  };

using namespace std;

// Sanitize a SITREP key for use as a Turtle local name.  The game-side
// convention is that keys are short identifier-ish words ("location",
// "exits"), so anything outside [A-Za-z0-9_-] is replaced with '_'.  Leading
// digits get an underscore prefix.  An empty or all-invalid key becomes "_".
static string sanitize_local_name(string_view raw) {
  if (raw.empty()) return "_";
  string out;
  out.reserve(raw.size());
  for (char ch : raw) {
    bool ok = (ch >= 'A' and ch <= 'Z') or (ch >= 'a' and ch <= 'z') or
              (ch >= '0' and ch <= '9') or ch == '_' or ch == '-';
    out += ok ? ch : '_';
  }
  if (out[0] >= '0' and out[0] <= '9') out = "_" + out;
  return out;
}

// Walk the list returned by 'SITREP' — a sequence of two-element lists of
// the form [ "key" value ] — and emit each pair as its own RDF predicate on
// archetype:situation.  Example input structure:
//   pair( pair("location", pair(obj:cell, undef)),
//         pair( pair("exits", pair(<list>, undef)), ... ) )
static void write_sitrep_rdf(ostream& out, const Value& sitrep_val) {
  out << "archetype:situation a archetype:SituationReport";
  Value node = sitrep_val->clone();
  while (node->isDefined()) {
    Value item = node->head();
    if (not item->isDefined()) break;
    Value key_val = item->head();
    Value value = item->tail()->head();
    if (key_val->isDefined() and value->isDefined()) {
      string key = sanitize_local_name(key_val->getString());
      out << "\n    ; archetype:" << key << " " << value->asRDF();
    }
    node = node->tail();
  }
  out << " .\n\n";
}

Value dispatch_to_universe(string message) {
  ObjectPtr main_object = Universe::instance().getObject("main");
  if (not main_object) {
    throw invalid_argument("No 'main' object");
  }
  if (Universe::instance().ended()) {
    throw invalid_argument("Universe has ended");
  }
  int start_id = Universe::instance().Messages.index(message);
  Value start{new MessageValue{start_id}};
  Value result = Object::send(main_object, std::move(start));
  if (result->isSameValueAs(Value{new AbsentValue})) {
    throw invalid_argument("No method for '" + message + "' on main object");
  }
  return result;
}

string update_universe(Storage& in, Storage& out, string input, int width,
                       bool sitrep, bool inspect) {
  // Paging, no; wrapping, yes.
  UserOutput str_output{new StringOutput};
  UserOutput wrapped{new WrappedOutput{str_output, width}};
  Universe::instance().setOutput(wrapped);
  UserOutput user_output = Universe::instance().output();
  UserInput str_input{new StringInput{input}};
  UserInput echo_input{new EchoingInput(str_input, user_output)};
  Universe::instance().setInput(echo_input);
  in >> Universe::instance();
  try {
    dispatch_to_universe("UPDATE");
  } catch (const archetype::QuitGame&) {
    Universe::instance().endItAll();
  }
  string result = dynamic_cast<StringOutput*>(str_output.get())->getOutput();

  if (sitrep and not Universe::instance().ended()) {
    ostringstream rdf_out;
    write_parser_rdf(rdf_out, /* with_prefixes = */ true);
    if (Universe::instance().Messages.find("SITREP") >= 0) {
      try {
        Value sitrep_val = dispatch_to_universe("SITREP");
        if (sitrep_val->isDefined()) {
          write_sitrep_rdf(rdf_out, sitrep_val);
        }
      } catch (const std::exception&) {
        // SITREP dispatched but failed at runtime; silently skip
      }
    }
    result += rdf_out.str();
  }

  if (inspect and not Universe::instance().ended()) {
    ostringstream rdf_out;
    dump_universe_rdf(rdf_out);
    result += rdf_out.str();
  }

  out << Universe::instance();
  return result;
}

} // namespace archetype
