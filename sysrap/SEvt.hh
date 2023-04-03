#pragma once
/**
SEvt.hh
=========

NB : THERE IS ALMOST ALWAYS ONLY ONE SEvt INSTANCE NO MATTER HOW MANY G4Event ARE HANDLED

* HAVING MORE THAN ONE SEvt INSTANCE CAN CAUSE PERPLEXING BUGS 


TODO : lifecycle leak checking 

Q: Where is this instanciated canonically ?
A: So far in G4CXOpticks::setGeometry and in the mains of lower level tests

gather vs get
-----------------

Note the distinction in the names of accessors:

*gather*
   resource intensive action done few times, that likely allocates memory,
   in the case of sibling class QEvent *gather* mostly includes downloading from device 
   Care is needed in memory management of returned arrays to avoid leaking.  

*get*
   cheap action, done as many times as needed, returning a pointer to 
   already allocated memory that is separately managed, eg within NPFold


header-only INSTANCE problem 
------------------------------

Attempting to do this header only gives duplicate symbol for the SEvt::INSTANCE.
It seems c++17 would allow "static inline SEvt* INSTANCE"  but c++17 
is problematic on laptop, so use separate header and implementation
for simplicity. 

It is possible with c++11 but is non-intuitive

* https://stackoverflow.com/questions/11709859/how-to-have-static-data-members-in-a-header-only-library

HMM : Alt possibility for merging sgs label into genstep header
------------------------------------------------------------------

gs vector of sgs provides summary of the full genstep, 
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
#include "sphit.h"
#include "sstate.h"
#include "srec.h"
#include "sseq.h"
#include "stag.h"
#include "sevent.h"
#include "sctx.h"

#include "squad.h"
#include "sframe.h"

#include "sgs.h"
#include "SComp.h"
#include "SRandom.h"

//struct SCF ; 
struct sphoton_selector ; 
struct sdebug ; 
struct NP ; 
struct NPFold ; 
struct SGeo ; 

#include "SYSRAP_API_EXPORT.hh"

struct SYSRAP_API SEvt : public SCompProvider
{

    int cfgrc ; 
    int index ; 
    const char* reldir ;  // HMM: perhaps static RELDIR fallback ?

    sphoton_selector* selector ; 
    sevent* evt ; 
    sdebug* dbg ; 
    std::string meta ; 

    NP* input_photon ; 
    NP* input_photon_transformed ; 
    NP* g4state ;    // populated by U4Engine::SaveStatus

    const SRandom*        random ; 
    const SCompProvider*  provider ; 
    NPFold*               fold ; 
    const SGeo*           cf ; 

    bool              hostside_running_resize_done ; // only ever becomes true for non-GPU running 
    bool              gather_done ; 
    bool              is_loaded ; 
    bool              is_loadfail ; 

    sframe            frame ;

    std::vector<unsigned> comp ; 
    std::vector<quad6> genstep ; 
    std::vector<sgs>   gs ; 

    unsigned           numphoton_collected ;    // avoid looping over all gensteps for every genstep
    unsigned           numphoton_genstep_max ;  // maximum photons in a genstep since last SEvt::clear

    std::vector<spho>  pho0 ;  // unordered push_back as they come 
    std::vector<spho>  pho ;   // spho are label structs holding 4*int 


    std::vector<int>     slot ; 
    std::vector<sphoton> photon ; 
    std::vector<sphoton> record ; 
    std::vector<srec>    rec ; 
    std::vector<sseq>    seq ; 
    std::vector<quad2>   prd ; 
    std::vector<stag>    tag ; 
    std::vector<sflat>   flat ; 
    std::vector<quad4>   simtrace ; 
    std::vector<quad4>   aux ; 

    // current_* are saved into the vectors on calling SEvt::pointPhoton 
    spho    current_pho = {} ; 
    quad2   current_prd = {} ; 
    sctx    current_ctx = {};  


    static constexpr const unsigned UNDEF = ~0u ; 
    static NP* UU ;  
    static NP* UU_BURN ;  
    static const plog::Severity LEVEL ; 
    static const int PIDX ; 
    static const int GIDX ; 
    static const int MISSING_INDEX ; 
    static const char*  DEFAULT_RELDIR ; 

    static SEvt* INSTANCE ; 
    static SEvt* Get() ; 
    static SEvt* Create() ; 
    static SEvt* CreateOrReuse() ; 

    static SEvt* HighLevelCreate(); // Create with bells-and-whistles needed by eg u4/tests/U4SimulateTest.cc

    static bool Exists(); 

    static void Check(); 
    static void AddTag(unsigned stack, float u ); 
    static int  GetTagSlot(); 
    static sgs AddGenstep(const quad6& q); 
    static sgs AddGenstep(const NP* a); 
    static void AddCarrierGenstep(); 
    static void AddTorchGenstep(); 

    static void Clear(); 
    static SEvt* Load(const char* rel=nullptr); 
    static void Save() ; 
    static void Save(const char* bas, const char* rel ); 
    static void Save(const char* dir); 
    static void SaveGenstepLabels(const char* dir, const char* name="gsl.npy"); 

    static void SetIndex(int index); 
    static void UnsetIndex(); 
    static int  GetIndex(); 

    static void SetReldir(const char* reldir); 
    static const char* GetReldir(); 

    static int GetNumPhotonCollected(); 
    static int GetNumPhotonGenstepMax(); 
    static int GetNumPhotonFromGenstep(); 
    static int GetNumGenstepFromGenstep(); 
    static int GetNumHit() ; 

    static NP* GatherGenstep(); 
    static NP* GetInputPhoton(); 
    static void SetInputPhoton(NP* ip); 
    static bool HasInputPhoton(); 

 
    SEvt(); 
    void init(); 

    static const char* GetSaveDir() ; 
    const char* getSaveDir() const ; 
    const char* getLoadDir() const ; 
    int getTotalItems() const ; 

    const char* getSearchCFBase() const ; 


    static const char* INPUT_PHOTON_DIR ; 
    static NP* LoadInputPhoton(); 
    static NP* LoadInputPhoton(const char* ip); 
    void initInputPhoton(); 
    void setInputPhoton(NP* p); 

    NP* getInputPhoton_() const ; 
    NP* getInputPhoton() const ;    // returns input_photon_transformed when exists 
    bool hasInputPhoton() const ; 
    bool hasInputPhotonTransformed() const ; 


    void initG4State() ; 
    NP* makeG4State() const ; 
    void setG4State(NP* state) ; 
    NP* gatherG4State() const ;
    const NP* getG4State() const ;

    static const bool setFrame_WIDE_INPUT_PHOTON ; 
    void setFrame(const sframe& fr ); 
    void setFrame_HostsideSimtrace() ; 
    void setGeo(const SGeo* cf); 
    void setFrame(unsigned ins_idx);  // requires setGeo to access the frame from SGeo
    static SEvt* CreateSimtraceEvent(); 


    const char* getFrameId() const ; 
    const NP*   getFrameArray() const ; 
    static const char* GetFrameId() ;
    static const NP*   GetFrameArray() ;


    static quad6 MakeInputPhotonGenstep(const NP* input_photon, const sframe& fr ); 


    void setCompProvider(const SCompProvider* provider); 
    bool isSelfProvider() const ; 
    std::string descProvider() const ; 


    void setNumPhoton(unsigned num_photon); 
    void setNumSimtrace(unsigned num_simtrace);


    void hostside_running_resize(); 
    void hostside_running_resize_(); 

    NP* gatherDomain() const ; 

    void clear_() ; 
    void clear() ; 
    void clear_partial(const char* keep_keylist, char delim=','); 


    void setIndex(int index_) ;  
    void unsetIndex() ;  
    int getIndex() const ; 

    void setReldir(const char* reldir_) ;  
    const char* getReldir() const ; 

    unsigned getNumGenstepFromGenstep() const ; // number of collected gensteps from size of collected gensteps vector
    unsigned getNumPhotonFromGenstep() const ;  // total photons since last clear obtained by looping over collected gensteps
    unsigned getNumPhotonCollected() const ;    // total collected photons since last clear
    unsigned getNumPhotonGenstepMax() const ;   // max photon in genstep since last clear

    static constexpr const unsigned G4_INDEX_OFFSET = 1000000 ; 
    sgs addGenstep(const quad6& q) ; 
    sgs addGenstep(const NP* a) ; 

    const sgs& get_gs(const spho& label) const ; // lookup genstep label from photon label  
    unsigned get_genflag(const spho& label) const ; 

    void beginPhoton(const spho& sp); 
    unsigned getCurrentPhotonIdx() const ; 

    void resumePhoton(const spho& sp);  // FastSim->SlowSim resume 
    void rjoinPhoton(const spho& sp);   // reemission rjoin
    void rjoin_resumePhoton(const spho& label); // reemission rjoin AND FastSim->SlowSim resume 


    void rjoinRecordCheck(const sphoton& rj, const sphoton& ph  ) const ; 
    static void ComparePhotonDump(const sphoton& a, const sphoton& b ); 
    void rjoinPhotonCheck(const sphoton& ph) const ; 
    void rjoinSeqCheck(unsigned seq_flag) const ; 

    void checkPhotonLineage(const spho& sp) const ; 
    void pointPhoton(const spho& sp); 
    void finalPhoton(const spho& sp); 

    static bool PIDX_ENABLED ;
    void addTag(unsigned tag, float u); 
    int getTagSlot() const ; 


    NP* gatherPho0() const ;   // unordered push_back as they come 
    NP* gatherPho() const ;    // resized at genstep and slotted in 
    NP* gatherGS() const ;   // genstep labels from std::vector<sgs>  

    NP* gatherGenstep() const ; 
    NP* gatherPhoton() const ; 
    NP* gatherRecord() const ; 
    NP* gatherRec() const ; 
    NP* gatherAux() const ; 
    NP* gatherSeq() const ; 
    NP* gatherPrd() const ; 
    NP* gatherTag() const ; 
    NP* gatherFlat() const ; 

    NP* gatherSeed() const ; 
    NP* gatherHit() const ; 
    NP* gatherSimtrace() const ; 

    static const char* ENVMETA ; 
    static void AddEnvMeta(NP* a, bool dump=false ); 

    NP* makePhoton() const ; 
    NP* makeRecord() const ; 
    NP* makeRec() const ; 
    NP* makeAux() const ; 
    NP* makeSeq() const ; 
    NP* makePrd() const ; 
    NP* makeTag() const ; 
    NP* makeFlat() const ; 
    NP* makeSimtrace() const ; 



    // SCompProvider methods

    std::string getMeta() const ; 
    NP* gatherComponent(unsigned comp) const ; 
    NP* gatherComponent_(unsigned comp) const ; 

    void saveLabels(const char* dir) const ;  // formerly savePho
    void saveFrame(const char* dir_) const ; 

    void saveGenstep(const char* dir) const ; 
    void saveGenstepLabels(const char* dir, const char* name="gsl.npy") const ; 


    void gather() ;  // with on device running this downloads


    // add extra metadata arrays to be saved within SEvt fold 
    static void AddArray(const char* k, const NP* a ); 
    void add_array( const char* k, const NP* a ); 


    // save methods not const as calls gather
    void save() ; 
    int  load() ; 
    void save(const char* base, const char* reldir1, const char* reldir2 ); 
    void save(const char* base, const char* reldir ); 
    static const char* DefaultDir() ; 

    const char* getOutputDir(const char* base_=nullptr) const ; 
    std::string descSaveDir(const char* dir_) const ; 
    void save(const char* dir); 
    int  load(const char* dir); 

    static std::string Brief() ; 
    std::string brief() const ; 
    std::string desc() const ; 
    std::string descGS() const ; 
    std::string descDir() const ; 
    std::string descFold() const ; 
    std::string descComponent() const ; 
    std::string descComp() const ; 


    const NP* getPhoton() const ; 
    const NP* getHit() const ; 
    const NP* getAux() const ; 

    unsigned getNumPhoton() const ; 
    unsigned getNumHit() const ; 

    void getPhoton(sphoton& p, unsigned idx) const ; 
    void getHit(   sphoton& p, unsigned idx) const ; 

    void getLocalPhoton(sphoton& p, unsigned idx) const ; 
    void getLocalHit(   sphit& ht, sphoton& p, unsigned idx) const ; 
    void getPhotonFrame( sframe& fr, const sphoton& p ) const ; 

    std::string descNum() const ; 
    std::string descPhoton(unsigned max_print=10) const ; 
    std::string descLocalPhoton(unsigned max_print=10) const ; 
    std::string descFramePhoton(unsigned max_print=10) const ; 
    std::string descFull(unsigned max_print=10) const ; 

    void getFramePhoton(sphoton& p, unsigned idx) const ; 
    void getFrameHit(   sphoton& p, unsigned idx) const ; 
};

