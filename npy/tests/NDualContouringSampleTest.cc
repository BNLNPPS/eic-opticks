#include <iostream>
#include <limits>
#include <algorithm>
#include <functional>


#include "NGLM.hpp"
#include "NPY.hpp"
#include "GLMFormat.hpp"

#include "NDualContouringSample.hpp"
#include "NTrianglesNPY.hpp"
#include "NSphere.hpp"
#include "NBox.hpp"

#include "OPTICKS_LOG.hh"


std::function<float(float,float,float)> Density_Func = NULL ;


void test_f_fff( std::function<float(float, float, float)> ff )
{
    LOG(info) << " test_f_fff(0,0,0) " << ff(0,0,0) ; 
}

void test_f_vec( std::function<float(const glm::vec3&)> f_vec, const glm::vec3& v)
{
    LOG(info) << " test_f_vec(v) " << f_vec(v) ; 
}


int main(int argc, char** argv)
{
    OPTICKS_LOG(argc, argv);

    nsphere* sph = make_sphere(0,0,0, 10) ;
    //nbox* box = make_box(5,5,5, 10) ;

    NDualContouringSample dcs ;
    NTrianglesNPY* tris = dcs(sph) ;
    LOG(info) << " num tris " << tris->getNumTriangles(); 

    return 0 ; 
}
