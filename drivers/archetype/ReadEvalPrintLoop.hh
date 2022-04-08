//
//  ReadEvalPrintLoop.h
//  archetype
//
//  Created by Derek Jones on 9/3/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#ifndef __archetype__ReadEvalPrintLoop__
#define __archetype__ReadEvalPrintLoop__

#include <iostream>
#include <stdexcept>

#include "Universe.hh"

namespace archetype {
    // Enter the read-eval-print loop.  Return error status:  zero for no
    // errors, nonzero for a problem.
    int repl();
}

#endif /* defined(__archetype__ReadEvalPrintLoop__) */
