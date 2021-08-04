//
//  WrappedOutput.hh
//  archetype
//
//  Created by Derek Jones on 9/20/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#ifndef __archetype__WrappedOutput__
#define __archetype__WrappedOutput__

#include "UserOutput.hh"

namespace archetype {
    class WrappedOutput : public IUserOutput {
        int maxColumns_;
        int cursor_;
    protected:
        UserOutput output_;
    public:
        WrappedOutput(UserOutput output, int max_columns = 80);
        virtual ~WrappedOutput();
        virtual void put(const std::string& line) override;
        virtual void endLine() override;
        virtual void banner(char ch) override;

        // Get and set the columns of text available in the console.
        // Zero is a special value meaning that the columns are
        // indeterminate, so never wrap lines.
        int maxColumns() const { return maxColumns_; }
        void setMaxColumns(int max_columns);
        
        // Set cursor back to the left margin.
        void resetCursor();
    };
}

#endif /* defined(__archetype__WrappedOutput__) */
