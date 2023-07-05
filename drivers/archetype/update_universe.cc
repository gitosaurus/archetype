#include "update_universe.hh"

#include "Object.hh"
#include "Serialization.hh"
#include "Universe.hh"
#include "WrappedOutput.hh"
#include "ConsoleOutput.hh"
#include "StringInput.hh"
#include "StringOutput.hh"
#include "Value.hh"

namespace archetype {

  class EchoingInput : public IUserInput {

  public:
    EchoingInput(UserInput input, UserOutput output):
      input_(input),
      output_(output)
    { }

    virtual char getKey() override {
      std::string key_str(input_->getKey(), 1);
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

string update_universe(Storage& in, Storage& out, string input, int width) {
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
  out << Universe::instance();
  return dynamic_cast<StringOutput*>(str_output.get())->getOutput();
}

} // namespace archetype
