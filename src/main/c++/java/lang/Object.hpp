/*
 * Javolution - Java(TM) Solution for Real-Time and Embedded Systems
 * Copyright (C) 2012 - Javolution (http://javolution.org/)
 * All rights reserved.
 */
#pragma once

#include <iostream>
#include "Javolution.hpp"
#include "java/lang/Void.hpp"

using namespace java::lang; // Set default java::lang namespace (global setting).

/** Macro to define default constructors for objects handles. */
#define CTOR(HANDLE, VALUE) HANDLE(Void = nullptr) {} HANDLE(VALUE* value) : Object(value) {}

namespace java {
namespace lang {

class Object;
class Class;
class String;

/**
 * <p> Holds the methods to be implemented by all classes and interfaces (through virtual inheritance).</p>
 */
class Object_Interface {
public:

    /**
     * Indicates whether some other object value is "equal to" this one.
     */
    virtual bool equals(const Object& other) const;

    /**
     * Returns the hash code value for this object.
     */
    virtual int hashCode() const {
    	std::size_t address = reinterpret_cast<std::size_t>(this);
        return (int) address;
    }

    /**
     * Returns the runtime class of this object.
     */
    virtual Class getClass() const;

    /**
     * Returns the string representation of this object.
     */
    virtual String toString() const;

    /**
     * Returns the monitor associated to this object. This method should be overridden by classes supporting
     * synchronization such as Collection classes.
     *
     * @throw UnsupportedOperationException if the class does not support synchronization (default).
     */
    virtual Type::Mutex& monitor_() const;


};

/**
 *  The base class for heap allocated values handled by Object instances (direct inheritance).
 *  Heap allocations/deallocations are performed using FastHeap (if activated).
 */
class Object_Value: public virtual Object_Interface {
friend class Object;

    Type::atomic_count refCount;

    void incRefCount() {
        ++refCount;
    }

    bool decRefCount() {
        return --refCount == 0;
    }

public:

    Object_Value() {
        std::atomic_init(&refCount, 0);
    }

    /** Allocates new instances from FastHeap. */
    inline void* operator new(size_t size) {
        return Type::FastHeap::INSTANCE.allocate(size);
    }

    /** Deletes this instance back to FastHeap. */
    inline void operator delete(void* mem) {
        Type::FastHeap::INSTANCE.deallocate(mem);
    }

    /** Destructor (virtual, could be called through a pointer to base class). */
    virtual ~Object_Value() {
    }
};

// Standard Java exceptions.
class Object_Exceptions : public virtual Object_Interface {
public:
    static void throwNullPointerException();
    static void throwArrayIndexOutOfBoundsException();
    static void throwNegativeArraySizeException();
};

/**
 * The root class for all "Java-Like" objects handles (through virtual inheritance).
 *
 * <p> Note: Object and its derived classes are smart pointers (intrusive pointers), they will invoke the delete on
 *           their values automatically once their reference counts reach zero.
 *
 * @see  <a href="https://docs.oracle.com/javase/8/docs/api/java/lang/Object.html">Java - Object</a>
 * @see  <a href="http://en.wikipedia.org/wiki/Comparison_of_Java_and_C%2B%2B">Comparison of Java and C++</a>
 * @see  <a href="http://umich.edu/~eecs381/handouts/C++11_smart_ptrs.pdf>Using C++11’s Smart Pointers</a>
 * @version 7.0
 */
class Object : public virtual Object_Interface {

    Object_Value* valuePtr; // The managed object.

 public:

    /** Equivalent to java::lang::Object_Interface */
    typedef Object_Interface Interface;

    /** Equivalent to java::lang::Object_Exceptions */
    typedef Object_Exceptions Exceptions;

    /** Equivalent to java::lang::Object_Value */
    typedef Object_Value Value;

    /** Default constructor (null value). */
    Object(Void = nullptr) : valuePtr(nullptr) {
    }

    /** Constructor for the specified value. */
    Object(Value* value) : valuePtr(value) {
        if (value != nullptr)
            value->incRefCount();
    }

    /** Constructor for the specified interface.*/
    Object(Interface* interface) : valuePtr(dynamic_cast<Value*>(interface)) {
        if (valuePtr != nullptr)
            valuePtr->incRefCount();
    }

    /** Copy constructor. */
    Object(const Object& that) :
            valuePtr(that.valuePtr) {
        if (valuePtr != nullptr)
            valuePtr->incRefCount();
    }

    /** Cast this object value to the specified type; returns nullptr if the cast is invalid. */
    template<class T> T* cast_() const {
        return dynamic_cast<T*>(valuePtr);
    }

    /**
     * Returns the shared value managed by this instance.
     *
     * @throw NullPointerException if this object value is null.
     */
    template<class T> T* this_() const {
        if (valuePtr == nullptr)
            Object_Exceptions::throwNullPointerException();
        return static_cast<T*>(valuePtr);
    }

    /**
     * Cast and returns the shared value managed by this instance.
     * Unlike the cast_() methods this method raises an exception if the object value is null.
     *
     * @throw NullPointerException if this object value is null.
     */
    template<class T> T* this_cast_() const {
        if (valuePtr == nullptr)
            Object_Exceptions::throwNullPointerException();
        return  dynamic_cast<T*>(valuePtr);
    }

    bool equals(const Object& other) const override {
        if (valuePtr == nullptr)
            Object_Exceptions::throwNullPointerException();
        return valuePtr == other.valuePtr;
    }

    int hashCode() const override {
        if (valuePtr == nullptr)
            Object_Exceptions::throwNullPointerException();
        return valuePtr->hashCode();
    }

    Class getClass() const override ;

    String toString() const override ;

    Type::Mutex& monitor_() const override {
        if (valuePtr == nullptr)
            Object_Exceptions::throwNullPointerException();
        return valuePtr->monitor_();
    }

    //////////////////////////////////
    // Intrusive Pointer Management //
    //////////////////////////////////

    Object& operator=(const Object& that) {
        Object(that).swap(*this);
        return *this;
    }

    Object& operator=(Value* that) {
        Object(that).swap(*this);
        return *this;
    }

    Object& operator=(Void) {
        if (valuePtr != nullptr)
            if (valuePtr->decRefCount())
                delete valuePtr;
        valuePtr = nullptr;
        return *this;
    }

    ~Object() {
        if (valuePtr != 0)
            if (valuePtr->decRefCount())
                delete valuePtr;
    }

    /** Returns the shared value managed by this object.*/
    Value* value_() const {
        return valuePtr;
    }

    /** Replaces the value of this object without incrementing its reference count or decrementing the reference count
     *  of the previous value. This method is typically used to prevent cycles (instance referencing to itself or to
     *  one its container). */
    void value_(Value* newValue) {
        valuePtr = newValue;
    }

    ////////////////////////////////
    // C++ Pointer Type Operators //
    ////////////////////////////////

    Value& operator*() const {
        if (valuePtr == nullptr) Object_Exceptions::throwNullPointerException();
        return *valuePtr;
    }

    Value* operator->() const {
        if (valuePtr == nullptr) Object_Exceptions::throwNullPointerException();
        return valuePtr;
    }

private:

    void swap(Object& that) {
        Value* tmp = valuePtr;
        valuePtr = that.valuePtr;
        that.valuePtr = tmp;
    }

};

}
}

inline bool Object_Interface::equals(const Object& other) const {
    return this == other.value_();
}

// Pointers equalities.

inline bool operator==(const Object& obj, const void* ptr) {
    return obj.value_() == ptr;
}
inline bool operator!=(const Object& obj, const void* ptr) {
    return obj.value_() != ptr;
}
inline bool operator==(const void* ptr, const Object& obj) {
    return obj.value_() == ptr;
}
inline bool operator!=(const void* ptr, const Object& obj) {
    return obj.value_() != ptr;
}
inline bool operator==(const Object& obj1, const Object& obj2) {
    return obj1.value_() == obj2.value_();
}
inline bool operator!=(const Object& obj1, const Object& obj2) {
    return obj1.value_() != obj2.value_();
}

// Stream operation.
std::ostream& operator<<(std::ostream& os, const Object& that);

std::wostream& operator<<(std::wostream& wos, const Object& that);

