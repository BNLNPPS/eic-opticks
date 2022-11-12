/**
SNameTest
===========

Names within a geometry tend to change often
so for maintainability (and because they are geometry specific) 
cannot assert when the names do not match expectations::

    CFBase=~/.opticks/GEOM/J004 SNameTest

**/


#include "SPath.hh"
#include "SSys.hh"
#include "NP.hh"
#include "SName.h"

struct SNameTest
{
    SName* id ; 
    char qt ; 

    SNameTest(int argc, char** argv); 
    void test_desc(); 
    void test_detail(); 
    void test_findIndex(int argc, char** argv); 
    void test_findIndices(int argc, char** argv); 
    void test_getIDXListFromSearch(); 
    void test_getIDXListFromNames(); 
    void test_hasNames(); 
};


SNameTest::SNameTest(int argc, char** argv)
    :
    id(SName::Load("$CFBase/CSGFoundry/meshname.txt")),
    qt(SSys::getenvchar("QTYPE", 'S'))
{
    assert( qt == 'S' || qt == 'E' || qt == 'C' ); 
    test_desc(); 
    //test_detail(); 
    test_findIndex(argc, argv); 
    test_findIndices(argc, argv); 
    test_getIDXListFromSearch();
    test_getIDXListFromNames();
    test_hasNames(); 
} 

void SNameTest::test_desc()
{
    std::cout << "id.desc()" << std::endl << id->desc() << std::endl ; 
}
void SNameTest::test_detail()
{
    std::cout << "id.detail()" << std::endl << id->detail() << std::endl ; 
}

void SNameTest::test_findIndex( int argc, char** argv )
{
    for(int i=1 ; i < argc ; i++)
    {
         const char* arg = argv[i] ; 
         unsigned count = 0 ; 
         int max_count = -1 ; 
         int idx = id->findIndex(arg, count, max_count); 
         std::cout 
             << " findIndex " << std::setw(80) << arg 
             << " count " << std::setw(3) << count 
             << " idx " << std::setw(3) << idx
             << std::endl 
             ;
    }
}

void SNameTest::test_findIndices( int argc, char** argv )
{
    for(int i=1 ; i < argc ; i++)
    {
         const char* arg = argv[i] ; 

         std::vector<unsigned> idxs ;   
         id->findIndices(idxs, arg, qt); 

         const char* elv = SName::IDXList(idxs); 
         std::cout 
             << " findIndices " << std::setw(80) << arg 
             << " idxs.size " << std::setw(3) << idxs.size() 
             << " SName::QTypeLabel " << SName::QTypeLabel(qt)
             << std::endl 
             << "descIndices"
             << std::endl 
             << id->descIndices(idxs)
             << std::endl
             << "SName::ELVString:[" << elv << "]"
             << std::endl 
             ;
    }
}

void SNameTest::test_getIDXListFromSearch()
{
     const char* contain = "_virtual0x" ; 
     const char* elv = id->getIDXListFromSearch(contain); 
     std::cout 
         << "test_getIDXListFromSearch" 
         << " contain [" << contain << "]"
         << " elv [" << elv << "]"
         << std::endl 
         ;
}


void SNameTest::test_getIDXListFromNames()
{
     const char* x_elv = "t110,117,134" ; 
     const char* names = "NNVTMCPPMTsMask_virtual0x,HamamatsuR12860sMask_virtual0x,mask_PMT_20inch_vetosMask_virtual0x" ; 
     char delim = ',' ; 
     const char* prefix = "t" ; 
     const char* elv = id->getIDXListFromNames(names, delim, prefix); 
     bool match = strcmp( elv, x_elv) == 0 ; 

     std::cout 
         << "test_getIDXListFromNames" 
         << " names [" << names << "]"
         << " elv [" << elv << "]"
         << " x_elv [" << x_elv << "]"
         << " match " << match 
         << std::endl 
         ;

     //assert( match ); 
}

void SNameTest::test_hasNames()
{
     const char* names = "NNVTMCPPMTsMask_virtual0x,HamamatsuR12860sMask_virtual0x,mask_PMT_20inch_vetosMask_virtual0x" ; 
     bool has = id->hasNames(names); 
     std::cout 
         << "test_hasNames"
         << " names [" << names << "]"
         << " has " << has
         << std::endl 
         ;
}

int main(int argc, char** argv)
{
    SNameTest t(argc, argv); 
    return 0 ; 
}
