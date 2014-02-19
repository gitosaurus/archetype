//
//  TokenStream.h
//  archetype
//
//  Created by Derek Jones on 2/16/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#ifndef __archetype__TokenStream__
#define __archetype__TokenStream__

#include <iostream>

#include "Token.h"
#include "SourceFile.h"

namespace archetype {
    class TokenStream {
        SourceFile& source_;
        Token token_;
        bool newlines_;
        bool consumed_;
    public:
        TokenStream(SourceFile& source);
        bool fetch();
        void didNotConsume();
    };
}

#endif /* defined(__archetype__TokenStream__) */
