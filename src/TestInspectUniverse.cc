//
//  TestInspectUniverse.cc
//  archetype
//
//  Created by Derek Jones on 2026-03-18.
//  Copyright (c) 2026 Derek Jones. All rights reserved.
//

#include <string>
#include <sstream>

#include "TestInspectUniverse.hh"
#include "TestRegistry.hh"
#include "Universe.hh"
#include "SourceFile.hh"
#include "TokenStream.hh"
#include "Capture.hh"
#include "inspect_universe.hh"

using namespace std;

namespace archetype {
    ARCHETYPE_TEST_REGISTER(TestInspectUniverse);

    // A typed object (widget) and a null-parent object (thing) with an attribute.
    // The setup method registers vocabulary and sets proximity.
    static char program[] =
    "type widget based on null\n"
    "  desc : \"generic widget\"\n"
    "methods\n"
    "  'NAME'     : \"gadget|widget\" -> system\n"
    "  'ANNOUNCE' : 'PRESENT' -> system\n"
    "end\n"
    "\n"
    "widget gizmo\n"
    "  desc : \"a nice gizmo\"\n"
    "methods\n"
    "  'NAME' : \"gizmo\" -> system\n"
    "end\n"
    "\n"
    "null thing\n"
    "  desc : \"just a thing\"\n"
    "methods\n"
    "  'NAME'     : \"thing|thingamajig\" -> system\n"
    "  'ANNOUNCE' : 'PRESENT' -> system\n"
    "end\n"
    "\n"
    "null setup\n"
    "methods\n"
    "  'go' : {\n"
    "    'INIT PARSER' -> system\n"
    "    'OPEN PARSER' -> system\n"
    "    'NOUN LIST'   -> system\n"
    "    'NAME' -> gizmo\n"
    "    'NAME' -> thing\n"
    "    'CLOSE PARSER' -> system\n"
    "    'ROLL CALL' -> system\n"
    "    'ANNOUNCE' -> gizmo\n"
    "    'ANNOUNCE' -> thing\n"
    "  }\n"
    "end\n"
    ;

    // Every shape a declared attribute can take.  'alias' names an attribute of
    // its own object, which is the aliasing the language has always allowed and
    // which the RDF deliberately does not chase.
    static char declared[] =
    "null target\n"
    "  desc : \"the target\"\n"
    "end\n"
    "\n"
    "null holder\n"
    "  spelled_out : \"a literal\"\n"
    "  an_object   : target\n"
    "  itself      : self\n"
    "  alias       : spelled_out\n"
    "  computed    : 3 + 4\n"
    "end\n"
    ;

    static string getTurtleOutput_(bool include_methods = false) {
        // Serialize the universe
        MemoryStorage mem;
        mem << Universe::instance();

        // Inspect it
        ostringstream ttl;
        inspect_universe(mem, ttl, include_methods);
        return ttl.str();
    }

    void TestInspectUniverse::testNullParentType_() {
        Universe::destroy();

        TokenStream t(make_source_from_str("inspect_test", program));
        ARCHETYPE_TEST(Universe::instance().make(t));

        string ttl = getTurtleOutput_();

        // 'thing' has a null parent, so it should get "a type:null"
        ARCHETYPE_TEST(ttl.find("obj:thing a type:null") != string::npos);

        // 'gizmo' inherits from widget, so it should get "a type:widget"
        ARCHETYPE_TEST(ttl.find("obj:gizmo a type:widget") != string::npos);

        // 'widget' is a prototype with a parent, so it should be "a rdfs:Class"
        ARCHETYPE_TEST(ttl.find("type:widget a rdfs:Class") != string::npos);

        // 'setup' also has a null parent
        ARCHETYPE_TEST(ttl.find("obj:setup a type:null") != string::npos);
    }

    void TestInspectUniverse::testVocabSyntax_() {
        Universe::destroy();

        TokenStream t(make_source_from_str("inspect_test", program));
        ARCHETYPE_TEST(Universe::instance().make(t));

        // Execute setup to register vocabulary
        Capture capture;
        Statement stmt = make_stmt_from_str("'go' -> setup");
        stmt->execute();

        string ttl = getTurtleOutput_(/* include_methods = */ true);

        // Vocabulary entries must not start with "; " — the first predicate
        // in a subject block must not be preceded by a semicolon.
        ARCHETYPE_TEST(ttl.find("obj:thing\n    ; ") == string::npos);
        ARCHETYPE_TEST(ttl.find("obj:gizmo\n    ; ") == string::npos);

        // Per-object phrases are emitted under the unified predicate.
        ARCHETYPE_TEST(ttl.find("archetype:matchesPhrase \"thing\"") != string::npos);
        ARCHETYPE_TEST(ttl.find("archetype:matchesPhrase \"thingamajig\"") != string::npos);
        ARCHETYPE_TEST(ttl.find("archetype:matchesPhrase \"gizmo\"") != string::npos);

        // ANNOUNCE made both objects proximate, so their phrases are live too.
        ARCHETYPE_TEST(ttl.find("archetype:matchesNow \"thing\"") != string::npos);
        ARCHETYPE_TEST(ttl.find("archetype:matchesNow \"gizmo\"") != string::npos);
    }

    void TestInspectUniverse::testProximateSyntax_() {
        Universe::destroy();

        TokenStream t(make_source_from_str("inspect_test", program));
        ARCHETYPE_TEST(Universe::instance().make(t));

        // Execute setup to register vocabulary and proximity
        Capture capture;
        Statement stmt = make_stmt_from_str("'go' -> setup");
        stmt->execute();

        string ttl = getTurtleOutput_(/* include_methods = */ true);

        // Proximate now lives on archetype:parser, not archetype:situation.
        ARCHETYPE_TEST(ttl.find("archetype:parser a archetype:SystemParser") != string::npos);
        // First object in the list must not be preceded by a comma.
        ARCHETYPE_TEST(ttl.find("archetype:proximate ,") == string::npos);
        ARCHETYPE_TEST(ttl.find("archetype:proximate obj:") != string::npos);
    }

    void TestInspectUniverse::testParserBlock_() {
        Universe::destroy();

        TokenStream t(make_source_from_str("inspect_test", program));
        ARCHETYPE_TEST(Universe::instance().make(t));

        // Run setup, then hand the parser a command so playerCommand_ and
        // normalized_ are populated before we inspect.
        Capture capture;
        Statement setup = make_stmt_from_str(
            "{'go' -> setup; 'PLAYER CMD' -> system; \"look at gizmo\" -> system}");
        setup->execute();

        string ttl = getTurtleOutput_(/* include_methods = */ true);

        // The parser is serialized as a first-class archetype:SystemParser.
        ARCHETYPE_TEST(ttl.find("archetype:parser a archetype:SystemParser") != string::npos);

        // CLOSE PARSER leaves the parser in NOUNS mode.
        ARCHETYPE_TEST(ttl.find("archetype:mode \"nouns\"") != string::npos);

        // The raw command round-trips verbatim; the normalized form is
        // present (exact spacing is an implementation detail of parse()).
        ARCHETYPE_TEST(ttl.find("archetype:playerCommand \"look at gizmo\"") != string::npos);
        ARCHETYPE_TEST(ttl.find("archetype:normalized ") != string::npos);
    }

    // An attribute written in source holds the expression it was written as and
    // is evaluated afresh on every access, so the dump has to choose which ones
    // it may ask.  The predicate is pinned here rather than through the Turtle
    // because the interesting negative case is 'read': were it ever to become
    // materializable, evaluating it would take a line from the player, and a
    // test that discovered that by hanging would be a poor way to find out.
    void TestInspectUniverse::testMaterializable_() {
        Universe::destroy();

        // No operator, nothing read, no context beyond self.
        ARCHETYPE_TEST(make_expr_from_str("\"a literal\"")->isMaterializable());
        ARCHETYPE_TEST(make_expr_from_str("target")->isMaterializable());
        ARCHETYPE_TEST(make_expr_from_str("self")->isMaterializable());

        // An operator can send a message, and a method may do anything.
        ARCHETYPE_TEST(not make_expr_from_str("3 + 4")->isMaterializable());
        ARCHETYPE_TEST(not make_expr_from_str("self -> 'NAME'")->isMaterializable());

        // Leaves all, and none of them free: two take from the player, and the
        // rest read a dispatch context an inspection is standing outside of.
        ARCHETYPE_TEST(not make_expr_from_str("read")->isMaterializable());
        ARCHETYPE_TEST(not make_expr_from_str("key")->isMaterializable());
        ARCHETYPE_TEST(not make_expr_from_str("sender")->isMaterializable());
        ARCHETYPE_TEST(not make_expr_from_str("message")->isMaterializable());
        ARCHETYPE_TEST(not make_expr_from_str("each")->isMaterializable());
    }

    // The graph a game builds in source -- what contains what, which branch a
    // question leads to -- lives in attributes that are declared and never
    // assigned.  Dropping those left it undiscoverable from the RDF.
    void TestInspectUniverse::testDeclaredAttributes_() {
        Universe::destroy();

        TokenStream t(make_source_from_str("declared_test", declared));
        ARCHETYPE_TEST(Universe::instance().make(t));

        string ttl = getTurtleOutput_();

        // A name standing for another object is the edge that was being lost.
        ARCHETYPE_TEST(ttl.find("attr:an_object obj:target") != string::npos);

        // Literals were never in doubt, and 'self' resolves to the holder.
        ARCHETYPE_TEST(ttl.find("attr:spelled_out \"a literal\"") != string::npos);
        ARCHETYPE_TEST(ttl.find("attr:itself obj:holder") != string::npos);

        // An alias is a live reference to whatever the other attribute is now.
        // Rendering it would mean evaluating that one, so it is left out: the
        // RDF is meant to be less than the save file, not a second encoding of
        // the program.
        ARCHETYPE_TEST(ttl.find("attr:alias") == string::npos);

        // Same reasoning, arrived at sooner: an operator is never asked.
        ARCHETYPE_TEST(ttl.find("attr:computed") == string::npos);
    }

    void TestInspectUniverse::runTests_() {
        testNullParentType_();
        testVocabSyntax_();
        testProximateSyntax_();
        testParserBlock_();
        testMaterializable_();
        testDeclaredAttributes_();
    }
}
