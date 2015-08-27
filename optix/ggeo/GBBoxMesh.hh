#pragma once

#include "GMesh.hh"
#include "GMergedMesh.hh"

class GBBoxMesh : public GMesh {

public:
    //enum { NUM_VERTICES = 8, NUM_FACES = 6*2 } ;
    enum { NUM_VERTICES = 24, NUM_FACES = 6*2 } ;

public:
    static GBBoxMesh* create(GMergedMesh* mergedmesh);

private:
    void eight(); 
    void twentyfour(); 

public:
    GBBoxMesh(GMergedMesh* mm) ; 
    virtual ~GBBoxMesh(); 

private:
    GMergedMesh* m_mergedmesh ; 
     
};


inline GBBoxMesh::~GBBoxMesh()
{
}



