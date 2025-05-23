#include <iostream>
#include <iomanip>

#define GLM_ENABLE_EXPERIMENTAL

#include "SGLFW.h"

void test_SGLFW_GLenum(const char* name)
{
    GLenum type = SGLFW_GLenum::Type(name); 
    const char* name2 = SGLFW_GLenum::Name(type); 
    std::cout 
        << " name " << std::setw(20) << name 
        << " type " << std::setw(5) << type 
        << " name2 " << std::setw(20) << name2
        << std::endl 
        ; 
    assert( strcmp(name, name2) == 0 ); 
}

void test_SGLFW_GLenum()
{
    test_SGLFW_GLenum("GL_BYTE"); 
    test_SGLFW_GLenum("GL_UNSIGNED_BYTE"); 
    test_SGLFW_GLenum("GL_SHORT"); 
    test_SGLFW_GLenum("GL_UNSIGNED_SHORT"); 
    test_SGLFW_GLenum("GL_INT"); 
    test_SGLFW_GLenum("GL_UNSIGNED_INT"); 
    test_SGLFW_GLenum("GL_HALF_FLOAT"); 
    test_SGLFW_GLenum("GL_FLOAT"); 
    test_SGLFW_GLenum("GL_DOUBLE"); 
}


int main()
{
    test_SGLFW_GLenum(); 
    return 0 ; 
}
