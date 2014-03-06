//
//  Value.h
//  archetype
//
//  Created by Derek Jones on 3/5/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#ifndef __archetype__Value__
#define __archetype__Value__

#include <iostream>
#include <string>
#include <memory>

#include "Keywords.h"

namespace archetype {
    
    class IValue {
    public:
        IValue() { }
        IValue(const IValue&) = delete;
        IValue& operator=(const IValue&) = delete;
        virtual std::string toString() const = 0;
    };
    
    typedef std::unique_ptr<IValue> Value;
    
    class MessageValue : public IValue {
        int message_;
    public:
        MessageValue(int message): message_(message) { }
        virtual std::string toString() const;
    };
    
    class NumericValue : public IValue {
        int value_;
    public:
        NumericValue(int value): value_(value) { }
        virtual std::string toString() const;
    };
    
    class ReservedConstantValue : public IValue {
        Keywords::Reserved_e word_;
    public:
        ReservedConstantValue(Keywords::Reserved_e word): word_(word) { }
        virtual std::string toString() const;
    };
    
    class StringValue : public IValue {
        std::string value_;
    public:
        StringValue(std::string value): value_(value) { }
        virtual std::string toString() const;
    };
    
    class IdentifierValue : public IValue {
        int id_;
    public:
        IdentifierValue(int id): id_(id) { }
        virtual std::string toString() const;
    };

}

#endif /* defined(__archetype__Value__) */
