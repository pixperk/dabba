#pragma once

#include<cstddef>//for std::size_t
#include <memory> // for std::unique_ptr

//we want to tie our stack to the scope of child

class ChildStack{
    static constexpr std::size_t kStackSize = 1024 * 1024;
    //this unique_ptr will manage the memory for the stack, 
    //ensuring it is properly deallocated when the ChildStack object goes out of scope.
    // (internally it will call delete[] on the allocated memory when the unique_ptr is destroyed)
    //this is RAII (Resource Acquisition Is Initialization) principle in C++ which helps manage resources automatically.
    std::unique_ptr<char[]> mem_;

public:
        ChildStack() : mem_(std::make_unique<char[]>(kStackSize)) {}

        char *top() const {
            //return the top of the stack, which is the end of the allocated memory
            return mem_.get() + kStackSize;
        }

};