#pragma once
/**
SCurandChunk.h
===============

::

    ~/o/sysrap/tests/SCurandState_test.sh

**/

#if defined(__CUDACC__) || defined(__CUDABE__)
#else

#include "sdirectory.h"
#include "spath.h"
#include "sstr.h"
#include <iomanip>
#include "SYSRAP_API_EXPORT.hh"

#endif

#include "scurandref.h"

struct SYSRAP_API SCurandChunk
{
    typedef unsigned long long ULL ; 
    scurandref ref = {} ; 

#if defined(__CUDACC__) || defined(__CUDABE__)
#else
    std::string desc() const ;
    std::string meta() const ;
    std::string name() const ;
    const char* path(const char* _dir=nullptr) const ;
    bool path_exists(const char* _dir=nullptr) const ; 
    int save( const char* _dir=nullptr ) const ; 

    static constexpr const long STATE_SIZE = 44 ;  
    static constexpr const char* RNGDIR = "${RNGDir:-$HOME/.opticks/rngcache/RNG}" ; 
    static constexpr const ULL M = 1000000 ; 
    static constexpr const char* PREFIX = "SCurandChunk_" ; 
    static constexpr const char* EXT = ".bin" ; 
    static constexpr char DELIM = '_' ;
    static constexpr const long NUM_ELEM = 5 ;  

    static std::string Desc(const SCurandChunk& chunk, const char* _dir=nullptr ); 
    static std::string Desc(const std::vector<SCurandChunk>& chunk, const char* _dir=nullptr ); 
    static int ParseDir( std::vector<SCurandChunk>& chunk, const char* _dir=nullptr );
    static int ParseName( SCurandChunk& chunk, const char* name ); 
    static long ParseNum(const char* num); 

    static std::string FormatIdx(ULL idx);
    static std::string FormatNum(ULL num);
    static std::string FormatMeta(const scurandref& d ); 
    static std::string FormatName(const scurandref& d ); 

    static long NumFromFilesize(const char* name, const char* _dir=nullptr); 
    static bool IsValid(const SCurandChunk& chunk, const char* _dir=nullptr);
    static int CountValid(const std::vector<SCurandChunk>& chunk, const char* _dir=nullptr );
    static scurandref* Find(std::vector<SCurandChunk>& chunk, long idx );
    static ULL NumTotal_SizeCheck(const std::vector<SCurandChunk>& chunk, const std::vector<ULL>& size );
    static ULL NumTotal_InRange(  const std::vector<SCurandChunk>& chunk, ULL i0, ULL i1 ); 

    static int Load( SCurandChunk& chunk, const char* name, ULL q_num=0, const char* _dir=nullptr );


#endif

};

#if defined(__CUDACC__) || defined(__CUDABE__)
#else

inline std::string SCurandChunk::desc() const
{
    std::stringstream ss ; 
    ss << Desc(*this) << "\n" ; 
    std::string str = ss.str() ; 
    return str ;  
}

inline std::string SCurandChunk::meta() const
{
    return FormatMeta(ref); 
}
inline std::string SCurandChunk::name() const
{
    return FormatName(ref); 
}
inline const char* SCurandChunk::path(const char* _dir) const
{
    std::string n = name(); 
    const char* dir = _dir ? _dir : RNGDIR ; 
    return spath::Resolve( dir, n.c_str() );  
}

inline bool SCurandChunk::path_exists(const char* _dir) const
{
    const char* pth = path(_dir) ; 
    return spath::Exists(pth); 
} 

inline std::string SCurandChunk::Desc(const SCurandChunk& chunk, const char* _dir )
{
    bool exists = chunk.path_exists(_dir); 
    std::stringstream ss ; 
    ss << chunk.path(_dir) << " exists " <<  ( exists ? "YES" : "NO " ) ; 
    std::string str = ss.str() ; 
    return str ;  
}

inline std::string SCurandChunk::Desc(const std::vector<SCurandChunk>& chunk, const char* _dir )
{
    int num_chunk = chunk.size(); 
    std::stringstream ss ; 
    ss << "SCurandChunk::Desc\n" ; 
    for(int i=0 ; i < num_chunk ; i++) ss << Desc(chunk[i], _dir) << "\n" ;  
    std::string str = ss.str() ; 
    return str ;  
} 



inline int SCurandChunk::ParseDir(std::vector<SCurandChunk>& chunk, const char* _dir )
{
    const char* dir = spath::Resolve(_dir ? _dir : RNGDIR) ; 
    std::vector<std::string> names ; 
    sdirectory::DirList( names, dir, PREFIX, EXT ); 

    int num_names = names.size(); 
    int count = 0 ; 

    for(int i=0 ; i < num_names ; i++) 
    {
        const std::string& n = names[i] ; 
        SCurandChunk c = {} ; 
        if(SCurandChunk::ParseName(c, n.c_str())==0) 
        {
            ULL chunk_offset = NumTotal_InRange(chunk, 0, chunk.size() ); 
            assert( c.ref.chunk_offset == chunk_offset );
            assert( c.ref.chunk_idx == count ); // chunk files must be in idx order 
            chunk.push_back(c);
            count += 1 ; 
        }
    }
    return 0 ; 
}



inline int SCurandChunk::ParseName( SCurandChunk& chunk, const char* name )
{
    if(name == nullptr ) return 1 ;  
    size_t other = strlen(PREFIX)+strlen(EXT) ; 
    if( strlen(name) <= other ) return 2 ; 

    std::string n = name ; 
    std::string meta = n.substr( strlen(PREFIX), strlen(name) - other ); 

    std::vector<std::string> elem ; 
    sstr::Split(meta.c_str(), DELIM, elem ); 

    unsigned num_elem = elem.size(); 
    if( num_elem != NUM_ELEM )  return 3 ; 

    chunk.ref.chunk_idx    = std::atoll(elem[0].c_str()) ; 
    chunk.ref.chunk_offset = ParseNum(elem[1].c_str()) ;
    chunk.ref.num          = ParseNum(elem[2].c_str()) ; 
    chunk.ref.seed         = std::atoll(elem[3].c_str()) ; 
    chunk.ref.offset       = std::atoll(elem[4].c_str()) ; 
    chunk.ref.states       = nullptr ; 

    std::cout << "SCurandChunk::ParseName " << std::setw(30) << n << " : [" << meta << "][" << chunk.name() << "]\n" ; 
    return 0 ; 
}



inline long SCurandChunk::ParseNum(const char* num)
{
    char* n = strdup(num); 
    char last = n[strlen(n)-1] ; 
    ULL scale = last == 'M' ? M : 1 ; 
    if(scale > 1) n[strlen(n)-1] = '\0' ; 
    ULL value = scale*std::atoll(num) ; 
    return value ; 
}



inline std::string SCurandChunk::FormatMeta(const scurandref& d)
{
    std::stringstream ss ; 
    ss 
       << FormatIdx(d.chunk_idx) 
       << DELIM
       << FormatNum(d.chunk_offset) 
       << DELIM
       << FormatNum(d.num)
       << DELIM
       << d.seed
       << DELIM
       << d.offset
       ; 
    std::string str = ss.str(); 
    return str ;   
}
inline std::string SCurandChunk::FormatName(const scurandref& d)
{
    std::stringstream ss ; 
    ss << PREFIX << FormatMeta(d) << EXT ; 
    std::string str = ss.str(); 
    return str ;   
}
 
inline std::string SCurandChunk::FormatIdx(ULL idx)
{
    std::stringstream ss; 
    ss << std::setw(4) << std::setfill('0') << idx ;
    std::string str = ss.str(); 
    return str ;   
}
inline std::string SCurandChunk::FormatNum(ULL num)
{
    ULL scale = M  ; 

    bool intmul = num % scale == 0 ; 
    if(!intmul) std::cerr
         << "SCurandChunk::FormatNum"
         << " num [" << num << "]"
         << " intmul " << ( intmul ? "YES" : "NO " )
         << "\n"
         ; 
    assert( intmul && "integer multiples of 1000000 are required" ); 
    ULL value = num/scale ; 

    std::stringstream ss; 
    ss << std::setw(4) << std::setfill('0') << value << 'M' ; 
    std::string str = ss.str(); 
    return str ;   
}

inline long SCurandChunk::NumFromFilesize(const char* name, const char* _dir)
{
    const char* dir = _dir ? _dir : RNGDIR ; 
    ULL file_size = spath::Filesize(dir, name); 

    bool expected_file_size = file_size % STATE_SIZE == 0 ; 
    ULL states = file_size/STATE_SIZE ;

    if(0) std::cerr
        << "SCurandChunk::NumFromFilesize"
        << " dir " << ( dir ? dir : "-" )
        << " name " << ( name ? name : "-" )
        << " file_size " << file_size
        << " STATE_SIZE " << STATE_SIZE
        << " states " << states
        << " expected_file_size " << ( expected_file_size ? "YES" : "NO" )
        << "\n"
        ;

    assert( expected_file_size );
    return states ; 
}

inline bool SCurandChunk::IsValid(const SCurandChunk& chunk, const char* _dir )
{
    bool exists = chunk.path_exists(_dir) ;  
    if(!exists) return false ; 

    std::string n = chunk.name(); 


    ULL chunk_num = chunk.ref.num ; 
    ULL file_num = NumFromFilesize(n.c_str(), _dir) ;  
    bool valid = chunk_num == file_num ; 

    if(!valid) std::cerr   
        << "SCurandChunk::IsValid"
        << " chunk file exists [" << n << "]"
        << " but filesize does not match name metadata "
        << " chunk_num " << chunk_num
        << " file_num " << file_num
        << " valid " << ( valid ? "YES" : "NO " )
        << "\n"
        ;
    return valid ; 
}

inline int SCurandChunk::CountValid(const std::vector<SCurandChunk>& chunk, const char* _dir )
{
    int num_chunk = chunk.size(); 
    int count = 0 ; 
    for(int i=0 ; i < num_chunk ; i++)
    {
        const SCurandChunk& c = chunk[i];     
        bool valid = IsValid(c, _dir); 
        if(!valid) continue ;  
        count += 1 ;          
    }
    return count ; 
}

inline scurandref* SCurandChunk::Find(std::vector<SCurandChunk>& chunk, long q_idx )
{
    int num_chunk = chunk.size(); 
    int count = 0 ; 
    scurandref* p = nullptr ; 
    for(int i=0 ; i < num_chunk ; i++)
    {
        SCurandChunk& c = chunk[i] ;     
        if( c.ref.chunk_idx == q_idx ) 
        {
            p = &(c.ref) ;  
            count += 1 ; 
        }
    }
    assert( count == 0 || count == 1 ); 
    return count == 1 ? p : nullptr ; 
}


inline unsigned long long SCurandChunk::NumTotal_SizeCheck(const std::vector<SCurandChunk>& chunk, const std::vector<ULL>& size )
{
    assert( chunk.size() == size.size() ) ;
    ULL tot = 0 ; 
    ULL num_chunk = chunk.size(); 
    for(ULL i=0 ; i < num_chunk ; i++)
    {
        const scurandref& d = chunk[i].ref ;     
        assert( d.chunk_idx == i );  
        assert( d.num == size[i] );  
        tot += d.num ; 
    }
    return tot ; 
}

inline unsigned long long SCurandChunk::NumTotal_InRange( const std::vector<SCurandChunk>& chunk, ULL i0, ULL i1 )
{
    ULL num_chunk = chunk.size(); 
    assert( i0 <= num_chunk ); 
    assert( i1 <= num_chunk ); 

    ULL num_tot = 0ull ; 
    for(ULL i=i0 ; i < i1 ; i++) 
    {
        const scurandref& d = chunk[i].ref ;     
        num_tot += d.num ; 
    } 
    return num_tot ; 
}




/**
SCurandChunk::Load
---------------------

1. parse the name storing metadata into the chunk struct
2. allocate chunk.states and load from file

A partial load is done for (q_num > 0 && q_num < file_num)

**/


inline int SCurandChunk::Load( SCurandChunk& chunk, const char* name, ULL q_num, const char* _dir )
{
    int prc = ParseName(chunk, name); 
    if(prc > 0) return prc ; 
    
    const char* dir = _dir ? _dir : RNGDIR ; 
    const char* p = spath::Resolve(dir, name); 
    std::cout << "SCurandChunk::load path " << ( p ? p : "-" )  << "\n" ; 

    FILE *fp = fopen(p,"rb");

    std::cout << "SCurandChunk::Load fp  " << ( fp ? "YES" : "NO " )  << "\n" ; 

    bool failed = fp == nullptr ; 
    if(failed) std::cerr << "SCurandChunk::Load unable to open file [" << p << "]" << std::endl ; 
    if(failed) return 1 ; 

    fseek(fp, 0L, SEEK_END);
    long file_size = ftell(fp);
    rewind(fp);

    std::cout << "SCurandChunk::load file_size " << file_size  << "\n" ; 
    assert( file_size % STATE_SIZE == 0 );  

    long file_num = file_size/STATE_SIZE;   // NB  STATE_SIZE not same as type_size
    long name_num = chunk.ref.num ;    // from ParseName

    bool consistent_num = file_num == name_num ; 
    long read_num = ( q_num > 0 && q_num < file_num ) ? q_num : file_num ; 
    chunk.ref.num = read_num ; 

    std::cerr
        << "SCurandChunk::Load"
        << " path " << p 
        << " q_num " << FormatNum(q_num)
        << " name_num " << FormatNum(name_num) << "(from parsing filename) "
        << " file_num " << FormatNum(file_num) << "(from file_size/STATE_SIZE) "
        << " consistent_num " << ( consistent_num ? "YES" : "NO " )
        << " read_num " << FormatNum(read_num)
        << "\n"
        ; 

    chunk.ref.states = (curandState*)malloc(sizeof(curandState)*chunk.ref.num);

    for(long i = 0 ; i < chunk.ref.num ; ++i )
    {   
        curandState& rng = chunk.ref.states[i] ;
        fread(&rng.d,                     sizeof(unsigned int),1,fp);   //  1
        fread(&rng.v,                     sizeof(unsigned int),5,fp);   //  5 
        fread(&rng.boxmuller_flag,        sizeof(int)         ,1,fp);   //  1 
        fread(&rng.boxmuller_flag_double, sizeof(int)         ,1,fp);   //  1
        fread(&rng.boxmuller_extra,       sizeof(float)       ,1,fp);   //  1
        fread(&rng.boxmuller_extra_double,sizeof(double)      ,1,fp);   //  2    11*4 = 44 
    }   
    fclose(fp);

    return 0 ; 
}



inline int SCurandChunk::save( const char* _dir ) const
{
    const char* p = path(_dir); 
    bool exists = spath::Exists(p); 
    std::cerr 
        << "SCurandChunk::save"
        << " p " << ( p ? p : "-" )
        << " exists " << ( exists ? "YES" : "NO " )
        << "\n"
        ;  

    if(exists) return 1 ; 

    sdirectory::MakeDirsForFile(p);
    FILE *fp = fopen(p,"wb");
    bool open_fail = fp == nullptr ; 

    if(open_fail) std::cerr 
        << "SCurandChunk::Save"
        << " FAILED to open file for writing" 
        << ( p ? p : "-" ) 
        << "\n" 
        ;

    if( open_fail ) return 3 ; 

    for(ULL i = 0 ; i < ref.num ; ++i )
    {  
        curandState& rng = ref.states[i] ;
        fwrite(&rng.d,                     sizeof(unsigned int),1,fp);
        fwrite(&rng.v,                     sizeof(unsigned int),5,fp);
        fwrite(&rng.boxmuller_flag,        sizeof(int)         ,1,fp);
        fwrite(&rng.boxmuller_flag_double, sizeof(int)         ,1,fp);
        fwrite(&rng.boxmuller_extra,       sizeof(float)       ,1,fp);
        fwrite(&rng.boxmuller_extra_double,sizeof(double)      ,1,fp);
    }  
    fclose(fp);
    return 0 ;
}

#endif

