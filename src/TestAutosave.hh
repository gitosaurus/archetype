//
//  TestAutosave.hh
//  archetype
//

#ifndef __archetype__TestAutosave__
#define __archetype__TestAutosave__

#include <iostream>

#include "ITestSuite.hh"

namespace archetype {
    class TestAutosave : public ITestSuite {
        void testTargetDerivation_();
        void testAtomicWriteRoundTrips_();
        void testBackupHoldsPreviousContents_();
        void testNoBackupLeavesNoLitter_();
        void testFailedWriteLeavesTargetIntact_();
        void testArmingDoesNotInventUpdateMessage_();
        void testCheckpointCapturesTurnState_();
    protected:
        virtual void runTests_() override;
    public:
        TestAutosave(std::string name): ITestSuite(name) { }
    };
}

#endif /* defined(__archetype__TestAutosave__) */
