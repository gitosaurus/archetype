//
//  TestSystemSorter.hh
//  archetype
//
//  Created by Derek Jones on 7/26/21.
//  Copyright © 2021 Derek Jones. All rights reserved.
//

#ifndef TestSystemSorter_hh
#define TestSystemSorter_hh

#include "ITestSuite.hh"

namespace archetype {
    class TestSystemSorter : public ITestSuite {
        void testSortAndSerialize_();
    protected:
        virtual void runTests_() override;
    public:
        TestSystemSorter(std::string name): ITestSuite(name) { }
    };
}

#endif /* TestSystemSorter_hh */
