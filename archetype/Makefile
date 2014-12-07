OBJECTS = Capture.o \
ConsoleInput.o \
Expression.o \
FileStorage.o \
Keywords.o \
main.o \
Object.o \
ReadEvalPrintLoop.o \
Serialization.o \
SourceFile.o \
Statement.o \
SystemObject.o \
SystemParser.o \
SystemSorter.o \
TestExpression.o \
TestIdIndex.o \
TestObject.o \
TestRegistry.o \
TestSerialization.o \
TestSourceFile.o \
TestStatement.o \
TestSystemObject.o \
TestSystemParser.o \
TestTokenStream.o \
TestUniverse.o \
TestValue.o \
TestWrappedOutput.o \
Token.o \
TokenStream.o \
Universe.o \
Value.o \
Wellspring.o \
WrappedOutput.o

CXXFLAGS=-std=c++0x
archetype: $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@

clean:
	$(RM) -f $(OBJECTS) archetype archetype.exe
