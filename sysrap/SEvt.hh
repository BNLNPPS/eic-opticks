#pragma once
/**
SEvt.hh
=========

Q: Where is this instanciated canonically ?
A: So far in the mains of tests


Attempting to do this header only gives duplicate symbol for the SEvt::INSTANCE.
It seems c++17 would allow "static inline SEvt* INSTANCE"  but c++17 
is problematic on laptop, so use separate header and implementation
for simplicity. 

It is possible with c++11 but is non-intuitive

* https://stackoverflow.com/questions/11709859/how-to-have-static-data-members-in-a-header-only-library

Replacing cfg4/CGenstepCollector

HMM: gs vector of sgs provides summary of the full genstep, 
changing the first quad of the genstep to hold this summary info 
would avoid the need for the summary vector and mean the genstep 
index and photon offset info was available on device.

Header of full genstep has plenty of spare bits to squeeze in
index and photon offset in addition to  gentype/trackid/matline/numphotons 

**/

#include <cassert>
#include <vector>
#include <string>
#include <sstream>
#include "plog/Severity.h"

#include "scuda.h"
#include "squad.h"
#include "sphoton.h"
#include "srec.h"
#include "sseq.h"
#include "squad.h"
#include "sgs.h"
#include "SComp.h"

struct sphoton_selector ; 
struct sevent ; 
struct sdebug ; 
struct NP ; 
struct NPFold ; 

#include "SYSRAP_API_EXPORT.hh"

struct SYSRAP_API SEvt : public SCompProvider
{
    int index ; 
    sphoton_selector* selector ; 
    sevent* evt ; 
    sdebug* dbg ; 
    std::string meta ; 
    const NP* input_photon ; 
    const SCompProvider*  provider ; 
    NPFold*               fold ; 


    std::vector<quad6> genstep ; 
    std::vector<sgs>   gs ; 
    std::vector<spho>  pho0 ;  // unordered push_back as they come 
    std::vector<spho>  pho ;   // spho are label structs holding 4*int 

    std::vector<int>     slot ; 
    std::vector<sphoton> photon ; 
    std::vector<sphoton> record ; 
    std::vector<srec>    rec ; 
    std::vector<sseq>    seq ; 

    spho    current_pho = {} ; 
    sphoton current_photon = {} ; 
    srec    current_rec = {} ; 
    sseq    current_seq = {} ; 


    static const plog::Severity LEVEL ; 
    static const int GIDX ; 
    static const int MISSING_INDEX ; 
    static SEvt* INSTANCE ; 
    static SEvt* Get() ; 
    static bool RECORDING ; 

    static void Check(); 
    static sgs AddGenstep(const quad6& q); 
    static sgs AddGenstep(const NP* a); 
    static void AddCarrierGenstep(); 
    static void AddTorchGenstep(); 

    static void Clear(); 
    static void Save() ; 
    static void Save(const char* base, const char* reldir ); 
    static void Save(const char* dir); 
    static void SetIndex(int index); 
    static void UnsetIndex(); 
    static int  GetIndex(); 
    static int GetNumPhoton(); 
    static NP* GetGenstep(); 
    static const NP* GetInputPhoton(); 

    bool isSelfProvider() const ; 
 
    SEvt(); 
    void init(); 
    void setCompProvider(const SCompProvider* provider); 
    void setNumPhoton(unsigned numphoton); 
    void resize(); 

    NP* getDomain() const ; 

    void clear() ; 
    unsigned getNumGenstep() const ; 

    void setIndex(int index_) ;  
    void unsetIndex() ;  
    int getIndex() const ; 

    unsigned getNumPhoton() const ; 
    sgs addGenstep(const quad6& q) ; 
    sgs addGenstep(const NP* a) ; 

    const sgs& get_gs(const spho& label) const ; // lookup genstep label from photon label  
    unsigned get_genflag(const spho& label) const ; 

    void beginPhoton(const spho& sp); 
    void rjoinPhoton(const spho& sp); 

    void rjoinRecordCheck(const sphoton& rj, const sphoton& ph  ) const ; 
    static void ComparePhotonDump(const sphoton& a, const sphoton& b ); 
    void rjoinPhotonCheck(const sphoton& ph) const ; 
    void rjoinSeqCheck(unsigned seq_flag) const ; 

    void checkPhoton(const spho& sp) const ; 
    void pointPhoton(const spho& sp); 
    void finalPhoton(const spho& sp); 


    NP* getPho0() const ;   // unordered push_back as they come 
    NP* getPho() const ;    // resized at genstep and slotted in 
    NP* getGS() const ;   // genstep labels from std::vector<sgs>  

    NP* getPhoton() const ; 
    NP* getRecord() const ; 
    NP* getRec() const ; 
    NP* getSeq() const ; 

    NP* makePhoton() const ; 
    NP* makeRecord() const ; 
    NP* makeRec() const ; 
    NP* makeSeq() const ; 


    // SCompProvider methods

    std::string getMeta() const ; 
    NP* getComponent(unsigned comp) const ; 
    NP* getComponent_(unsigned comp) const ; 

    void saveLabels(const char* dir) const ;  // formerly savePho

    void saveGenstep(const char* dir) const ; 
    NP* getGenstep() const ; 
    const NP* getInputPhoton() const ; 
    void setInputPhoton(const NP* p); 

    void gather_components() ; 

    static const char* FALLBACK_DIR ; 
    static const char* DefaultDir() ; 

    // save methods not const as calls gather_components 
    void save() ; 
    void save(const char* base, const char* reldir ); 
    void save(const char* dir); 

    std::string desc() const ; 
    std::string descGS() const ; 
    std::string descFold() const ; 
    std::string descComponent() const ; 
};



