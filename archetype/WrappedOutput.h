//
//  WrappedOutput.h
//  archetype
//
//  Created by Derek Jones on 9/20/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#ifndef __archetype__WrappedOutput__
#define __archetype__WrappedOutput__

#include "UserOutput.h"

namespace archetype {
    class WrappedOutput : public IUserOutput {
        UserOutput output_;
        int maxRows_;
        int maxColumns_;
        int rows_;
        int cursor_;
        void wrapWait_();
    public:
        WrappedOutput(UserOutput output);
        virtual ~WrappedOutput();
        virtual void put(const std::string& line) override;
        virtual void endLine() override;
        virtual void resetPager() override;

        // Get and set the rows of text available in the console.
        // Zero is a special value meaning that the rows are
        // indeterminate, so never page the output.
        int maxRows() const { return maxRows_; }
        void setMaxRows(int rows) { maxRows_ = rows; }

        // Get and set the columns of text available in the console.
        // Zero is a special value meaning that the columns are
        // indeterminate, so never wrap lines.
        int maxColumns() const { return maxColumns_; }
        void setMaxColumns(int columns) { maxColumns_ = columns; }
    };
}

#endif /* defined(__archetype__WrappedOutput__) */
