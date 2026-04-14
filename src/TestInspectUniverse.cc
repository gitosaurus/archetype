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

    static string getTurtleOutput_() {
        // Serialize the universe
        MemoryStorage mem;
        mem << Universe::instance();

        // Inspect it
        ostringstream ttl;
        inspect_universe(mem, ttl);
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

        string ttl = getTurtleOutput_();

        // Vocabulary entries must not start with "; " — the first predicate
        // in a subject block must not be preceded by a semicolon.
        ARCHETYPE_TEST(ttl.find("obj:thing\n    ; ") == string::npos);
        ARCHETYPE_TEST(ttl.find("obj:gizmo\n    ; ") == string::npos);

        // But they should have the noun phrases
        ARCHETYPE_TEST(ttl.find("archetype:nounPhrase \"thing\"") != string::npos);
        ARCHETYPE_TEST(ttl.find("archetype:nounPhrase \"thingamajig\"") != string::npos);
        ARCHETYPE_TEST(ttl.find("archetype:nounPhrase \"gizmo\"") != string::npos);
    }

    void TestInspectUniverse::testProximateSyntax_() {
        Universe::destroy();

        TokenStream t(make_source_from_str("inspect_test", program));
        ARCHETYPE_TEST(Universe::instance().make(t));

        // Execute setup to register vocabulary and proximity
        Capture capture;
        Statement stmt = make_stmt_from_str("'go' -> setup");
        stmt->execute();

        string ttl = getTurtleOutput_();

        // The proximate list must not have a leading comma before the first object.
        // Valid:   "archetype:proximate\n    obj:gizmo"
        // Invalid: "archetype:proximate\n    , obj:gizmo"
        ARCHETYPE_TEST(ttl.find("archetype:proximate\n    , ") == string::npos);
        ARCHETYPE_TEST(ttl.find("archetype:proximate\n    obj:") != string::npos);
    }

    void TestInspectUniverse::runTests_() {
        testNullParentType_();
        testVocabSyntax_();
        testProximateSyntax_();
    }
}
