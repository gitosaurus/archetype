cmake_minimum_required(VERSION 3.10)
project(archetype)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(archetype
Capture.cc
ConsoleInput.cc
Expression.cc
FileStorage.cc
Keywords.cc
Object.cc
PagedOutput.cc
ReadEvalPrintLoop.cc
Serialization.cc
SourceFile.cc
Statement.cc
SystemObject.cc
SystemParser.cc
SystemSorter.cc
TestExpression.cc
TestIdIndex.cc
TestObject.cc
TestRegistry.cc
TestSerialization.cc
TestSourceFile.cc
TestStatement.cc
TestSystemObject.cc
TestSystemParser.cc
TestSystemSorter.cc
TestTokenStream.cc
TestUniverse.cc
TestValue.cc
TestWrappedOutput.cc
Token.cc
TokenStream.cc
Universe.cc
Value.cc
Wellspring.cc
WrappedOutput.cc
inspect_universe.cc
update_universe.cc
main.cc
)

include(GNUInstallDirs)
install(TARGETS archetype RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
