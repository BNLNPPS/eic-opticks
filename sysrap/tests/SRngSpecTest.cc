#include <sstream>
#include <csignal>
#include <vector>
#include "SRngSpec.hh"
#include "SEvt.hh"
#include "OPTICKS_LOG.hh"

int main(int argc, char** argv)
{
    OPTICKS_LOG(argc, argv); 

    SEvt* evt = SEvt::Create(0) ; 
    LOG(info) << evt->brief() ; 

    unsigned long long seed = 0ull ; 
    unsigned long long offset = 0ull ; 

    LOG(info); 

#if defined __APPLE__
    std::vector<unsigned> rngmax_M = { 1, 3  } ; 
#else
    std::vector<unsigned> rngmax_M = { 1, 3, 10 } ; 
#endif

    for(unsigned i=0 ; i < rngmax_M.size() ; i++)
    {
        unsigned rngmax = rngmax_M[i]*1000000 ; 
        const char* path1 = SRngSpec::CURANDStatePath(NULL, rngmax, seed, offset) ;       
        std::cout << path1 << std::endl ;

        const SRngSpec* spec = new SRngSpec(rngmax, seed, offset); 
        const char* path2 = spec->getCURANDStatePath(); 
        bool path_expect = strcmp(path1, path2) == 0 ;
        if(!path_expect) std::raise(SIGINT); 
        assert(path_expect); 

        std::cout << spec->desc() << std::endl ; 


        bool valid = spec->isValid(); 
        if(!valid)
        {
            LOG(error) << " invalid for rngmax " << rngmax ; 
        }

        assert( valid ); 
    }

    return 0 ; 
}
