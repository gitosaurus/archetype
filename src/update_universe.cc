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
    try {
      Value sitrep_val = dispatch_to_universe("SITREP");
      rdf_out << "SITREP " << sitrep_val->asRDF() << "\n";
    } catch (const std::exception&) {
      // SITREP not available; silently skip
    }
    write_parser_rdf(rdf_out, /* with_prefixes = */ true);
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
