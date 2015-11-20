#pragma once

#include <glm/glm.hpp>

struct nplane ; 
struct ndisc ; 
struct npart ;

struct nsphere {
    nsphere(float x, float y, float z, float w);
    nsphere(const glm::vec4& param_);

    static ndisc intersect(nsphere& a, nsphere& b);

    npart part();
    npart zrhs(float z); // +z to the right  
    npart zlhs(float z);  

    void dump(const char* msg);

    glm::vec4 param ; 
};


inline nsphere::nsphere(float x, float y, float z, float w)
{
    param.x = x  ;
    param.y = y  ;
    param.z = z  ;
    param.w = w  ;
}

inline nsphere::nsphere(const glm::vec4& param_)
{
    param = param_ ;
}


