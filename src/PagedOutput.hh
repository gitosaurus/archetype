//
//  PagedOutput.hh
//  archetype
//
//  Created by Derek Jones on 8/1/21.
//  Copyright © 2021 Derek Jones. All rights reserved.
//

#ifndef PagedOutput_hpp
#define PagedOutput_hpp

#include "WrappedOutput.hh"

namespace archetype {
    class PagedOutput : public WrappedOutput {
        int maxRows_;
        int rows_;
        void wrapWait_();
    public:
        PagedOutput(UserOutput output, int max_columns = 80, int max_rows = 24);
        virtual ~PagedOutput();
        virtual void endLine() override;
        virtual void resetPager() override;

        // Get and set the rows of text available in the console.
        // Zero is a special value meaning that the rows are
        // indeterminate, so never page the output.
        int maxRows() const { return maxRows_; }
        void setMaxRows(int rows) { maxRows_ = rows; }
    };
}

#endif /* PagedOutput_hh */
