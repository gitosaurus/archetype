cmake_minimum_required(VERSION 3.10)
project(archetype)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra)

# Everything the interpreter itself is made of: no entry point, no test suites.
# Autosave and FileStorage are in here rather than alongside main.cc because
# Object::dispatch calls Autosave::watchingTurns() on every message send.  A
# build that never arms autosave -- the browser -- just leaves that static bool
# false and pays the branch.
set(ARCHETYPE_CORE_SOURCES
Autosave.cc
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
Token.cc
TokenStream.cc
Universe.cc
Value.cc
Wellspring.cc
WrappedOutput.cc
inspect_universe.cc
update_universe.cc
)

# Each suite self-registers from within its own translation unit, so nothing in
# the core refers to any of these and a build can simply leave them all out.
set(ARCHETYPE_TEST_SOURCES
TestAutosave.cc
TestExpression.cc
TestIdIndex.cc
TestInspectUniverse.cc
TestObject.cc
TestPlaySession.cc
TestRegistry.cc
TestSerialization.cc
TestSourceFile.cc
TestStatement.cc
TestSystemObject.cc
TestSystemParser.cc
TestSystemSorter.cc
TestTokenStream.cc
TestUniverse.cc
TestUpdateUniverse.cc
TestValue.cc
TestWrappedOutput.cc
)

if(EMSCRIPTEN)

    # The browser build: no command line to parse and no test runner, but an
    # embind surface for a page to drive one turn at a time.
    add_executable(archetype ${ARCHETYPE_CORE_SOURCES} wasm_api.cc)
    set_target_properties(archetype PROPERTIES SUFFIX ".js")

    # Native WebAssembly exceptions, on compile as well as link.  The
    # interpreter throws in the ordinary course of play -- QuitGame every time
    # a game ends -- and without these an escaping throw aborts the module.
    target_compile_options(archetype PRIVATE -fwasm-exceptions)
    target_link_options(archetype PRIVATE
        -fwasm-exceptions
        -lembind
        -sMODULARIZE=1
        -sEXPORT_NAME=createArchetype
        -sALLOW_MEMORY_GROWTH=1
        -sENVIRONMENT=web
    )

else()

    add_executable(archetype
        ${ARCHETYPE_CORE_SOURCES}
        ${ARCHETYPE_TEST_SOURCES}
        main.cc
    )

    include(GNUInstallDirs)
    install(TARGETS archetype RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

endif()
