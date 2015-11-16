#include "GTestShape.hh"

#include "GCache.hh"
#include "GBBoxMesh.hh"

#include "GMesh.hh"
#include "GSolid.hh"
#include "GVector.hh"
#include "GMatrix.hh"
#include "NLog.hpp"

// npy-
#include "NPY.hpp"
#include "NSphere.hpp"
#include "NLog.hpp"



GSolid* GTestShape::make(char typecode, glm::vec4& spec )
{
    GSolid* solid = NULL ; 
    switch(typecode)
    {
        case 'B': solid = makeBox(spec)    ;break;
        case 'S': solid = makeSphere(spec) ;break;
    }
    return solid ; 
}

GSolid* GTestShape::makeBox(glm::vec4& spec)
{
    float size = spec.w ; 
    gbbox bb(gfloat3(-size), gfloat3(size));  
    return makeBox(bb);
}

GSolid* GTestShape::makeBox(gbbox& bbox)
{
    LOG(debug) << "GTestShape::makeBox" ;

    unsigned int nvert = 24 ; 
    unsigned int nface = 6*2 ; 

    gfloat3* vertices = new gfloat3[nvert] ;
    guint3* faces = new guint3[nface] ;
    gfloat3* normals = new gfloat3[nvert] ;

    GBBoxMesh::twentyfour(bbox, vertices, faces, normals );

    unsigned int meshindex = 0 ; 
    unsigned int nodeindex = 0 ; 

    GMesh* mesh = new GMesh(meshindex, vertices, nvert,  
                                       faces, nface,    
                                       normals,  
                                       NULL ); // texcoords

    mesh->setColors(  new gfloat3[nvert]);
    mesh->setColor(0.5,0.5,0.5);  


    GMatrixF* transform = new GMatrix<float>();

    GSolid* solid = new GSolid(nodeindex, transform, mesh, UINT_MAX, NULL );     

    solid->setBoundary(0);     // unlike ctor these create arrays
    solid->setSensor( NULL );      

    return solid ; 
}



GSolid* GTestShape::makeSphere(glm::vec4& spec)
{
    LOG(debug) << "GTestShape::makeSphere" ;

    unsigned int subdiv = 0 ; 
    unsigned int ntri = 20*(1 << (subdiv * 2)) ;
    NPY<float>* triangles = NSphere::icosahedron(0);  // (subdiv, ntri)  (0,20)
    assert(triangles->getNumItems() == ntri);

    float radius = spec.w ; 

    unsigned int meshindex = 0 ; 
    unsigned int nodeindex = 0 ; 

    GMesh* mesh = GMesh::make_mesh(triangles, radius, meshindex);

    GMatrixF* transform = new GMatrix<float>();

    GSolid* solid = new GSolid(nodeindex, transform, mesh, UINT_MAX, NULL );     

    solid->setBoundary(0);     // unlike ctor these create arrays
    solid->setSensor( NULL );      

    return solid ; 
}





