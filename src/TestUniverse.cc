//
//  TestUniverse.cc
//  archetype
//
//  Created by Derek Jones on 3/22/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <format>
#include <string>
#include <string_view>
#include <sstream>
#include <memory>
#include <vector>
#include <filesystem>
#include <fstream>
#include <system_error>

#include "TestUniverse.hh"
#include "TestRegistry.hh"
#include "Universe.hh"
#include "SourceFile.hh"
#include "TokenStream.hh"
#include "Wellspring.hh"
#include "Capture.hh"

using namespace std;

namespace archetype {
    ARCHETYPE_TEST_REGISTER(TestUniverse);

    static char program_basic_1[] =
    "null vase\n"
    "  desc: \"vase\"\n"
    "  weight: 1\n"
    "methods\n"
    "  'heft': write \"The \", desc, \" has a weight of \", weight, \".\"\n"
    "end\n"
    ;

    static char program_basic_2[] =
    "type edible based on null\n"
    "  desc: \"snack\"\n"
    "methods\n"
    "  'eat': write \"You gobble down the \", desc, \".\"\n"
    "end\n"
    "edible cracker desc : \"cracker\" flavor : \"salty\" end\n"
    "edible granola_bar end\n"
    ;

    void TestUniverse::testBasicObjects_() {
        Universe::destroy();

        TokenStream t1(make_source_from_str("program_basic_1", program_basic_1));
        ARCHETYPE_TEST(Universe::instance().make(t1));
        Capture capture1;
        Statement stmt1 = make_stmt_from_str("'heft' -> vase");
        stmt1->execute();
        string actual1 = capture1.getCapture();
        string expected1 = "The vase has a weight of 1.\n";
        ARCHETYPE_TEST_EQUAL(actual1, expected1);

        TokenStream t2(make_source_from_str("program_basic_2", program_basic_2));
        ARCHETYPE_TEST(Universe::instance().make(t2));
        Capture capture2;
        Statement stmt2 = make_stmt_from_str("'eat' -> cracker");
        stmt2->execute();
        string actual2 = capture2.getCapture();
        string expected2 = "You gobble down the cracker.\n";
        ARCHETYPE_TEST_EQUAL(actual2, expected2);
        Capture capture3;
        Statement stmt3 = make_stmt_from_str("'eat' -> granola_bar");
        stmt3->execute();
        string actual3 = capture3.getCapture();
        string expected3 = "You gobble down the snack.\n";
        ARCHETYPE_TEST_EQUAL(actual3, expected3);
    }

    static char program_nullity[] =
    "type thing based on null methods 'look' : write(\"some thing\") end\n"
    "thing null methods 'get' : write(\"You cannot.\") end\n"
    "null slop methods 'throw' : write(\"Splat!\") end\n"
    ;

    void TestUniverse::testNullIsNull_() {
        Universe::destroy();

        TokenStream t1(make_source_from_str("program_nullity", program_nullity));
        ARCHETYPE_TEST(Universe::instance().make(t1));
    }

    static char program_dynamic[] =
    "type blinds based on null\n"
    "  shade : UNDEFINED\n"
    "methods\n"
    "  'look' : write \"A crummy shade of \", shade, \".\"\n"
    "end\n"
    "\n"
    "null customer left : UNDEFINED right : UNDEFINED other_ref : UNDEFINED methods\n"
    "  'buy' : { create blinds named left; left.shade := \"white\"\n"
    "            create blinds named right; right.shade := \"alabaster\"; other_ref := right }\n"
    "end\n"
    ;

    void TestUniverse::testDynamicObjects_() {
        Universe::destroy();

        ObjectPtr obj1 = Universe::instance().defineNewObject();
        // 0 = null, 1 = system, so 2 is the first possible
        ARCHETYPE_TEST_EQUAL(obj1->id(), 2);

        ObjectPtr obj2 = Universe::instance().defineNewObject();
        ARCHETYPE_TEST_EQUAL(obj2->id(), 3);

        Universe::instance().destroyObject(2);

        ObjectPtr obj3 = Universe::instance().defineNewObject();
        ARCHETYPE_TEST_EQUAL(obj1->id(), Object::INVALID);

        // The destroyed object's id is retired, not handed on:  whatever still
        // refers to 2 must go on finding nothing there.
        ARCHETYPE_TEST_EQUAL(obj3->id(), 4);
        ARCHETYPE_TEST(not Universe::instance().getObject(2));

        // Destroying it again is harmless, and so is destroying what never
        // existed.  The reserved objects are not to be had at all.
        Universe::instance().destroyObject(2);
        Universe::instance().destroyObject(4000);
        Universe::instance().destroyObject(-1);
        Universe::instance().destroyObject(Universe::NullObjectId);
        Universe::instance().destroyObject(Universe::SystemObjectId);
        ARCHETYPE_TEST(Universe::instance().getObject(Universe::NullObjectId) != nullptr);
        ARCHETYPE_TEST_EQUAL(Universe::instance().getObject(Universe::SystemObjectId)->id(),
                             Universe::SystemObjectId);
        ARCHETYPE_TEST_EQUAL(obj3->id(), 4);

        TokenStream t(make_source_from_str("dynamic.arch", program_dynamic));
        ARCHETYPE_TEST(Universe::instance().make(t));
        Capture capture;
        Statement stmt = make_stmt_from_str("{'buy' -> customer; 'look' -> customer.left; 'look' -> customer.right}");
        stmt->execute();
        string actual = capture.getCapture();
        ARCHETYPE_TEST_EQUAL(actual, string("A crummy shade of white.\nA crummy shade of alabaster.\n"));

        // Cause some compilation errors
        Statement no_type_stmt = make_stmt_from_str("create nonsense named customer.left");
        ARCHETYPE_TEST(no_type_stmt == nullptr);

        Statement type_not_instance = make_stmt_from_str("create customer named customer.left");
        ARCHETYPE_TEST(type_not_instance == nullptr);

        // Destroy an object and verify that all references to and through it become undefined
        Statement ref1 = make_stmt_from_str("customer.right.shade");
        Statement ref2 = make_stmt_from_str("customer.other_ref.shade");
        // First, check the references before destruction
        Value val1 = ref1->execute()->stringConversion();
        Value val2 = ref2->execute()->stringConversion();
        ARCHETYPE_TEST(val1->isDefined());
        ARCHETYPE_TEST(val2->isDefined());
        ARCHETYPE_TEST(val1->getString() == val2->getString());
        Statement destroy_stmt = make_stmt_from_str("destroy customer.right");
        destroy_stmt->execute();
        val1 = ref1->execute()->stringConversion();
        val2 = ref2->execute()->stringConversion();
        ARCHETYPE_TEST(not val1->isDefined());
        ARCHETYPE_TEST(not val2->isDefined());

        // The same again, with the same stale reference:  customer.right still
        // names the object that is gone.  This used to take the interpreter
        // down, one way or another depending on where the id had sat.
        destroy_stmt->execute();
        destroy_stmt->execute();

        // And now the point of retiring ids.  A new object must not appear
        // behind either of the references that outlived the old one, which is
        // what happened when it was given the old one's id.
        Statement buy_again = make_stmt_from_str("create blinds named customer.left");
        int newcomer_id = buy_again->execute()->objectConversion()->getObject();
        Statement stale = make_stmt_from_str("customer.right");
        int stale_id = stale->execute()->objectConversion()->getObject();
        ARCHETYPE_TEST(newcomer_id != stale_id);
        ARCHETYPE_TEST(not Universe::instance().getObject(stale_id));
        ARCHETYPE_TEST(not ref1->execute()->stringConversion()->isDefined());
        ARCHETYPE_TEST(not ref2->execute()->stringConversion()->isDefined());
        Value same = make_stmt_from_str("customer.right = customer.left")->execute();
        ARCHETYPE_TEST(not same->isTrueEnough());
    }

    static char program_inclusion[] =
    "include \"snack.arch\"\n"
    "null flavor desc : \"Confusing object with same name as an attribute\" end\n"
    "null conservatory\n"
    "  desc: \"A nicely appointed room overlooking the garden.\"\n"
    "  contents: cracker\n"
    "methods\n"
    "  'look' : write \"A \", contents.flavor, \" \", contents.desc, \" is here.\"\n"
    "end\n"
    ;

    void TestUniverse::testInclusion_() {
        Universe::destroy();

        Wellspring::instance().put("snack.arch", make_source_from_str("snack.arch", program_basic_2));
        TokenStream t3(make_source_from_str("program_inclusion", program_inclusion));
        ARCHETYPE_TEST(Universe::instance().make(t3));
        Capture capture;
        Statement stmt = make_stmt_from_str("'look' -> conservatory");
        stmt->execute();
        string actual = capture.getCapture();
        string expected = "A salty cracker is here.\n";
        ARCHETYPE_TEST_EQUAL(actual, expected);
    }

    static char program_double_inclusion[] =
    "include \"snack\"\n"
    "include \"snack\"\n"
    "null larder\n"
    "  desc: \"A cold larder off the kitchen.\"\n"
    "  contents: cracker\n"
    "methods\n"
    "  'look' : write \"A \", contents.flavor, \" \", contents.desc, \" is here.\"\n"
    "end\n"
    ;

    // A file named twice is read once.
    //
    // This has to go through a file on disk.  The inclusion test above hands
    // its source to the Wellspring with put(), which lands it in sources_
    // under the very name the include asks for -- so it is found by lookup and
    // never by search.  Searching is the path every real include takes, and it
    // was the path that forgot to record what it had opened.
    void TestUniverse::testInclusionHappensOnce_() {
        Universe::destroy();
        Wellspring::destroy();

        filesystem::path dir = filesystem::temp_directory_path() / "archetype_include_once";
        error_code ec;
        filesystem::remove_all(dir, ec);
        filesystem::create_directories(dir, ec);
        {
            ofstream snack{dir / "snack.arch"};
            snack << program_basic_2;
        }
        Wellspring::instance().addSearchPath(dir.string());

        ARCHETYPE_TEST(Wellspring::instance().hasNeverBeenOpened("snack"));
        TokenStream t(make_source_from_str("program_double_inclusion", program_double_inclusion));
        ARCHETYPE_TEST(Universe::instance().make(t));
        ARCHETYPE_TEST(not Wellspring::instance().hasNeverBeenOpened("snack"));

        // The second mention was skipped rather than re-read, and what the
        // first one brought in is still intact.
        Capture capture;
        Statement stmt = make_stmt_from_str("'look' -> larder");
        stmt->execute();
        string actual = capture.getCapture();
        string expected = "A salty cracker is here.\n";
        ARCHETYPE_TEST_EQUAL(actual, expected);

        filesystem::remove_all(dir, ec);
    }

    static char program_default_methods[] =
    "type something based on null\n"
    "methods\n"
    "  'foo' : \"bar\"\n"
    "   default : \"nothing from something\"\n"
    "end\n"
    "\n"
    "something thing\n"
    "methods\n"
    "  'fizzle' : \"bazzle\"\n"
    "  default: \"something has everything\"\n"
    "end\n"
    "\n"
    "something nothing methods 'sizzle' : \"gristle\" end\n"
    ;

    void TestUniverse::testDefaultMethods_() {
        Universe::destroy();

        TokenStream t4(make_source_from_str("program_default_methods", program_default_methods));
        ARCHETYPE_TEST(Universe::instance().make(t4));

        list<pair<string, string>> test_pairs = {
            { "'foo' -> thing", "bar" },
            { "'fee' -> nothing", "nothing from something" },
            { "'fizzle' -> nothing", "nothing from something" },
            { "'sizzle' -> nothing", "gristle" },
            { "'fee' -> thing", "something has everything" },
            { "'fizzle' -> thing", "bazzle" },
        };
        for (auto const& t : test_pairs) {
            cout << "TESTING: {" << t.first << "}" << endl;
            Statement stmt = make_stmt_from_str(t.first);
            Value actual = stmt->execute()->stringConversion();
            ARCHETYPE_TEST(actual->isDefined());
            ARCHETYPE_TEST_EQUAL(actual->getString(), t.second);
        }
    }

    // 'LOAD STATE' answers FALSE to a file it cannot load, and the game that
    // asked is still the game it was -- including when the file is a real save
    // cut short, which is not found out until the universe is half replaced.
    void TestUniverse::testFailedLoadLeavesUniverseAlone_() {
        Universe::destroy();
        TokenStream t(make_source_from_str("load_state",
            "null shelf x : 7 methods 'look' : write \"x is \", x end\n"));
        ARCHETYPE_TEST(Universe::instance().make(t));

        filesystem::path dir = filesystem::temp_directory_path() / "archetype_failed_load";
        error_code ec;
        filesystem::remove_all(dir, ec);
        filesystem::create_directories(dir, ec);
        auto send = [](string_view what, const filesystem::path& file) {
            Statement stmt = make_stmt_from_str(format("['{}' \"{}\"] -> system", what, file.string()));
            return stmt->execute()->stringConversion()->getString();
        };

        filesystem::path good = dir / "good.acx";
        ARCHETYPE_TEST_EQUAL(send("SAVE STATE", good), string{"TRUE"});
        auto size = filesystem::file_size(good);

        filesystem::path text = dir / "notes.txt";
        { ofstream out{text}; out << "This is not a saved game.\n"; }
        filesystem::path cut_short = dir / "cut_short.acx";
        filesystem::copy_file(good, cut_short);
        filesystem::resize_file(cut_short, size - 4);
        filesystem::path cut_in_half = dir / "cut_in_half.acx";
        filesystem::copy_file(good, cut_in_half);
        filesystem::resize_file(cut_in_half, size / 2);

        // Move on from what was saved, so that "left alone" and "reloaded"
        // are different answers.
        make_stmt_from_str("shelf.x := 8")->execute();

        for (auto const& bad : {text, cut_short, cut_in_half, dir / "no_such_file.acx"}) {
            ARCHETYPE_TEST_EQUAL(send("LOAD STATE", bad), string{"FALSE"});
            Capture capture;
            make_stmt_from_str("'look' -> shelf")->execute();
            ARCHETYPE_TEST_EQUAL(capture.getCapture(), string{"x is 8\n"});
        }

        // And a real save still loads.
        ARCHETYPE_TEST_EQUAL(send("LOAD STATE", good), string{"TRUE"});
        {
            Capture capture;
            make_stmt_from_str("'look' -> shelf")->execute();
            ARCHETYPE_TEST_EQUAL(capture.getCapture(), string{"x is 7\n"});
        }
        filesystem::remove_all(dir, ec);
    }

    void TestUniverse::testSelfBasedType_() {
        // A type cannot be its own parent, directly or by way of an instance.
        for (string program : {
            "type ouroboros based on ouroboros end\n",
            "type ouroboros based on nowhere end\n",
            "type sound based on null end\nsound thing end\ntype odd based on thing end\n",
        }) {
            Universe::destroy();
            TokenStream t(make_source_from_str("self_based", program));
            ARCHETYPE_TEST(not Universe::instance().make(t));
        }
        // What is still allowed still works, and its ancestry comes to an end.
        Universe::destroy();
        TokenStream t(make_source_from_str("well_based",
            "type sound based on null x : 1 end\n"
            "type sounder based on sound end\n"
            "sounder thing end\n"));
        ARCHETYPE_TEST(Universe::instance().make(t));
        Value x = make_stmt_from_str("thing.x")->execute()->numericConversion();
        ARCHETYPE_TEST(x->isDefined());
        ARCHETYPE_TEST_EQUAL(x->getNumber(), 1);
        Value y = make_stmt_from_str("thing.y")->execute()->valueConversion();
        ARCHETYPE_TEST(not y->isDefined());
    }

    static char program_messaging[] =
    "type echo_type based on null\n"
    "  desc: \"Echo Type!\"\n"
    "methods\n"
    "  'hiya' : 'hello' --> echo\n"
    "  default: message\n"
    "end\n"
    "\n"
    "null echo\n"
    "methods\n"
    "  'hello': \"Hello, \" & sender.desc\n"
    "  'goodbye' : \"Goodbye from \" & desc\n"
    "  default: message\n"
    "end\n"
    "\n"
    "echo_type me_too\n"
    "  desc: \"me too\"\n"
    "methods\n"
    "  'greet' : 'hello' -> echo\n"
    "end\n"
    "\n"
    "null another\n"
    "  desc: \"another\"\n"
    "methods\n"
    "  'chirp' : 'hiya' -> me_too\n"
    "  'cheep' : 'chirp' -> self\n"
    "  'goodbye' : message --> echo\n"
    "end\n"
    ;

    void TestUniverse::testMessagingKeywords_() {
        Universe::destroy();

        TokenStream t5(make_source_from_str("program_messaging", program_messaging));
        ARCHETYPE_TEST(Universe::instance().make(t5));

        Statement stmt1 = make_stmt_from_str("'some message' -> echo");
        Value actual1 = stmt1->execute()->stringConversion();
        ARCHETYPE_TEST(actual1->isDefined());
        string expected1 = "some message";
        ARCHETYPE_TEST_EQUAL(actual1->getString(), expected1);

        Statement stmt2 = make_stmt_from_str("\"Random string\" -> echo");
        Value actual2 = stmt2->execute()->stringConversion();
        ARCHETYPE_TEST(actual2->isDefined());
        ARCHETYPE_TEST_EQUAL(actual2->getString(), string("Random string"));

        Statement stmt3 = make_stmt_from_str("42 -> echo");
        Value actual3 = stmt3->execute()->numericConversion();
        ARCHETYPE_TEST(actual3->isDefined());
        ARCHETYPE_TEST_EQUAL(actual3->getNumber(), 42);

        Statement stmt4 = make_stmt_from_str("42 -> me_too");
        Value actual4 = stmt4->execute()->numericConversion();
        ARCHETYPE_TEST(actual4->isDefined());
        ARCHETYPE_TEST_EQUAL(actual4->getNumber(), 42);

        Statement stmt5 = make_stmt_from_str("'yelp' -> another");
        Value actual5 = stmt5->execute();
        ARCHETYPE_TEST(actual5->isDefined());
        Value expected5 = make_unique<AbsentValue>();
        ARCHETYPE_TEST(actual5->isSameValueAs(expected5));

        Statement stmt6 = make_stmt_from_str("'con' & 'structed' -> another");
        Value actual6 = stmt6->execute();
        ARCHETYPE_TEST(actual6->isDefined());
        Value expected6 = make_unique<AbsentValue>();
        ARCHETYPE_TEST(actual6->isSameValueAs(expected6));

        list<pair<string, string>> test_pairs = {
            { "'greet' -> me_too", "Hello, me too" },
            { "'chirp' -> another", "Hello, another" },
            { "'cheep' -> another", "Hello, another" },
            { "'goodbye' -> another", "Goodbye from another" },
        };
        for (auto const& t : test_pairs) {
            cout << "TESTING: {" << t.first << "}" << endl;
            Statement stmt = make_stmt_from_str(t.first);
            Value actual = stmt->execute()->stringConversion();
            ARCHETYPE_TEST(actual->isDefined());
            ARCHETYPE_TEST_EQUAL(actual->getString(), t.second);
        }
    }

    static char program_serialization[] =
    "type stuff based on null\n"
    "  desc : \"nothing special\"\n"
    "methods\n"
    "  'inspect' : write desc, \" is on the coffee table\"\n"
    "end\n"
    "\n"
    "null coffee_table\n"
    "  desc : \"coffee table\"\n"
    "  views : 0\n"
    "methods \n"
    "  'desc' : write desc\n"
    "  'look' : { views +:= 1; 'check' }\n"
    "  'check' : { writes \"You have looked \"\n"
    "              if views = 1 then write \"once.\" else write views, \" times.\" }\n"
    "end\n"
    ;

    void TestUniverse::testSerialization_() {
        Universe::destroy();

        // Test the ability to create a program, modify it a little, save it, get it back
        TokenStream t1(make_source_from_str("serialization", program_serialization));
        ARCHETYPE_TEST(Universe::instance().make(t1));
        Statement look_stmt = make_stmt_from_str("'look' -> coffee_table");
        Capture look1;
        look_stmt->execute();
        ARCHETYPE_TEST_EQUAL(look1.getCapture(), string("You have looked once.\n"));

        // Put some stuff on the coffee table
        vector<int> objects;
        for (auto s : {"cup", "magazine", "remote", "saltshaker" }) {
            Statement create_stmt = make_stmt_from_str("create stuff named coffee_table." + string(s));
            Statement rename_stmt = make_stmt_from_str(string(s) + ".desc := \"" + string(s) + "\"");
            int obj_id = create_stmt->execute()->objectConversion()->getObject();
            objects.push_back(obj_id);
            rename_stmt->execute();
        }

        // Now create some holes by deleting one of the middle ones
        Statement holes_stmt = make_stmt_from_str("{destroy coffee_table.magazine; destroy coffee_table.remote}");
        holes_stmt->execute();

        MemoryStorage mem;
        mem << Universe::instance();

        look_stmt->execute(); // look 2
        look_stmt->execute(); // look 3
        look_stmt->execute(); // look 4
        Capture look5;
        look_stmt->execute();
        ARCHETYPE_TEST_EQUAL(look5.getCapture(), string("You have looked 5 times.\n"));

        // Add a new dynamic object.  It must not take the place of the
        // magazine or the remote:  their ids are retired, so it goes after
        // the saltshaker.
        Statement create_another = make_stmt_from_str("create stuff named coffee_table.napkin");
        int napkin_id = create_another->execute()->objectConversion()->getObject();
        ARCHETYPE_TEST_EQUAL(napkin_id, objects.at(3) + 1);

        // Restore old state!
        mem >> Universe::instance();
        Capture after_undo;
        look_stmt->execute();
        ARCHETYPE_TEST_EQUAL(after_undo.getCapture(), string("You have looked 2 times.\n"));

        // Verify that the cup and saltshaker are still at their old addresses
        Statement find_cup = make_stmt_from_str("coffee_table.cup");
        int cup_id = find_cup->execute()->objectConversion()->getObject();
        ARCHETYPE_TEST_EQUAL(cup_id, objects.at(0));

        Statement find_saltshaker = make_stmt_from_str("coffee_table.saltshaker");
        int saltshaker_id = find_saltshaker->execute()->objectConversion()->getObject();
        ARCHETYPE_TEST_EQUAL(saltshaker_id, objects.at(3));

        // And be sure the others are unusable.  They won't evaluate to UNDEFINED,
        // only because Archetype doesn't force an early lookup in object conversion,
        // but the resulting object should be null in the Universe.
        Statement find_magazine = make_stmt_from_str("coffee_table.magazine");
        int magazine_id = find_magazine->execute()->objectConversion()->getObject();
        ObjectPtr magazine = Universe::instance().getObject(magazine_id);
        ARCHETYPE_TEST(not magazine);

        Statement find_remote = make_stmt_from_str("coffee_table.remote");
        int remote_id = find_remote->execute()->objectConversion()->getObject();
        ObjectPtr remote = Universe::instance().getObject(remote_id);
        ARCHETYPE_TEST(not remote);

        // A restored universe must hand out ids the way the saved one did.  The
        // napkin above went in after the saltshaker; a coaster created after
        // the undo has to land on that same id, and above all must not land on
        // the magazine's or the remote's, which the restored attributes still
        // hold.
        Statement create_after_undo =
            make_stmt_from_str("create stuff named coffee_table.coaster");
        int coaster_id = create_after_undo->execute()->objectConversion()->getObject();
        ARCHETYPE_TEST_EQUAL(coaster_id, napkin_id);
        ARCHETYPE_TEST(not Universe::instance().getObject(magazine_id));
        ARCHETYPE_TEST(not Universe::instance().getObject(remote_id));

        // Destroy the last object there is, and save.  Its id has been retired
        // off the end, where no record in the file will mention it, and a load
        // that lost track would hand it straight back out.
        make_stmt_from_str("destroy coffee_table.coaster")->execute();
        MemoryStorage after_last;
        after_last << Universe::instance();
        after_last >> Universe::instance();
        Statement create_doily = make_stmt_from_str("create stuff named coffee_table.doily");
        int doily_id = create_doily->execute()->objectConversion()->getObject();
        ARCHETYPE_TEST_EQUAL(doily_id, coaster_id + 1);
    }

    void TestUniverse::runTests_() {
        testBasicObjects_();
        testNullIsNull_();
        testDynamicObjects_();
        testInclusion_();
        testInclusionHappensOnce_();
        testDefaultMethods_();
        testSelfBasedType_();
        testFailedLoadLeavesUniverseAlone_();
        testMessagingKeywords_();
        testSerialization_();
    }
}
