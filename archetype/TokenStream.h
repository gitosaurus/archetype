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
        bool keepLooking_;
    public:
        TokenStream(SourceFile& source);
        bool fetch();
        Token token() const { return token_; }
        void didNotConsume();
        void expectGeneral(std::string expected);
        void errorMessage(std::string message);
        void stopLooking();
    };
}

#endif /* defined(__archetype__TokenStream__) */
