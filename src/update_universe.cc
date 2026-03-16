#include "update_universe.hh"

#include "Object.hh"
#include "Serialization.hh"
#include "Universe.hh"
#include "WrappedOutput.hh"
#include "StringInput.hh"
#include "StringOutput.hh"
#include "SystemObject.hh"
#include "Value.hh"

#include <sstream>

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

// Produce the parser-level portion of the situation report: proximate objects,
// effective noun phrases (those matching proximate objects), and all verb phrases.
// Output is RDF/Turtle collections, one per line, prefixed by a label.
string sitrep_parser_context() {
  ObjectPtr sys = Universe::instance().getObject(Universe::SystemObjectId);
  SystemObject* system = dynamic_cast<SystemObject*>(sys.get());
  if (not system or not system->parser_) return "";

  const auto& parser = *system->parser_;

  // Helper: join a phrase word list into a single string.
  auto phrase_str = [](const list<Value>& words) -> string {
    ostringstream ss;
    for (auto ii = words.begin(); ii != words.end(); ++ii) {
      if (ii != words.begin()) ss << ' ';
      ss << (*ii)->getString();
    }
    return ss.str();
  };

  // Helper: render an object reference as an RDF URI.
  auto obj_ref = [](int obj_id) -> string {
    ObjectValue ov{obj_id};
    return ov.asRDF();
  };

  ostringstream out;

  // Proximate objects
  if (not parser.proximate_.empty()) {
    out << "PROXIMATE (";
    for (int obj_id : parser.proximate_) {
      out << " " << obj_ref(obj_id);
    }
    out << " )\n";
  }

  // Effective nouns: phrases matching proximate objects
  out << "NOUNS (";
  for (const auto& nm : parser.nounMatches_) {
    if (parser.proximate_.count(nm.second)) {
      out << " ( \"" << phrase_str(nm.first) << "\" " << obj_ref(nm.second) << " )";
    }
  }
  out << " )\n";

  // All verb phrases
  out << "VERBS (";
  for (const auto& vm : parser.verbMatches_) {
    out << " ( \"" << phrase_str(vm.first) << "\" " << obj_ref(vm.second) << " )";
  }
  out << " )\n";

  return out.str();
}

string update_universe(Storage& in, Storage& out, string input, int width,
                       bool sitrep) {
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
    try {
      Value sitrep_val = dispatch_to_universe("SITREP");
      result += "SITREP " + sitrep_val->asRDF() + "\n";
    } catch (const std::exception&) {
      // SITREP not available; silently skip
    }
    result += sitrep_parser_context();
  }

  out << Universe::instance();
  return result;
}

} // namespace archetype
