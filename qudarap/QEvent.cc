#include <cuda_runtime.h>
#include <sstream>

#include "SEvt.hh"
#include "SGeo.hh"
#include "SPath.hh"

#include "scuda.h"
#include "squad.h"
#include "sphoton.h"
#include "srec.h"
#include "sseq.h"
#include "sevent.h"

#include "sqat4.h"
#include "stran.h"
#include "SU.hh"

#include "SComp.h"
#include "SGenstep.hh"
#include "SEvent.hh"
#include "SEvt.hh"
#include "SEventConfig.hh"
#include "NP.hh"
#include "PLOG.hh"

#include "OpticksGenstep.h"

#include "QEvent.hh"
#include "QBuf.hh"
#include "QBuf.hh"
#include "QU.hh"


template struct QBuf<quad6> ; 

const plog::Severity QEvent::LEVEL = PLOG::EnvLevel("QEvent", "DEBUG"); 
QEvent* QEvent::INSTANCE = nullptr ; 
QEvent* QEvent::Get(){ return INSTANCE ; }

sevent* QEvent::getDevicePtr() const
{
    return d_evt ; 
}

/**
QEvent::QEvent
----------------

Instanciation allocates device buffers with sizes configured by SEventConfig

* As selector is only needed CPU side it is not down in sevent.h 
  but it could be in SEvt.hh 

**/

QEvent::QEvent()
    :
    sev(SEvt::Get()),
    selector(sev ? sev->selector : nullptr),
    evt(sev ? sev->evt : nullptr),
    d_evt(QU::device_alloc<sevent>(1)),
    gs(nullptr),
    input_photon(nullptr),
    upload_count(0),
    meta()
{
    INSTANCE = this ; 
    init(); 
}

/**
QEvent::init
--------------

Only configures limits, no allocation yet. Allocation happens in QEvent::setGenstep QEvent::setNumPhoton

HMM: hostside sevent.h instance could reside in SEvt together with selector then hostside setup
can be common between the branches 

**/

void QEvent::init()
{
    if(!sev) LOG(fatal) << "QEvent instanciated before SEvt instanciated : this is not going to fly " ; 

    assert(sev); 
    assert(evt); 
    assert(selector); 

    sev->setCompProvider(this);  
}

NP* QEvent::getDomain() const { return sev ? sev->getDomain() : nullptr ; }


std::string QEvent::desc() const
{
    std::stringstream ss ; 
    ss << evt->desc() << std::endl ;
    std::string s = ss.str();  
    return s ; 
}


void QEvent::setMeta(const char* meta_)
{
    meta = meta_ ;   // std::string
} 

bool QEvent::hasMeta() const 
{
    return meta.empty() == false ; 
}


/**
QEvent::setGenstep
--------------------

Canonically invoked from QSim::simulate and QSim::simtrace just prior to cx->launch 

1. gensteps uploaded to QEvent::init allocated evt->genstep device buffer, 
   overwriting any prior gensteps and evt->num_genstep is set 

2. *count_genstep_photons* calculates the total number of seeds (and photons) by 
   adding the photons from each genstep and setting evt->num_seed

3. *fill_seed_buffer* populates seed buffer using num photons per genstep from genstep buffer

3. invokes setNumPhoton which may allocate records


* HMM: find that without zeroing the seed buffer the seed filling gets messed up causing QEventTest fails 
  doing this in QEvent::init is not sufficient need to do in QEvent::setGenstep.  
  **This is a documented limitation of sysrap/iexpand.h**
 
  So far it seems that no zeroing is needed for the genstep buffer. 

HMM: what about simtrace ? ce-gensteps are very different to ordinary gs 

**/

int QEvent::setGenstep()  // onto device
{
    NP* gs = SEvt::GetGenstep(); 
    SEvt::Clear();   // clear the quad6 vector, ready to collect more genstep
    if(gs == nullptr) LOG(fatal) << "Must SEvt::AddGenstep before calling QEvent::setGenstep " ;
    return gs == nullptr ? -1 : setGenstep(gs) ; 
} 

int QEvent::setGenstep(NP* gs_) 
{ 
    gs = gs_ ; 
    SGenstep::Check(gs); 
    evt->num_genstep = gs->shape[0] ; 

    if( evt->genstep == nullptr && evt->seed == nullptr ) 
    {
        LOG(info) << " device_alloc genstep and seed " ; 
        evt->genstep = QU::device_alloc<quad6>( evt->max_genstep ) ; 
        evt->seed    = QU::device_alloc<int>(   evt->max_photon )  ;
    }

    LOG(LEVEL) << SGenstep::Desc(gs, 10) ;
 
    bool num_gs_allowed = evt->num_genstep <= evt->max_genstep ;
    if(!num_gs_allowed) LOG(fatal) << " evt.num_genstep " << evt->num_genstep << " evt.max_genstep " << evt->max_genstep ; 
    assert( num_gs_allowed ); 

    QU::copy_host_to_device<quad6>( evt->genstep, (quad6*)gs->bytes(), evt->num_genstep ); 

    QU::device_memset<int>(   evt->seed,    0, evt->max_photon );

    //count_genstep_photons();   // sets evt->num_seed
    //fill_seed_buffer() ;       // populates seed buffer
    count_genstep_photons_and_fill_seed_buffer();   // combi-function doing what both the above do 


    int gencode0 = SGenstep::GetGencode(gs, 0); // gencode of first genstep   

    if(OpticksGenstep_::IsFrame(gencode0))
    {
        setNumSimtrace( evt->num_seed ); 
    }
    else if(OpticksGenstep_::IsInputPhoton(gencode0))
    {
        setInputPhoton(); 
    }
    else
    {
        setNumPhoton( evt->num_seed );  // photon, rec, record may be allocated here depending on SEventConfig
    }
    upload_count += 1 ; 
    return 0 ; 
}

/**
QEvent::setInputPhoton
------------------------

This is a private method invoked only from QEvent::setGenstep

**/

void QEvent::setInputPhoton()
{
    input_photon = SEvt::GetInputPhoton(); 
    if( input_photon == nullptr ) 
        LOG(fatal) 
            << " INCONSISTENT : OpticksGenstep_INPUT_PHOTON by no input photon array " 
            ; 

    assert(input_photon);  
    assert(input_photon->has_shape( -1, 4, 4) ); 
    int num_photon = input_photon->shape[0] ; 
    assert( evt->num_seed == num_photon ); 

    setNumPhoton( num_photon ); 
    QU::copy_host_to_device<sphoton>( evt->photon, (sphoton*)input_photon->bytes(), num_photon ); 
}



int QEvent::setGenstep(quad6* qgs, unsigned num_gs )  // TODO: what uses this ? eliminate ?
{
    NP* gs_ = NP::Make<float>( num_gs, 6, 4 ); 
    gs_->read2( (float*)qgs );   
    return setGenstep( gs_ ); 
}




bool QEvent::hasGenstep() const { return evt->genstep != nullptr ; }
bool QEvent::hasSeed() const {    return evt->seed != nullptr ; }
bool QEvent::hasPhoton() const {  return evt->photon != nullptr ; }
bool QEvent::hasRecord() const { return evt->record != nullptr ; }
bool QEvent::hasRec() const    { return evt->rec != nullptr ; }
bool QEvent::hasSeq() const    { return evt->seq != nullptr ; }
bool QEvent::hasHit() const    { return evt->hit != nullptr ; }
bool QEvent::hasSimtrace() const  { return evt->simtrace != nullptr ; }




/**
QEvent::count_genstep_photons
------------------------------

thrust::reduce using strided iterator summing over GPU side gensteps 

**/

extern "C" unsigned QEvent_count_genstep_photons(sevent* evt) ; 
unsigned QEvent::count_genstep_photons()
{
   return QEvent_count_genstep_photons( evt );  
}

/**
QEvent::fill_seed_buffer
---------------------------

Populates seed buffer using the number of photons from each genstep 

The photon seed buffer is a device buffer containing integer indices referencing 
into the genstep buffer. The seeds provide the association between the photon 
and the genstep required to generate it.

**/

extern "C" void QEvent_fill_seed_buffer(sevent* evt ); 
void QEvent::fill_seed_buffer()
{
    QEvent_fill_seed_buffer( evt ); 
}

extern "C" void QEvent_count_genstep_photons_and_fill_seed_buffer(sevent* evt ); 
void QEvent::count_genstep_photons_and_fill_seed_buffer()
{
    QEvent_count_genstep_photons_and_fill_seed_buffer( evt ); 
}





NP* QEvent::getGenstep() const 
{
    return gs ; 
}

NP* QEvent::getInputPhoton() const 
{
    return input_photon ; 
}




/**
QEvent::getGenstepFromDevice
-----------------------------

Gensteps originate on host and are uploaded to device, so downloading
them from device is not usually done. 

**/

NP* QEvent::getGenstepFromDevice() const 
{
    NP* a = NP::Make<float>( evt->num_genstep, 6, 4 ); 
    QU::copy_device_to_host<quad6>( (quad6*)a->bytes(), evt->genstep, evt->num_genstep ); 
    return a ; 
}


NP* QEvent::getSeed() const 
{
    if(!hasSeed()) LOG(fatal) << " getSeed called when there is no such array, use SEventConfig::SetCompMask to avoid " ; 
    if(!hasSeed()) return nullptr ;  
    NP* s = NP::Make<int>( evt->num_seed ); 
    QU::copy_device_to_host<int>( (int*)s->bytes(), evt->seed, evt->num_seed ); 
    return s ; 
}


/**
QEvent::getPhoton(NP* p) :  mutating API
-------------------------------------------
**/

void QEvent::getPhoton(NP* p) const 
{
    LOG(fatal) << "[ evt.num_photon " << evt->num_photon << " p.sstr " << p->sstr() << " evt.photon " << evt->photon ; 
    assert( p->has_shape(evt->num_photon, 4, 4) ); 
    QU::copy_device_to_host<sphoton>( (sphoton*)p->bytes(), evt->photon, evt->num_photon ); 
    LOG(fatal) << "] evt.num_photon " << evt->num_photon  ; 
}

NP* QEvent::getPhoton() const 
{
    NP* p = NP::Make<float>( evt->num_photon, 4, 4);
    getPhoton(p); 
    return p ; 
}




void QEvent::getSimtrace(NP* t) const 
{
    LOG(fatal) << "[ evt.num_simtrace " << evt->num_simtrace << " t.sstr " << t->sstr() << " evt.simtrace " << evt->simtrace ; 
    assert( t->has_shape(evt->num_simtrace, 4, 4) ); 
    QU::copy_device_to_host<quad4>( (quad4*)t->bytes(), evt->simtrace, evt->num_simtrace ); 
    LOG(fatal) << "] evt.num_simtrace " << evt->num_simtrace  ; 
}
NP* QEvent::getSimtrace() const 
{
    if(!hasSimtrace()) LOG(fatal) << " getSimtrace called when there is no such array, use SEventConfig::SetCompMask to avoid " ;
    if(!hasSimtrace()) return nullptr ;  
    NP* t = NP::Make<float>( evt->num_simtrace, 4, 4);
    getSimtrace(t); 
    return t ; 
}

void QEvent::getSeq(NP* seq) const 
{
    if(!hasSeq()) return ; 
    LOG(fatal) << "[ evt.num_seq " << evt->num_seq << " seq.sstr " << seq->sstr() << " evt.seq " << evt->seq ; 
    assert( seq->has_shape(evt->num_seq, 2) ); 
    QU::copy_device_to_host<sseq>( (sseq*)seq->bytes(), evt->seq, evt->num_seq ); 
    LOG(fatal) << "] evt.num_seq " << evt->num_seq  ; 
}

NP* QEvent::getSeq() const 
{
    if(!hasSeq()) LOG(fatal) << " getSeq called when there is no such array, use SEventConfig::SetCompMask to avoid " ; 
    if(!hasSeq()) return nullptr ;
  
    NP* seq = sev->makeSeq(); 

    getSeq(seq); 
    return seq ; 
}

NP* QEvent::getRecord() const 
{
    if(!hasRecord()) LOG(fatal) << " getRecord called when there is no such array, use SEventConfig::SetCompMask to avoid " ; 
    if(!hasRecord()) return nullptr ;  

    NP* r = sev->makeRecord(); 

    LOG(info) << " evt.num_record " << evt->num_record ; 
    QU::copy_device_to_host<sphoton>( (sphoton*)r->bytes(), evt->record, evt->num_record ); 
    return r ; 
}

NP* QEvent::getRec() const 
{
    if(!hasRec()) LOG(fatal) << " getRec called when there is no such array, use SEventConfig::SetCompMask to avoid " ; 
    if(!hasRec()) return nullptr ;  

    NP* r = sev->makeRec(); 

    LOG(info) 
        << " evt.num_photon " << evt->num_photon 
        << " evt.max_rec " << evt->max_rec 
        << " evt.num_rec " << evt->num_rec  
        << " evt.num_photon*evt.max_rec " << evt->num_photon*evt->max_rec  
        ;

    assert( evt->num_photon*evt->max_rec == evt->num_rec );  

    QU::copy_device_to_host<srec>( (srec*)r->bytes(), evt->rec, evt->num_rec ); 
    return r ; 
}


unsigned QEvent::getNumHit() const 
{
    assert( evt->photon ); 
    assert( evt->num_photon ); 

    evt->num_hit = SU::count_if_sphoton( evt->photon, evt->num_photon, *selector );    

    LOG(info) << " evt.photon " << evt->photon << " evt.num_photon " << evt->num_photon << " evt.num_hit " << evt->num_hit ;  
    return evt->num_hit ; 
}

/**
QEvent::getHit
------------------

1. count *evt.num_hit* passing the photon *selector* 
2. allocate *evt.hit* GPU buffer
3. copy_if from *evt.photon* to *evt.hit* using the photon *selector*
4. host allocate the NP hits array
5. copy hits from device to the host NP hits array 
6. free *evt.hit* on device
7. return NP hits array to caller, who becomes owner of the array 

Note that the device hits array is allocated and freed for each launch.  
This is due to the expectation that the number of hits will vary greatly from launch to launch 
unlike the number of photons which is expected to be rather similar for most launches other than 
remainder last launches. 

The alternative to this dynamic "busy" handling of hits would be to reuse a fixed hits buffer
sized to max_photons : that however seems unpalatable due it always doubling up GPU memory for 
photons and hits.  

hitmask metadata was formerly placed on the hit array, 
subsequently moved to domain_meta as domain should 
always be present, unlike hits. 

**/

NP* QEvent::getHit() const 
{
    // hasHit at this juncture is misleadingly always false, 
    // because the hits array is derived by *getHit_* which  selects from the photons 
    if(!hasPhoton()) LOG(fatal) << " getHit called when there is no photon array " ; 
    if(!hasPhoton()) return nullptr ; 

    assert( evt->photon ); 
    assert( evt->num_photon ); 
    evt->num_hit = SU::count_if_sphoton( evt->photon, evt->num_photon, *selector );    

    LOG(info) 
         << " evt.photon " << evt->photon 
         << " evt.num_photon " << evt->num_photon 
         << " evt.num_hit " << evt->num_hit
         << " selector.hitmask " << selector->hitmask
         << " SEventConfig::HitMask " << SEventConfig::HitMask()
         << " SEventConfig::HitMaskLabel " << SEventConfig::HitMaskLabel()
         ;  

    NP* hit = evt->num_hit > 0 ? getHit_() : nullptr ; 

    return hit ; 
}

NP* QEvent::getHit_() const 
{
    evt->hit = QU::device_alloc<sphoton>( evt->num_hit ); 

    SU::copy_if_device_to_device_presized_sphoton( evt->hit, evt->photon, evt->num_photon,  *selector );

    NP* hit = NP::Make<float>( evt->num_hit, 4, 4 ); 

    QU::copy_device_to_host<sphoton>( (sphoton*)hit->bytes(), evt->hit, evt->num_hit );

    QU::device_free<sphoton>( evt->hit ); 

    evt->hit = nullptr ; 
    LOG(info) << " hit.sstr " << hit->sstr() ; 

    return hit ; 
}



std::string QEvent::getMeta() const 
{     
    return meta ; 
}
NP* QEvent::getComponent(unsigned comp) const 
{
    unsigned mask = SEventConfig::CompMask(); 
    return mask & comp ? getComponent_(comp) : nullptr ; 
}
NP* QEvent::getComponent_(unsigned comp) const 
{
    NP* a = nullptr ; 
    switch(comp)
    {   
        case SCOMP_GENSTEP:   a = getGenstep()  ; break ;   
        case SCOMP_PHOTON:    a = getPhoton()   ; break ;   
        case SCOMP_RECORD:    a = getRecord()   ; break ;   
        case SCOMP_REC:       a = getRec()      ; break ;   
        case SCOMP_SEQ:       a = getSeq()      ; break ;   
        case SCOMP_SEED:      a = getSeed()     ; break ;   
        case SCOMP_HIT:       a = getHit()      ; break ;   
        case SCOMP_SIMTRACE:  a = getSimtrace() ; break ;   
        case SCOMP_DOMAIN:    a = getDomain()   ; break ;   
        case SCOMP_INPHOTON:  a = getInputPhoton() ; break ;   
    }   
    return a ; 
}



/**
QEvent::setNumPhoton
---------------------

Canonically invoked internally from QEvent::setGenstep but may be invoked 
directly from "friendly" photon only tests without use of gensteps.  

Sets evt->num_photon asserts that is within allowed *evt->max_photon* and calls *uploadEvt*

This assumes that the number of photons for subsequent launches does not increase 
when collecting records : that is ok as running with records is regarded as debugging. 
**/

void QEvent::setNumPhoton(unsigned num_photon )
{
    bool num_photon_allowed = int(num_photon) <= evt->max_photon ; 
    if(!num_photon_allowed) LOG(fatal) << " num_photon " << num_photon << " evt.max_photon " << evt->max_photon ; 
    assert( num_photon_allowed ); 

    if( evt->photon == nullptr ) 
    {
        // TODO: use SEvt::setNumPhoton to modify the evt->num_...  splitting alloc from changing num
        evt->num_photon = num_photon  ; 
        evt->num_record = evt->max_record * evt->num_photon ;  
        evt->num_seq    = evt->max_seq > 0 ? evt->num_photon : 0 ; 
        evt->num_rec    = evt->max_rec * evt->num_photon ;  

        evt->photon  = evt->num_photon > 0 ? QU::device_alloc_zero<sphoton>( evt->max_photon ) : nullptr ; 
        evt->record  = evt->num_record > 0 ? QU::device_alloc_zero<sphoton>( evt->num_record ) : nullptr ; 
        evt->rec     = evt->num_rec    > 0 ? QU::device_alloc_zero<srec>(    evt->num_rec  )   : nullptr ; 
        evt->seq     = evt->num_seq    > 0 ? QU::device_alloc_zero<sseq>(    evt->num_seq  )   : nullptr ; 

        LOG(info) 
            << " device_alloc photon " 
            << " evt.num_photon " << evt->num_photon 
            << " evt.max_photon " << evt->max_photon
            << " evt.num_record " << evt->num_record 
            << " evt.num_rec    " << evt->num_rec 
            ;

    } 
    else
    {
         LOG(error) << " evt.photon is not nullptr : evt.photon : " << evt->photon ; 
    }


    uploadEvt(); 
}

void QEvent::setNumSimtrace(unsigned num_simtrace)
{
    evt->num_simtrace = num_simtrace ; 
    bool num_simtrace_allowed = evt->num_simtrace <= evt->max_simtrace ; 
    if(!num_simtrace_allowed) LOG(fatal) << " evt.num_simtrace " << evt->num_simtrace << " evt.max_simtrace " << evt->max_simtrace ; 
    assert( num_simtrace_allowed ); 

    if( evt->simtrace == nullptr ) 
    {
        evt->simtrace = QU::device_alloc<quad4>( evt->max_simtrace ) ; 
    
        LOG(info) 
            << " device_alloc simtrace " 
            << " evt.num_simtrace " << evt->num_simtrace 
            << " evt.max_simtrace " << evt->max_simtrace
            ;
    }
    uploadEvt(); 
}
 


unsigned QEvent::getNumPhoton() const
{
    return evt->num_photon ; 
}
unsigned QEvent::getNumSimtrace() const
{
    return evt->num_simtrace ; 
}


/**
QEvent::uploadEvt 
--------------------

Copies host side *evt* instance (with updated num_genstep and num_photon) to device side  *d_evt*.  
Note that the evt->genstep and evt->photon pointers are not updated, so the same buffers are reused for each launch. 

**/

void QEvent::uploadEvt()
{
    LOG(LEVEL) << std::endl << evt->desc() ; 
    QU::copy_host_to_device<sevent>(d_evt, evt, 1 );  
}

extern "C" void QEvent_checkEvt(dim3 numBlocks, dim3 threadsPerBlock, sevent* evt, unsigned width, unsigned height ) ; 

void QEvent::checkEvt() 
{ 
    unsigned width = getNumPhoton() ; 
    unsigned height = 1 ; 
    LOG(info) << " width " << width << " height " << height ; 

    dim3 numBlocks ; 
    dim3 threadsPerBlock ; 
    QU::ConfigureLaunch( numBlocks, threadsPerBlock, width, height ); 
 
    assert( d_evt ); 
    QEvent_checkEvt(numBlocks, threadsPerBlock, d_evt, width, height );   
}


