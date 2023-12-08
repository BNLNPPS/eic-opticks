// ./CSGSolidTest.sh
#include <iostream>
#include <csignal>

#include "OPTICKS_LOG.hh"
#include "scuda.h"
#include "CSGSolid.h"
#include "NP.hh"

void test_Make_Write(const char* path)
{
    LOG(info) << path ; 

    CSGSolid r =  CSGSolid::Make("red"  ,    1, 0 ) ; r.center_extent = {1.f, 1.f, 1.f, 1.f } ;
    CSGSolid g =  CSGSolid::Make("green",    1, 1 ) ; g.center_extent = {2.f, 2.f, 2.f, 2.f } ;
    CSGSolid b =  CSGSolid::Make("blue",     1, 2 ) ; b.center_extent = {2.f, 2.f, 2.f, 2.f } ;
    CSGSolid c =  CSGSolid::Make("cyan",     1, 3 ) ; c.center_extent = {3.f, 3.f, 3.f, 3.f } ;
    CSGSolid m =  CSGSolid::Make("magenta",  1, 4 ) ; m.center_extent = {4.f, 4.f, 4.f, 4.f } ;
    CSGSolid y =  CSGSolid::Make("yellow",   1, 5 ) ; y.center_extent = {5.f, 5.f, 5.f, 5.f } ;

    std::vector<CSGSolid> so ; 

    so.push_back(r); 
    so.push_back(g); 
    so.push_back(b); 
    so.push_back(c); 
    so.push_back(m); 
    so.push_back(y); 

    for(int i=0 ; i < int(so.size()); i++) std::cout << so[i].desc() << std::endl ; 


    std::cout << "sizeof(CSGSolid)" << sizeof(CSGSolid) << std::endl ; 
    assert( sizeof(float) == sizeof(int));

    unsigned num_quad = 3 ; 
    assert( sizeof(CSGSolid) == num_quad*4*sizeof(float) ); 
    unsigned num_items = so.size(); 
    assert( num_items == 6 );
    unsigned num_values = sizeof(CSGSolid)/sizeof(int) ; 

    bool num_values_expect = num_values == num_quad*4 ;
    assert( num_values_expect ); 
    if(!num_values_expect) std::raise(SIGINT); 

    NP::Write( path, (int*)so.data(), num_items, num_values ) ;
}

void test_labelMatch()
{
    LOG(info); 

    CSGSolid r =  CSGSolid::Make("red"  ,    1, 0 ) ; r.center_extent = {1.f, 1.f, 1.f, 1.f } ;

    bool match_red = r.labelMatch("red") ;
    bool match_green = r.labelMatch("green") ;

    std::cout << " r.label " << r.label << std::endl ; 
    std::cout << " r.label " << r.label << " match_red " << match_red << std::endl ; 
    std::cout << " r.label " << r.label << " match_green " << match_green << std::endl ; 
}

void test_ParseLabel()
{
    LOG(info); 

    char t0 = 'r' ; 
    unsigned idx0 = 8 ; 
    std::string label = CSGSolid::MakeLabel(t0, idx0); 

    char t1 ; 
    unsigned idx1 ; 
    int rc = CSGSolid::ParseLabel(label.c_str(), t1, idx1 ); 
    assert( rc == 0 ); 
    if(rc!=0) std::raise(SIGINT); 

    assert( t0 == t1 ); 
    assert( idx0 == idx1 );  
}

void test_get_ridx()
{
    LOG(info); 

    char t0 = 'r' ; 
    unsigned idx0 = 8 ; 
    std::string label = CSGSolid::MakeLabel(t0, idx0); 

    unsigned numPrim = 1 ; 
    unsigned primOffset = 0 ; 
    CSGSolid so = CSGSolid::Make(label.c_str(), numPrim, primOffset );  

    bool ridx_expect = so.get_ridx() == int(idx0) ;
    assert(ridx_expect ); 
    if(!ridx_expect) std::raise(SIGINT); 
}



int main(int argc, char** argv)
{
    OPTICKS_LOG(argc, argv); 

    const char* path = argc > 1 ? argv[1] : "$TMP/CSGSolidTest/CSGSolidTest.npy" ; 
    test_Make_Write(path); 
    test_labelMatch(); 
    test_ParseLabel(); 
    test_get_ridx(); 

    return 0 ; 
}
