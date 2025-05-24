#include <iostream>

#include "OPTICKS_LOG.hh"
#include "spath.h"
#include "GDXML.hh"

int main(int argc, char** argv)
{
    OPTICKS_LOG(argc, argv); 

    const char* srcpath = argc > 1 ? argv[1] : nullptr ; 
    const char* dstpath = spath::Resolve("$TMP/GDXMLTest/test.gdml") ; 

    if(!srcpath) 
    {
        std::cout << "Expecting a single argument /path/to/name.gdml " << std::endl ; 
        std::cout << "when issues are detected with the GDML a kludge fixed version is written to /path/to/name_GDXML.gdml " << std::endl ;   
        return 0 ; 
    }

    GDXML::Fix(dstpath, srcpath) ; 

    LOG(info)
        << " srcpath " << srcpath  
        << " dstpath " << dstpath  
        ; 

    return 0 ; 
}

