#include "GFlags.hh"

#include <map>
#include <vector>
#include <string>

#include "Index.hpp"
#include "GAttrSeq.hh"

#include "regexsearch.hh"
#include "NLog.hpp"


//const char* GFlags::ENUM_HEADER_PATH = "$ENV_HOME/graphics/optixrap/cu/photon.h" ;
const char* GFlags::ENUM_HEADER_PATH = "$ENV_HOME/opticks/OpticksPhoton.h" ;

void GFlags::init(const char* path)
{
    m_index = parseFlags(path);
    
    m_aindex = new GAttrSeq(m_cache, "GFlags");
    m_aindex->loadPrefs(); // color, abbrev and order 

    m_aindex->setSequence(m_index);
}

void GFlags::save(const char* idpath)
{
    m_index->setExt(".ini"); 
    m_index->save(idpath);    
}


Index* GFlags::parseFlags(const char* path)
{
    typedef std::pair<unsigned int, std::string>  upair_t ;
    typedef std::vector<upair_t>                  upairs_t ;
    upairs_t ups ;
    enum_regexsearch( ups, path ); 

    Index* index = new Index("GFlags");
    for(unsigned int i=0 ; i < ups.size() ; i++)
    {
        upair_t p = ups[i];
        unsigned int mask = p.first ;
        unsigned int bitpos = ffs(mask);  // first set bit, 1-based bit position
        assert( mask == (1 << (bitpos-1)));
        index->add( p.second.c_str(), bitpos );
    }
    return index ; 
}


std::map<unsigned int, std::string> GFlags::getNamesMap()
{
    return m_aindex->getNamesMap() ; 
}

