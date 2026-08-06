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
#include <list>
#include <string_view>

#include "Token.hh"
#include "SourceFile.hh"

namespace archetype {
    class TokenStream {
        SourceFilePtr source_;
        Token token_;
        std::list<bool> newlineIsToken_;
        bool consumed_;
        bool keepLooking_;
    public:
        TokenStream(SourceFilePtr source);
        [[nodiscard]] bool fetch();

        bool isNewlineSignificant() const { return newlineIsToken_.front(); }
        void considerNewline() { newlineIsToken_.push_front(true); }
        void restoreNewlineSignificance() { newlineIsToken_.pop_front(); }

        Token token() const { return token_; }
        void didNotConsume();
        void expectGeneral(std::string_view expected);
        void expected(Token required);
        void hitEOF(Token required);
        [[nodiscard]] bool insistOn(Token required);
        void errorMessage(std::string_view message);
        void stopLooking();
    };
}

#endif /* defined(__archetype__TokenStream__) */
