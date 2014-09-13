//
//  TestIdIndex.cpp
//  archetype
//
//  Created by Derek Jones on 2/11/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <iostream>
#include <string>

#include "TestIdIndex.h"
#include "TestRegistry.h"
#include "IdIndex.h"

using namespace std;

namespace archetype {
    ARCHETYPE_TEST_REGISTER(TestIdIndex);

    void TestIdIndex::runTests_() {
        IdIndex<string> strindex;
        ARCHETYPE_TEST_EQUAL(strindex.has("First"), false);

        strindex.index("First");
        ARCHETYPE_TEST_EQUAL(strindex.has("First"), true);
        ARCHETYPE_TEST_EQUAL(strindex.index("Second"), 1);
        ARCHETYPE_TEST_EQUAL(strindex.index("First"), 0);

        ARCHETYPE_TEST_EQUAL(strindex.get(1), string("Second"));
    }

}
