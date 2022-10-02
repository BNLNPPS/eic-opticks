/*
 * Copyright (c) 2019 Opticks Team. All Rights Reserved.
 *
 * This file is part of Opticks
 * (see https://bitbucket.org/simoncblyth/opticks).
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); 
 * you may not use this file except in compliance with the License.  
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software 
 * distributed under the License is distributed on an "AS IS" BASIS, 
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
 * See the License for the specific language governing permissions and 
 * limitations under the License.
 */

#include <cassert>
#include <iostream>


#include "SSys.hh"
#include "SProc.hh"

// npy-
#include "NPY.hpp"
#include "GLMPrint.hpp"
#include "RecordsNPY.hpp"
#include "OPTICKS_LOG.hh"

// okc-
#include "Opticks.hh"
#include "OpticksEventSpec.hh"
#include "OpticksEvent.hh"
#include "OpticksGenstep.hh"


void test_genstep_derivative()
{
    OpticksEventSpec sp("cerenkov", "1", "dayabay", "") ;
    OpticksEvent evt(&sp) ;

    NPY<float>* trk = evt.loadGenstepDerivativeFromFile("track");
    assert(trk);

    LOG(info) << trk->getShapeString();

    glm::vec4 origin    = trk->getQuadF(0,0) ;
    glm::vec4 direction = trk->getQuadF(0,1) ;
    glm::vec4 range     = trk->getQuadF(0,2) ;

    print(origin,"origin");
    print(direction,"direction");
    print(range,"range");

}


void test_genstep()
{   
    OpticksEventSpec sp("cerenkov", "1", "dayabay", "") ;
    OpticksEvent evt(&sp) ;

/*
   not compiling anymore
    evt.setGenstepData(evt.loadGenstepFromFile());
    evt.dumpPhotonData();
*/
}

void test_appendNote()
{   
    OpticksEventSpec sp("cerenkov", "1", "dayabay", "") ;
    OpticksEvent evt(&sp) ;

    evt.appendNote("hello");
    evt.appendNote("world");

    std::string note = evt.getNote();

    bool match = note.compare("hello world") == 0 ;
    LOG_IF(fatal, !match) << "got unexpected " << note ; 
    assert(match);

}

void test_resetEvent(Opticks* ok, unsigned nevt, char ctrl)
{
    unsigned num_photons = 10000 ; 
    unsigned tagoffset = 0 ; 

    NPY<float>* gs = OpticksGenstep::MakeCandle(num_photons, tagoffset ) ;

    float vm0 = SProc::VirtualMemoryUsageMB() ; 

    for(unsigned i=0 ; i < nevt ; i++)
    {
        ok->createEvent(gs, ctrl); 
        ok->resetEvent(ctrl); 
    }

    float vm1 = SProc::VirtualMemoryUsageMB() ; 
    float dvm = vm1 - vm0 ; 
    float leak_per_evt = dvm/float(nevt) ; 

    LOG(info) 
       << " vm0 " << vm0
       << " vm1 " << vm1
       << " dvm " << dvm
       << " nevt " << nevt 
       << " leak_per_evt (MB) " << leak_per_evt 
       << " ctrl [" << ctrl << "]"
       ;

    delete gs ; 

}

void test_addNumPhotons(Opticks* ok, unsigned num_onestep_photons, unsigned num_gs )
{
    LOG(info) << " num_onestep_photons " << num_onestep_photons << " num_gs " << num_gs ; 
    char ctrl = '-' ; 
    unsigned tagoffset = 0 ; 
    ok->createEvent(tagoffset, ctrl); 
    OpticksEvent* evt = ok->getEvent(ctrl); 

    NPY<float>* photons = evt->getPhotonData(); 
    NPY<float>* onestep = nullptr ; 

    for(unsigned i=0 ; i < num_gs ; i++)
    {
        assert( photons == evt->getPhotonData() ); 

        onestep = NPY<float>::make(num_onestep_photons, 4, 4) ;
        onestep->zero(); 
        onestep->fill(float(i)); 

        //evt->addNumPhotons(num_onestep_photons); // makes little sense as the NPY::add increments anyhow

        photons->add(onestep) ; 
         
        evt->updateNumPhotonsFromPhotonArraySize();    

        std::cout << evt->desc() << std::endl ; 
    }

    const char* path = "$TMP/okc/test_addNumPhotons.npy" ; 
    LOG(info) << " save to " << path ; 
    photons->save(path); 
    SSys::npdump(path, "np.float32" ); 
}


int main(int argc, char** argv)
{
    //int nevt = argc > 1 ? atoi(argv[1]) : 1000 ; 

    OPTICKS_LOG(argc, argv);
    //test_genstep_derivative();
    //test_genstep();
    //test_appendNote();

    Opticks ok(argc, argv); 
    ok.configure(); 

    glm::vec4 space_domain(0.f,0.f,0.f,1000.f); 
    ok.setSpaceDomain(space_domain); 

    //test_resetEvent(&ok, nevt, '+'); 
    //test_resetEvent(&ok, nevt, '-'); 


    test_addNumPhotons(&ok, 100, 10) ; 


    return 0 ;
}
