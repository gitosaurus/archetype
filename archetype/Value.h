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
#include <stdexcept>

#include "Keywords.h"

namespace archetype {

    class IValue;
    typedef std::unique_ptr<IValue> Value;
    
    class IValue {
    public:
        IValue() { }
        IValue(const IValue&) = delete;
        IValue& operator=(const IValue&) = delete;
    
        virtual bool isUndefined() const      { return false; }
        virtual std::string getString() const { throw std::logic_error("Value is not a string"); }
        virtual int getNumber() const         { throw std::logic_error("Value is not a number"); }
        
        virtual Value stringConversion() const;
        virtual Value numericConversion() const;
    };
    
    class UndefinedValue : public IValue {
    public:
        UndefinedValue() { }
        virtual bool isUndefined() const override { return true; }
    };
    
    class MessageValue : public IValue {
        int message_;
    public:
        MessageValue(int message): message_(message) { }
        virtual Value stringConversion() const override;
    };
    
    class NumericValue : public IValue {
        int value_;
    public:
        NumericValue(int value): value_(value) { }
        
        virtual int getNumber() const override;
        
        virtual Value stringConversion() const override;
        virtual Value numericConversion() const override;
    };
    
    class ReservedConstantValue : public IValue {
        Keywords::Reserved_e word_;
    public:
        ReservedConstantValue(Keywords::Reserved_e word): word_(word) { }
        
        virtual Value stringConversion() const override;
    };
    
    class StringValue : public IValue {
        std::string value_;
    public:
        StringValue(std::string value): value_(value) { }
        
        virtual std::string getString() const override;
        
        virtual Value stringConversion() const override;
        virtual Value numericConversion() const override;
    };
    
    class IdentifierValue : public IValue {
        int id_;
    public:
        IdentifierValue(int id): id_(id) { }
        
        virtual Value stringConversion() const override;
    };

}

#endif /* defined(__archetype__Value__) */
