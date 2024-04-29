#include <cassert>
#include <cstring>
#include <vector>
#include "CU.h"

#include "scuda.h"
#include "CSGPrim.h"
#include "SCSGPrimSpec.h"

int main(int argc, char** argv)
{
    int primOffset = 0 ; 
    int numPrim = 10 ; 

    std::vector<CSGPrim> pp ; 
    for(int i=0 ; i < numPrim ; i++)
    {
        CSGPrim p = {} ; 
        p.setSbtIndexOffset(i);
        p.setAABB( float(1+i)*100.f );  
        pp.push_back(p); 
    }

    CSGPrim* d_prim = CU::UploadVec(pp); 
    SCSGPrimSpec psd = CSGPrim::MakeSpec( d_prim,  primOffset, numPrim ); ;
    psd.device = true ; 
    psd.downloadDump("CUTest.downloadDump"); 


    std::vector<CSGPrim> tt ; 
    CU::DownloadVec(tt, d_prim, numPrim);
    for(int i=0 ; i < numPrim ; i++)
    {
        const CSGPrim& p = tt[i] ; 
        std::cout << p.desc() << std::endl;  
    }
    assert( 0 == memcmp( tt.data(), pp.data(), sizeof(CSGPrim)*numPrim ) ); 

    return 0 ; 
}


