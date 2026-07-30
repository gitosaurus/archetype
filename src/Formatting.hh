//
//  Formatting.hh
//  archetype
//
//  Created by Derek Jones on 7/30/26.
//  Copyright (c) 2026 Derek Jones. All rights reserved.
//

#ifndef __archetype__Formatting__
#define __archetype__Formatting__

#include <format>
#include <ostream>
#include <sstream>
#include <string>

namespace archetype {

    // Bridges the types that already know how to write themselves to an ostream
    // into std::format, so that a diagnostic can be one format string instead of
    // a paragraph of stream insertions.  A std::formatter specialization derives
    // from this and calls render() with whatever does the streaming.
    struct StreamedFormatter {
        constexpr auto parse(std::format_parse_context& ctx) const {
            auto it = ctx.begin();
            if (it != ctx.end() and *it != '}') {
                throw std::format_error("Archetype types accept no format specifiers");
            }
            return it;
        }

        template <class Emit>
        static auto render(Emit emit, std::format_context& ctx) {
            std::ostringstream out;
            emit(out);
            return std::format_to(ctx.out(), "{}", out.str());
        }
    };

}

#endif /* defined(__archetype__Formatting__) */
