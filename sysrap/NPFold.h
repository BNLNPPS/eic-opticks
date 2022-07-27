#pragma once
/**
NPFold.h : collecting and persisting NP arrays keyed by relative paths
========================================================================

This does not use other sysrap headers, other than NP, NPU 
as it will likely be incorporated into np/NPU once matured. 

There are two load/save modes:

1. with index txt file "NPFold_index.txt" : the default mode
   in which the ordering of the keys are preserved
 
2. without index txt file : the ordering of keys/arrays 
   is not preserved, it will be the fts filesystem traversal order 
   This mode allows all .npy from within any directory tree to 
   be loaded into the NPFold instance.   

**/

#include <string>
#include <algorithm> 
#include <iterator> 
#include <vector> 
#include <cstdlib>
#include <cstdio>
#include <sys/types.h>
#include <fts.h>
#include <cstring>
#include <errno.h>
#include <sstream>
#include <iomanip>

#include "NP.hh"

struct NPFold 
{
    static constexpr const char* EXT = ".npy" ; 
    static constexpr const char* INDEX = "NPFold_index.txt" ; 
    static constexpr const char* META  = "NPFold_meta.txt" ; 

    static bool IsNPY(const char* k); 
    static std::string FormKey(const char* k); 
    static NPFold* Load(const char* base); 
    static NPFold* Load(const char* base, const char* rel); 
    static int Compare(const NPFold* a, const NPFold* b, bool dump ); 

    static int Compare(const FTSENT** one, const FTSENT** two); 
    static void Indent(int i); 

    std::vector<std::string> kk ; 
    std::vector<const NP*>   aa ; 
    std::string meta ; 
    const char* savedir ; 
    const char* loaddir ; 

    NPFold(); 

    void check() const ; 
    void add(const char* k, const NP* a); 
    void set(const char* k, const NP* a); 

    int num_items() const ; 
    const char* get_key(unsigned idx) const ; 
    const NP*   get_array(unsigned idx) const ; 

    int find(const char* k) const ; 
    bool has_key(const char* k) const ; 

    const NP* get(const char* k) const ; 
    int   get_num(const char* k) const ; 

    void save(const char* base) ; 
    void save(const char* base, const char* rel) ; 
    int load(const char* base) ; 
    int load(const char* base, const char* rel) ; 

    int  load_fts(const char* base) ; 
    int  load_index(const char* base) ; 
    void load_array(const char* base, const char* relp); 

    std::string desc() const ; 
    std::string brief() const ; 
}; 



inline NPFold* NPFold::Load(const char* base)
{
    NPFold* nf = new NPFold ; 
    nf->load(base); 
    return nf ;  
}

inline NPFold* NPFold::Load(const char* base, const char* rel)
{
    NPFold* nf = new NPFold ; 
    nf->load(base, rel); 
    return nf ;  
}

inline int NPFold::Compare(const NPFold* a, const NPFold* b, bool dump)
{
    int na = a->num_items(); 
    int nb = b->num_items(); 
    bool item_match = na == nb ;  

    if(dump) std::cout << " na " << na << " nb " << nb << " item_match " << item_match << std::endl ; 
    if(!item_match ) return -1 ; 

    int mismatch = 0 ; 
    for(int i=0 ; i < na ; i++)
    {
        const char* a_key = a->get_key(i); 
        const char* b_key = b->get_key(i); 
        const NP*   a_arr = a->get_array(i); 
        const NP*   b_arr = b->get_array(i); 

        bool key_match = strcmp(a_key, b_key) == 0 ; 
        if(dump) std::cout 
            << " a_key " << std::setw(40) << a_key 
            << " b_key " << std::setw(40) << b_key 
            << " key_match " << key_match 
            << std::endl 
            ; 
        if(!key_match) mismatch += 1  ; 

        bool arr_match = NP::Memcmp(a_arr, b_arr) == 0 ; 
        if(dump) std::cout 
            << " a_arr " << std::setw(40) << a_arr->sstr() 
            << " b_arr " << std::setw(40) << b_arr->sstr() 
            << " arr_match " << arr_match 
            << std::endl
            ; 

        if(!arr_match) mismatch += 1 ; 
    }

    if(dump) std::cout << "NPFold::Compare mismatch " << mismatch << std::endl ; 
    return mismatch ; 
}


inline NPFold::NPFold()
    :
    savedir(nullptr),
    loaddir(nullptr)
{
}


inline void NPFold::check() const
{
    assert( kk.size() == aa.size() ); 
}

inline bool NPFold::IsNPY(const char* k)
{
    return strlen(k) > strlen(EXT) && strcmp( k + strlen(k) - strlen(EXT), EXT ) == 0 ; 
}

inline std::string NPFold::FormKey(const char* k)
{
    std::stringstream ss ; 
    ss << k ; 
    if(!IsNPY(k)) ss << EXT ; 
    std::string s = ss.str(); 
    return s ; 
}


/**
NPFold::add
------------

If added keys do not end with the EXT ".npy" then the EXT is added prior to collection. 

**/

inline void NPFold::add(const char* k, const NP* a) 
{
    std::string key = FormKey(k); 
    kk.push_back(key); 
    aa.push_back(a); 
}

inline void NPFold::set(const char* k, const NP* a) 
{
    int idx = find(k); 
    if(idx == -1)  
    {
        add(k, a); 
    }
    else
    {
        aa[idx] = a ;  // HMM: are leaking the old one  
    }
}


inline int NPFold::num_items() const 
{
    check(); 
    return kk.size(); 
}
inline const char* NPFold::get_key(unsigned idx) const 
{
    return idx < kk.size() ? kk[idx].c_str() : nullptr ;
}
inline const NP* NPFold::get_array(unsigned idx) const 
{
    return idx < aa.size() ? aa[idx] : nullptr ;
}

/**
NPFold::find
-------------

If the key *k* does not ext with EXT ".npy" then that is added before searching.

**/
inline int NPFold::find(const char* k) const
{
    std::string key = FormKey(k); 
    size_t idx = std::distance( kk.begin(), std::find( kk.begin(), kk.end(), key.c_str() )) ; 
    return idx < kk.size() ? idx : -1 ; 
}

inline bool NPFold::has_key(const char* k) const 
{
    int idx = find(k); 
    return idx > -1 ; 
}

inline const NP* NPFold::get(const char* k) const 
{
    int idx = find(k) ; 
    return idx > -1 ? aa[idx] : nullptr ; 
}

inline int NPFold::get_num(const char* k) const 
{
    const NP* a = get(k) ; 
    return a == nullptr ? -1 : a->shape[0] ;   
}


inline void NPFold::save(const char* base)  // not const as sets savedir
{
    savedir = strdup(base); 

    //std::cout << "NPFold::save " << base << std::endl ; 
    NP::WriteNames(base, INDEX, kk );  
    for(unsigned i=0 ; i < kk.size() ; i++) 
    {
        const char* k = kk[i].c_str() ; 
        const NP* a = aa[i] ; 
        if( a == nullptr )
        {
            std::cout << " base " << base << " k " << k << " ERROR MISSING ARRAY FOR KEY " << std::endl ;   
        }
        else
        { 
            a->save(base, k );  
        }
    }
    // this motivated adding directory creation to NP::save 

   if(!meta.empty()) NP::WriteString(base, META, meta.c_str() );  
}

inline void NPFold::save(const char* base_, const char* rel) // not const as sets savedir
{
    std::string base = NP::form_path(base_, rel); 
    save(base.c_str()); 
}







inline int NPFold::Compare(const FTSENT** one, const FTSENT** two)
{
    return (strcmp((*one)->fts_name, (*two)->fts_name));
}
inline void NPFold::Indent(int i)
{ 
    for(; i > 0; i--) printf("    ");
}

inline void NPFold::load_array(const char* base, const char* relp)
{
    if(IsNPY(relp)) add(relp,  NP::Load(base, relp) ) ; 
}
 
inline int NPFold::load_fts(const char* base_) 
{
    char* base = const_cast<char*>(base_);  
    char* basepath[2] {base, nullptr};

    FTS* fs = fts_open(basepath,FTS_COMFOLLOW|FTS_NOCHDIR,&Compare);
    if(fs == nullptr) return 1 ; 

    FTSENT* node = nullptr ;
    while((node = fts_read(fs)) != nullptr)
    {   
        switch (node->fts_info) 
        {   
            case FTS_D :
                break;
            case FTS_F :
            case FTS_SL:
                {
                    char* relp = node->fts_path+strlen(base)+1 ;
                    load_array(base, relp) ; 
                }
                break;
            default:
                break;
        }   
    }   
    fts_close(fs);
    return 0 ; 
}

inline int NPFold::load_index(const char* base) 
{
    std::vector<std::string> keys ; 
    NP::ReadNames(base, INDEX, keys );  
    for(unsigned i=0 ; i < keys.size() ; i++) load_array(base, keys[i].c_str() );  
    return 0 ; 
}

inline int NPFold::load(const char* base) 
{
    loaddir = strdup(base); 
    bool has_meta = NP::Exists(base, META) ; 
    if(has_meta) meta = NP::ReadString( base, META ); 

    bool has_index = NP::Exists(base, INDEX) ; 
    return has_index ? load_index(base) : load_fts(base) ; 
}
inline int NPFold::load(const char* base_, const char* rel) 
{
    std::string base = NP::form_path(base_, rel); 
    return load(base.c_str()); 
}






inline std::string NPFold::desc() const  
{
    std::stringstream ss ; 
    ss << "NPFold::desc"  << std::endl ; 
    for(unsigned i=0 ; i < kk.size() ; i++) 
    {
        const std::string& k = kk[i] ; 
        const NP* a = aa[i] ; 
        ss << std::setw(40) << k << " : " << ( a ? a->sstr() : "-" ) << std::endl ;  
    }
    std::string s = ss.str(); 
    return s ; 
}

inline std::string NPFold::brief() const 
{
    std::stringstream ss ; 
    if(loaddir) ss << " loaddir:" << loaddir ; 
    if(savedir) ss << " savedir:" << savedir ; 
    std::string s = ss.str(); 
    return s ; 
}

