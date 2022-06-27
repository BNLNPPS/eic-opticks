
#include "OPTICKS_LOG.hh"
#include "OpticksGenstep.h"
#include "SEventConfig.hh"
#include "SEvt.hh"


void test_AddGenstep()
{
   SEvt evt ; 

   for(unsigned i=0 ; i < 10 ; i++)
   {
       quad6 q ; 
       q.set_numphoton(1000) ; 
       unsigned gentype = i % 2 == 0 ? OpticksGenstep_SCINTILLATION : OpticksGenstep_CERENKOV ;  
       q.set_gentype(gentype); 

       SEvt::AddGenstep(q);    
   }

   std::cout << SEvt::Get()->desc() << std::endl ; 
}






int main(int argc, char** argv)
{
    OPTICKS_LOG(argc, argv); 

    /*
    test_AddGenstep(); 
    */

    unsigned max_bounce = 9 ; 
    SEventConfig::SetMaxBounce(max_bounce); 
    SEventConfig::SetMaxRecord(max_bounce+1); 
    SEventConfig::SetMaxRec(max_bounce+1); 
    SEventConfig::SetMaxSeq(max_bounce+1); 

    SEvt sev ; 

    SEvt::SetIndex(-214); 
    SEvt::UnsetIndex(); 

    quad6 gs ; 
    gs.set_numphoton(1) ; 
    gs.set_gentype(OpticksGenstep_TORCH); 

    sev.addGenstep(gs);  

    spho label = {0,0,0,0} ; 

    sev.beginPhoton(label); 

    int bounce0 = 0 ; 
    int bounce1 = 0 ; 

    bounce0 = sev.slot[label.id] ;  
    sev.pointPhoton(label);  
    bounce1 = sev.slot[label.id] ;  
    assert( bounce1 == bounce0 + 1 ); 

    std::cout 
         << " i " << std::setw(3) << -1
         << " bounce0 " << std::setw(3) << bounce0 
         << " : " << sev.current_ctx.p.descFlag() 
         << std::endl
         ; 


    std::vector<unsigned> history = { 
       BOUNDARY_TRANSMIT, 
       BOUNDARY_TRANSMIT, 
       BULK_SCATTER, 
       BULK_SCATTER, 
       BULK_SCATTER, 
       BULK_SCATTER, 
       BULK_SCATTER, 
       BULK_SCATTER, 
       BULK_SCATTER, 
       BULK_SCATTER, 
       BULK_SCATTER, 
       BULK_SCATTER, 
       BULK_REEMIT, 
       BOUNDARY_TRANSMIT, 
       SURFACE_DETECT
    } ; 

    for(int i=0 ; i < int(history.size()) ; i++)
    {   
        unsigned flag = history[i] ; 
        sev.current_ctx.p.set_flag(flag); 

        bounce0 = sev.slot[label.id] ;  
        sev.pointPhoton(label);  
        bounce1 = sev.slot[label.id] ;  
        assert( bounce1 == bounce0 + 1 ); 

        std::cout 
             << " i " << std::setw(3) << i 
             << " bounce0 " << std::setw(3) << bounce0 
             << " : " << sev.current_ctx.p.descFlag() 
             << std::endl
             ; 
    }

    sev.finalPhoton(label); 

    sev.save("$TMP/SEvtTest");
    LOG(info) << sev.desc() ;

    return 0 ; 
}

