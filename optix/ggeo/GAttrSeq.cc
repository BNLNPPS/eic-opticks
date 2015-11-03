#include "GAttrSeq.hh"

// npy-
#include "Index.hpp"
#include "stringutil.hpp"

#include "GItemList.hh"
#include "GCache.hh"
#include "GColors.hh"

#include <sstream>
#include <iostream>
#include <iomanip>
#include <boost/algorithm/string.hpp>

#include "NLog.hpp"

//#include <boost/log/trivial.hpp>
//#define LOG BOOST_LOG_TRIVIAL
// trace/debug/info/warning/error/fatal


unsigned int GAttrSeq::ERROR_COLOR = 0xAAAAAA ; 


void GAttrSeq::loadPrefs()
{
    if(m_cache->loadPreference(m_color, m_type, "color.json"))
        LOG(info) << "GAttrSeq::loadPrefs color " << m_type ;

    if(m_cache->loadPreference(m_abbrev, m_type, "abbrev.json"))
        LOG(info) << "GAttrSeq::loadPrefs abbrev " << m_type ;

    if(m_cache->loadPreference(m_order, m_type, "order.json"))
        LOG(info) << "GAttrSeq::loadPrefs order " << m_type ;
}


void GAttrSeq::setSequence(NSequence* seq)
{
    m_sequence = seq ; 
}

const char* GAttrSeq::getColorName(const char* key)
{
    return m_color.count(key) == 1 ? m_color[key].c_str() : NULL ; 
}

unsigned int GAttrSeq::getColorCode(const char* key )
{
    const char*  colorname =  getColorName(key) ;
    GColors* palette = m_cache->getColors();
    unsigned int colorcode  = palette->getCode(colorname, 0xFFFFFF) ; 
    return colorcode ; 
}

std::vector<unsigned int>& GAttrSeq::getColorCodes()
{
    if(m_sequence && m_color_codes.size() == 0)
    {
        unsigned int ni = m_sequence->getNumKeys();
        for(unsigned int i=0 ; i < ni ; i++)
        {
            const char* key = m_sequence->getKey(i);
            unsigned int code = key ? getColorCode(key) : ERROR_COLOR ;
            m_color_codes.push_back(code);
        }         
    }
    return m_color_codes ; 
}

std::vector<std::string>& GAttrSeq::getLabels()
{
    if(m_sequence && m_labels.size() == 0)
    {
        unsigned int ni = m_sequence->getNumKeys();
        for(unsigned int i=0 ; i < ni ; i++)
        {
            const char* key = m_sequence->getKey(i);
            const char*  colorname = key ? getColorName(key) : NULL ;
            unsigned int colorcode = key ? getColorCode(key) : ERROR_COLOR ;

            std::stringstream ss ;    

            // default label 
            ss << std::setw(3)  << i 
               << std::setw(30) << ( key ? key : "NULL" )
               << std::setw(10) << std::hex << colorcode << std::dec
               << std::setw(15) << ( colorname ? colorname : "" )
               ;

            m_labels.push_back(ss.str());
        }     
    }
    return m_labels ; 
}


std::string GAttrSeq::getAbbr(const char* key)
{
    return m_abbrev.count(key) == 1 ? m_abbrev[key] : key ;  // copying key into string
}


void GAttrSeq::dump(const char* keys, const char* msg)
{
    if(!m_sequence) return ; 
    LOG(info) << msg << " " << ( keys ? keys : "-" ) ; 

    if(keys)
    {
        typedef std::vector<std::string> VS ; 
        VS elem ; 
        boost::split(elem, keys, boost::is_any_of(","));
        for(VS::const_iterator it=elem.begin() ; it != elem.end() ; it++)
             dumpKey(it->c_str()); 
    }
    else
    {
        unsigned int ni = m_sequence->getNumKeys();
        for(unsigned int i=0 ; i < ni ; i++)
        {
            const char* key = m_sequence->getKey(i);
            dumpKey(key);
        }
    }
}

void GAttrSeq::dumpKey(const char* key)
{
    if(key == NULL)
    {
        LOG(warning) << "GAttrSeq::dump NULL key " ;
        return ;   
    }
 

    unsigned int idx = m_sequence->getIndex(key);
    if(idx == GItemList::UNSET)
    {
        LOG(warning) << "GAttrSeq::dump no item named: " << key ; 
    }
    else
    {
        std::string abbr = getAbbr(key);  
        const char* colorname = getColorName(key);  
        unsigned int colorcode = getColorCode(key);              

        std::cout << std::setw(5) << idx 
                  << std::setw(30) << key 
                  << std::setw(10) << std::hex << colorcode << std::dec
                  << std::setw(15) << ( colorname ? colorname : "-" ) 
                  << std::setw(15) << abbr
                  << std::endl ; 
    }
}



std::string GAttrSeq::decodeHexSequenceString(const char* seq, unsigned char ctrl)
{
    if(!seq) return "NULL" ;

    std::string lseq(seq);
    if(ctrl & REVERSE)
        std::reverse(lseq.begin(), lseq.end());
  
    std::stringstream ss ; 
    for(unsigned int i=0 ; i < lseq.size() ; i++) 
    {
        std::string sub = lseq.substr(i, 1) ;
        unsigned int local = hex_lexical_cast<unsigned int>(sub.c_str());
        unsigned int idx =  ctrl & ONEBASED ? local - 1 : local ;  
        const char* key = m_sequence->getKey(idx) ;
        std::string elem = ( ctrl & ABBREVIATE ) ? getAbbr(key) : key ; 
        ss << elem << " " ; 
    }
    return ss.str();
}





void GAttrSeq::dumpHexTable(Index* seqtab, const char* msg)
{
    LOG(info) << msg ;

    unsigned int colorcode(0xFFFFFF);

    for(unsigned int i=0 ; i < seqtab->getNumKeys() ; i++)
    {
        const char* key = seqtab->getKey(i);
        std::string label = getLabel(seqtab, key, colorcode ); 
        std::cout << std::setw(5) << i
                  << label 
                  << std::endl ; 
    }

    std::cout << std::setw(5) << "TOT" 
              << std::setw(10) << seqtab->getIndexSourceTotal()
              << std::endl ;
}


std::string GAttrSeq::getLabel(Index* index, const char* key, unsigned int& colorcode)
{
    colorcode = 0xFFFFFF ;

    unsigned int source = index->getIndexSource(key); // the count for seqmat, seqhis
    float fraction      = index->getIndexSourceFraction(key);
    std::string dseq    = decodeHexSequenceString(key);

    std::stringstream ss ;  
    ss
        << std::setw(10) << source 
        << std::setw(10) << std::setprecision(3) << std::fixed << fraction 
        << std::setw(25) << ( key ? key : "-" ) 
        << std::setw(40) << dseq 
        ; 
    
    return ss.str();
}



/*
std::pair<std::string, unsigned int>  GAttrSeq::getLabel(const char* key)
{
}
*/





