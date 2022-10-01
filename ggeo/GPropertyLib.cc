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
#include <cstring>
#include <climits>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <iomanip>

#include <boost/algorithm/string.hpp>

#include "SLogger.hh"
#include "SConstant.hh"
#include "SAbbrev.hh"
#include "NP.hh"
#include "sdomain.h"


// brap-
#include "BMeta.hh"
#include "BStr.hh"
#include "BDir.hh"
#include "Map.hh"

// npy-
#include "NGLM.hpp"
#include "NPY.hpp"

// optickscore-
#include "Opticks.hh"
#include "OpticksResource.hh"
#include "OpticksAttrSeq.hh"

// ggeo-
#include "GVector.hh"
#include "GDomain.hh"
#include "GItemList.hh"
#include "GProperty.hh"
#include "GPropertyMap.hh"
#include "GPropertyLib.hh"
#include "GConstant.hh"

#include "PLOG.hh"



const plog::Severity GPropertyLib::LEVEL = PLOG::EnvLevel("GPropertyLib", "DEBUG") ;



unsigned int GPropertyLib::UNSET = UINT_MAX ; 
unsigned int GPropertyLib::NUM_MATSUR = BOUNDARY_NUM_MATSUR  ;    // 4 material/surfaces that comprise a boundary om-os-is-im 
unsigned int GPropertyLib::NUM_PROP = BOUNDARY_NUM_PROP  ; 
unsigned int GPropertyLib::NUM_FLOAT4 = BOUNDARY_NUM_FLOAT4  ; 

const char* GPropertyLib::METANAME = "GPropertyLibMetadata.json" ; 


unsigned GPropertyLib::getUNSET(){ return UNSET ; }

void GPropertyLib::checkBufferCompatibility(unsigned int nk, const char* msg)
{

    if(nk != NUM_PROP)
    {
        LOG(fatal) << " GPropertyLib::checkBufferCompatibility "
                   << msg     
                   << " nk " << nk
                   << " NUM_PROP " << NUM_PROP
                   << " loading GPropertyLib with last dimension inconsistent with GPropLib::NUM_PROP " 
                   << " resolve by recreating the geocache, run with -G "
                   ;

    }
    assert(nk == NUM_PROP);
}


GDomain<double>* GPropertyLib::getDefaultDomain()    // static 
{
   // this is normal domain, only with --finebndtex does the finedomain interpolation get load within GBndLib::load
   //return new GDomain<double>(Opticks::DOMAIN_LOW, Opticks::DOMAIN_HIGH, Opticks::DOMAIN_STEP ); 
    return GDomain<double>::GetDefaultDomain(); 
}


const char* GPropertyLib::material = "material" ; 
const char* GPropertyLib::surface  = "surface" ; 
const char* GPropertyLib::source   = "source" ; 
const char* GPropertyLib::bnd_     = "bnd" ; 


// this ctor is used from GBndLib::load for interpolating to finedom
GPropertyLib::GPropertyLib(GPropertyLib* other, GDomain<double>* domain, bool optional )
    :
     m_log(new SLogger("GPropertyLib::GPropertyLib.interpolating-ctor")),
     m_ok(other->getOpticks()),
     m_resource(NULL),
     m_buffer(NULL),
     m_meta(NULL),
     m_attrnames(NULL),
     m_names(NULL),
     m_type(strdup(other->getType())),
     m_comptype(NULL),
     m_standard_domain(domain),
     m_optional(optional),
     m_defaults(NULL),
     m_closed(false),
     m_valid(true)
{
    init();
    (*m_log)("DONE");

    setNames(other->getNames());  // need setter for m_attrnames hookup
}


// this is the normal ctor, used by GBndLib, GMaterialLib
// the domain will get set in ::init to the default
GPropertyLib::GPropertyLib(Opticks* ok, const char* type, bool optional) 
     :
     //m_log(new SLogger("GPropertyLib::GPropertyLib.normal-ctor", type)),
     m_log(NULL),
     m_ok(ok),
     m_resource(NULL),
     m_buffer(NULL),
     m_meta(NULL),
     m_attrnames(NULL),
     m_names(NULL),
     m_type(strdup(type)),
     m_comptype(NULL),
     m_standard_domain(NULL),
     m_optional(optional),
     m_defaults(NULL),
     m_closed(false),
     m_valid(true)
{
     init();
     //(*m_log)("DONE");
}

Opticks* GPropertyLib::getOpticks() const 
{
    return m_ok ; 
}

const char* GPropertyLib::getType() const 
{
    return m_type ; 
}

const char* GPropertyLib::getComponentType() const 
{
    return m_comptype ; 
}

GPropertyLib::~GPropertyLib()
{
}

GDomain<double>* GPropertyLib::getStandardDomain()
{
    return m_standard_domain ;
}

void GPropertyLib::setStandardDomain(GDomain<double>* domain)
{
    m_standard_domain = domain ;
}

void  GPropertyLib::dumpDomain(const char* msg)
{
    GDomain<double>* dom = getStandardDomain() ;    

    double low = dom->getLow() ;
    double high = dom->getHigh() ;
    double step = dom->getStep() ;
    double dscale = double(GConstant::h_Planck*GConstant::c_light/GConstant::nanometer) ;
 
    LOG(info) << msg  ;

    LOG(info) << " low/high/step " 
              << " low  " << low
              << " high " << high
              << " step " << step
              << " dscale " << dscale 
              << " dscale/low " << dscale/low 
              << " dscale/high " << dscale/high
              ; 

    LOG(info) << "GPropertyLib::dumpDomain" 
               << " GC::nanometer " << GConstant::nanometer
               << " GC::h_Planck " << GConstant::h_Planck
               << " GC::c_light (mm/ns ~299.792) " << GConstant::c_light
               << " dscale " << dscale 
               ;   
 
}


GPropertyMap<double>* GPropertyLib::getDefaults()
{
    return m_defaults ;
}

void GPropertyLib::setBuffer(NPY<double>* buf)
{
    m_buffer = buf ;
}
NPY<double>* GPropertyLib::getBuffer() const 
{
    return m_buffer ;
}

/**
GPropertyLib::getBuf
----------------------

Convert NPY into NP with metadata and names passed along 

**/

NP* GPropertyLib::getBuf() const
{
    NP* buf = m_buffer ? m_buffer->spawn() : nullptr ; 
    const std::vector<std::string>& names = getNameList(); 
    if(buf && names.size() > 0)
    {
        buf->set_names(names); 
    }
    return buf ; 
}



template <typename T>
T GPropertyLib::getBufferMeta(const char* key, const char* fallback) const 
{
    return m_buffer->getMeta<T>(key, fallback);  
} 


void GPropertyLib::setMeta(BMeta* meta)
{
    m_meta = meta ;
}
BMeta* GPropertyLib::getMeta() const 
{
    return m_meta ; 
}

GItemList* GPropertyLib::getNames() const 
{
    return m_names ;
}

const std::vector<std::string>& GPropertyLib::getNameList() const 
{
    return m_names->getList(); 
}

OpticksAttrSeq* GPropertyLib::getAttrNames()
{
    return m_attrnames ;
}

std::string GPropertyLib::getAbbr(const char* key) const 
{
    assert(m_attrnames);
    return m_attrnames->getAbbr(key);
/*
cat opticksdata/export/DayaBay/GMaterialLib/abbrev.json
{
    "ADTableStainlessSteel": "AS",
    "Acrylic": "Ac",
    "Air": "Ai",
    "Aluminium": "Al",
    "Bialkali": "Bk",
*/
}


void GPropertyLib::setClosed(bool closed)
{
    m_closed = closed ; 
}
bool GPropertyLib::isClosed() const 
{
    return m_closed ; 
}

bool GPropertyLib::hasDomain() const 
{
    return m_standard_domain != NULL  ; 
}



void GPropertyLib::setValid(bool valid)
{
    m_valid = valid ; 
}
bool GPropertyLib::isValid() const 
{
    return m_valid ; 
}

void GPropertyLib::setNoLoad(bool noload)
{
    m_noload = noload ; 
}
bool GPropertyLib::isNoLoad() const 
{
    return m_noload ; 
}

bool GPropertyLib::isOptional() const 
{
    return m_optional ; 
}








void GPropertyLib::init()
{
    m_resource = m_ok->getResource();

    if(m_standard_domain == NULL)
    {
        //m_standard_domain = getDefaultDomain(); 
        m_standard_domain = GDomain<double>::GetDefaultDomain(); 

        unsigned len = getStandardDomainLength() ;
        unsigned len2 =  sdomain::DomainLength() ;
        if(len != len2)
        { 
            m_standard_domain->Summary("GPropertyLib::m_standard_domain");
            LOG(fatal) 
                << " domain length MISMATCH "
                << " len2 " << len2
                << " len " << len 
                ;
        }
        assert( len == len2 );   

    }
    else
    {
        LOG(warning) 
           << " using non-default domain " 
           << " step " << m_standard_domain->getStep()
           ; 
    }


    m_defaults = new GPropertyMap<double>("defaults", UINT_MAX, "defaults");
    m_defaults->setStandardDomain(m_standard_domain);


    m_attrnames = new OpticksAttrSeq(m_ok, m_type);
    m_attrnames->loadPrefs(); // color.json, abbrev.json and order.json 
    LOG(debug) << "GPropertyLib::init loadPrefs-DONE " ; 

    // hmm GPropertyMap expects bordersurface or skinsurface

    if(     strcmp(m_type, "GMaterialLib")==0)      m_comptype=material ;
    else if(strcmp(m_type, "GScintillatorLib")==0)  m_comptype=material ;
    else if(strcmp(m_type, "GSurfaceLib")==0)       m_comptype=surface ;
    else if(strcmp(m_type, "GSourceLib")==0)        m_comptype=source ;
    else if(strcmp(m_type, "GBndLib")==0)           m_comptype=bnd_ ;
    else                                            m_comptype=NULL  ;

    if(!m_comptype)
    {
        LOG(fatal) << "GPropertyLib::init " << m_type ;  
        assert( 0 &&  "unexpected GPropertyLib type");
    }    
}

std::map<std::string, unsigned int>& GPropertyLib::getOrder()
{
    return m_attrnames->getOrder() ; 
}

std::map<unsigned int, std::string> GPropertyLib::getNamesMap()
{
    return m_attrnames->getNamesMap() ; 
}

void GPropertyLib::getCurrentOrder(std::map<std::string, unsigned>& order ) 
{
    assert(m_names);
    m_names->getCurrentOrder(order); 
}


std::string GPropertyLib::getCacheDir()
{
    return m_resource->getPropertyLibDir(m_type);
}
std::string GPropertyLib::getPreferenceDir()
{
    return m_resource->getPreferenceDir(m_type);
}


unsigned int GPropertyLib::getIndex(const char* shortname) const 
{
    assert( isClosed() && " must close the lib before the indices can be used, as preference sort order may be applied at the close" ); 
    assert(m_names);
    return m_names->getIndex(shortname);
}



const char* GPropertyLib::getName(unsigned index) const 
{
    assert(m_names);
    const char* key = m_names->getKey(index);
    return key ; 
}

void GPropertyLib::getIndicesWithNameEnding( std::vector<unsigned>& indices, const char* ending ) const 
{
    return m_names->getIndicesWithKeyEnding(indices, ending ); 
}


std::string GPropertyLib::getBufferName(const char* suffix)
{
    std::string name = m_type ;  
    if(suffix) name += suffix ; 
    return name + ".npy" ; 
}


/**
GPropertyLib::close
---------------------

After sorting,  the subclass implementations of createNames, createBuffer, createMeta
are invoked : getting the lib ready to be persisted by for example copying the 
properties of the individual instances (eg GMaterial) into the buffer.

**/


void GPropertyLib::beforeClose()
{
   // do nothing default : that can be overridden in subclasses 
}

void GPropertyLib::close()
{
    LOG(verbose) << "[" ;

    beforeClose(); 


    if(m_ok->isDbgClose())
    {
        LOG(fatal) << "[--dbgclose] hari-kari " ; 
        assert(0);
    }

    sort();   // pure virtual implemented in sub classes

    // create methods from sub-class specializations
    GItemList* names = createNames();  
    NPY<double>* buf = createBuffer() ;  
    BMeta* meta = createMeta();

    LOG(LEVEL)
         << " type " << m_type 
         << " buf " <<  ( buf ? buf->getShapeString() : "NULL" )
         ; 

    //names->dump("GPropertyLib::close") ;

    setNames(names);
    setBuffer(buf);
    setMeta(meta);
    setClosed();

    LOG(verbose) << "]" ;
}


void GPropertyLib::saveToCache(NPYBase* buffer, const char* suffix)
{
    assert(suffix);
    std::string dir = getCacheDir(); 
    std::string name = getBufferName(suffix);


    LOG(LEVEL) 
        << " dir " << dir
        << " name " << name 
        << " type " << m_type 
        ;

    if(buffer)
    {
        buffer->save(dir.c_str(), name.c_str());   
    }
    else
    {
        LOG(error) 
            << " NULL BUFFER "
            << " dir " << dir
            << " name " << name
            ; 
    }
}

void GPropertyLib::saveToCache()
{
    LOG(LEVEL) << "[" ; 
 
    if(!isClosed()) close();

    if(m_buffer)
    {
        std::string dir = getCacheDir(); 
        std::string name = getBufferName();

        m_buffer->save(dir.c_str(), name.c_str());   

        if(m_meta)
        {
            m_meta->save(dir.c_str(),  METANAME ); 
        }
    }

    if(m_names)
    {
        saveNames(NULL);
    }

    LOG(LEVEL) << "]" ; 

}

void GPropertyLib::saveNames(const char* dir) const 
{
    assert(m_names); 
    m_names->save( dir ? dir : m_resource->getIdPath() );
}



void GPropertyLib::saveNames(const char* idpath, const char* reldir, const char* txtname) const
{
    if(m_names == NULL) 
    {
        LOG(fatal) << " no names to save " 
                   << " " << ( m_closed ? "CLOSED" : "not-closed" ) 
                   ; 
        return ; 
    }
    m_names->save(idpath, reldir, txtname); 
}




void GPropertyLib::loadFromCache()
{
    LOG(verbose) << "GPropertyLib::loadFromCache" ;

    std::string dir = getCacheDir(); 
    std::string name = getBufferName();

    LOG(verbose) << "GPropertyLib::loadFromCache" 
              << " dir " << dir
              << " name " << name 
               ;

  
    NPY<double>* buf = NPY<double>::load(dir.c_str(), name.c_str()); 


    if(!buf && isOptional())
    {
        LOG(info) << "Optional buffer not present "
                  << " dir " << dir
                  << " name " << name 
                  ;
        return ; 
    }


    if(!buf)
    {
        LOG(fatal) << "GPropertyLib::loadFromCache FAILED " 
                   << " dir " << dir
                   << " name " << name 
                   ;
    }
    assert(buf && "YOU PROBABLY NEED TO CREATE/RE-CREATE THE GEOCACHE BY RUNNING  : op.sh -G ");

    BMeta* meta = BMeta::Load(dir.c_str(), METANAME ) ; 
    assert( meta && "probably the geocache is an old version : lacking metadata : recreate geocache with -G option " );


    setBuffer(buf); 
    setMeta(meta) ; 

    GItemList* names = GItemList::Load(m_resource->getIdPath(), m_type);
    setNames(names); 

    import();   // <--- implemented in subclasses 
}


void GPropertyLib::setNames(GItemList* names)
{
    m_names = names ;
    m_attrnames->setSequence(names);
}




unsigned int GPropertyLib::getStandardDomainLength()
{
    return m_standard_domain ? m_standard_domain->getLength() : 0 ;
}

GProperty<double>* GPropertyLib::getDefaultProperty(const char* name)
{
    return m_defaults ? m_defaults->getProperty(name) : NULL ;
}


GProperty<double>* GPropertyLib::makeConstantProperty(double value)
{
    GProperty<double>* prop = GProperty<double>::from_constant( value, m_standard_domain->getValues(), m_standard_domain->getLength() );
    return prop ; 
}

GProperty<double>* GPropertyLib::makeRampProperty()
{
   return GProperty<double>::ramp( m_standard_domain->getLow(), m_standard_domain->getStep(), m_standard_domain->getValues(), m_standard_domain->getLength() );
}


GProperty<double>* GPropertyLib::getProperty(GPropertyMap<double>* pmap, const char* dkey) const 
{
    assert(pmap);

    const char* lkey = getLocalKey(dkey); assert(lkey);  // missing local key mapping 

    GProperty<double>* prop = pmap->getProperty(lkey) ;

    return prop ;  
}



/**
GPropertyLib::getPropertyOrDefault
------------------------------------

See GMaterialLib::createStandardMaterial for the canonical usage of this, 
creating standardized materials.

WARNING : the returned property will sometimes be a shared default value, 
thus the caller needs to make a copy of the returned property 
to make it their own. 

Why is that needed:

SHARING DEFAULT PROPERTY INSTANCES FROM LIB LEVEL ACROSS MULTIPLE MATERIALS
BECOMES A PROBLEM ONCE YOU WANT TO CHANGE ONE OF THEM : BECAUSE OF THE 
SHARED POINTER CHANGING THE MATERIAL OF ONE PROPERTTY WILL RESULT IN CHANGING THAT
PROPERTY FOR ALL MATERIALS THAT USED THE DEFAULT ... 

SOLUTION IS TO MAKE A COPY OF EVERY PROPERTY COLLECTED INTO EACH MATERIAL, 
SO THE MATERIAL "OWNS" ITS PROPERTIES WHICH CAN THEN BE CHANGED AS NEEDED.
**/

GProperty<double>* GPropertyLib::getPropertyOrDefault(GPropertyMap<double>* pmap, const char* dkey)
{
    // convert destination key such as "detect" into local key "EFFICIENCY" 

    const char* lkey = getLocalKey(dkey); assert(lkey);  // missing local key mapping 

    GProperty<double>* fallback = getDefaultProperty(dkey);  assert(fallback);

    GProperty<double>* prop = pmap ? pmap->getProperty(lkey) : NULL ;

    GProperty<double>* uprop = prop ? prop : fallback ; 

    return uprop ;
}



const char* GPropertyLib::getLocalKey(const char* dkey) const // mapping between standard keynames and local key names, eg refractive_index -> RINDEX
{
    return m_keymap.count(dkey) == 0 ? NULL : m_keymap.at(dkey).c_str();
}

void GPropertyLib::setKeyMap(const char* spec)
{
    m_keymap.clear();

    char delim = ',' ;
    std::istringstream f(spec);
    std::string s;
    while (getline(f, s, delim)) 
    {
        std::size_t colon = s.find(":");
        if(colon == std::string::npos)
        {
            printf("GPropertyLib::setKeyMap SKIPPING ENTRY WITHOUT COLON %s\n", s.c_str());
            continue ;
        }
        
        std::string dk = s.substr(0, colon);
        std::string lk = s.substr(colon+1);
        //printf("GPropertyLib::setKeyMap dk [%s] lk [%s] \n", dk.c_str(), lk.c_str());
        m_keymap[dk] = lk ; 
    }
}




NPY<unsigned int>* GPropertyLib::createUint4Buffer(std::vector<guint4>& vec )
{
    unsigned int ni = vec.size() ;
    unsigned int nj = 4  ; 

 
    NPY<unsigned int>* ibuf = NPY<unsigned int>::make( ni, nj) ;
    ibuf->zero();
    unsigned int* idat = ibuf->getValues();
    for(unsigned int i=0 ; i < ni ; i++)     
    {
        const guint4& entry = vec[i] ;
        for(unsigned int j=0 ; j < nj ; j++) idat[i*nj+j] = entry[j] ;  
    }
    return ibuf ; 
}


void GPropertyLib::importUint4Buffer(std::vector<guint4>& vec, NPY<unsigned int>* ibuf )
{
    if(ibuf == NULL)
    {
        LOG(warning) << "GPropertyLib::importUint4Buffer NULL buffer "  ; 
        setValid(false);
        return ; 
    } 

    LOG(debug) << "GPropertyLib::importUint4Buffer" ; 

    unsigned int* idat = ibuf->getValues();
    unsigned int ni = ibuf->getShape(0);
    unsigned int nj = ibuf->getShape(1);

    assert(nj == 4); 

    for(unsigned int i=0 ; i < ni ; i++)
    {
        guint4 entry(UNSET,UNSET,UNSET,UNSET);

        entry.x = idat[i*nj+0] ;
        entry.y = idat[i*nj+1] ;
        entry.z = idat[i*nj+2] ;
        entry.w = idat[i*nj+3] ;

        vec.push_back(entry);
    }
}

void GPropertyLib::addRaw(GPropertyMap<double>* pmap)
{
    m_raw.push_back(pmap);
}

void GPropertyLib::addRawOriginal(GPropertyMap<double>* pmap)
{
    m_raw_original.push_back(pmap);
}


unsigned GPropertyLib::getNumRaw() const 
{
    return m_raw.size();
}
unsigned GPropertyLib::getNumRawOriginal() const 
{
    return m_raw_original.size();
}




GPropertyMap<double>* GPropertyLib::getRaw(unsigned index) const 
{
    return index < m_raw.size() ? m_raw[index] : NULL ;
}
GPropertyMap<double>* GPropertyLib::getRawOriginal(unsigned index) const 
{
    return index < m_raw_original.size() ? m_raw_original[index] : NULL ;
}


GPropertyMap<double>* GPropertyLib::getRaw(const char* shortname) const 
{
    unsigned num_raw = m_raw.size();
    for(unsigned i=0 ; i < num_raw ; i++)
    {  
        GPropertyMap<double>* pmap = m_raw[i];
        const char* name = pmap->getShortName();
        if(strcmp(shortname, name) == 0) return pmap ;         
    }
    return NULL ; 
}

GPropertyMap<double>* GPropertyLib::getRawOriginal(const char* shortname) const 
{
    unsigned num_raw_original = m_raw_original.size();
    for(unsigned i=0 ; i < num_raw_original ; i++)
    {  
        GPropertyMap<double>* pmap = m_raw_original[i];
        const char* name = pmap->getShortName();
        if(strcmp(shortname, name) == 0) return pmap ;         
    }
    return NULL ; 
}


/**
GPropertyLib::SelectPropertyMapsWithProperties
------------------------------------------------

**/

void GPropertyLib::SelectPropertyMapsWithProperties(
    std::vector<GPropertyMap<double>*>& dst, 
    const char* props, 
    char delim, 
    const std::vector<GPropertyMap<double>*>& src
    ) 
{
    std::vector<std::string> elem ;
    BStr::split(elem, props, delim);

    for(unsigned i=0 ; i < src.size() ; i++)
    {
        GPropertyMap<double>* s = src[i] ; 
        unsigned found(0);
        for(unsigned p=0 ; p < elem.size() ; p++)
        { 
           if(s->hasProperty(elem[p].c_str())) found+=1 ;        
        }
        if(found == elem.size()) dst.push_back(s);
    }

    LOG(LEVEL)
         << props 
         << " src.size()  " << src.size() 
         << " dst.size()  " << dst.size() 
         ; 

}

void GPropertyLib::findRawMapsWithProperties( std::vector<GPropertyMap<double>*>& dst, const char* props, char delim )
{
    SelectPropertyMapsWithProperties(dst, props, delim, m_raw ); 
}

void GPropertyLib::findRawOriginalMapsWithProperties( std::vector<GPropertyMap<double>*>& dst, const char* props, char delim )
{
    SelectPropertyMapsWithProperties(dst, props, delim, m_raw_original ); 
}



void GPropertyLib::loadRaw()
{
    loadRaw( m_raw, SConstant::ORIGINAL_DOMAIN_SUFFIX, false ); 
}
void GPropertyLib::loadRawOriginal()
{
    loadRaw( m_raw_original, SConstant::ORIGINAL_DOMAIN_SUFFIX, true ); 
}

/**
GPropertyLib::loadRaw
-----------------------

*endswith* flips whether to include or excludes directories with the name suffix

**/


void GPropertyLib::loadRaw( std::vector<GPropertyMap<double>*>& dst, const char* dirname_suffix, bool endswith ) 
{
    std::string dir = getCacheDir();   // eg $IDPATH/GScintillatorLib

    std::vector<std::string> names ; 

    BDir::dirdirlist(names, dir.c_str(), dirname_suffix, endswith );   // find sub-directory names for all raw items in lib eg GdDopedLS,LiquidScintillator

    LOG(LEVEL) 
        << " dir " << dir
        << " names.size " << names.size()
        << " dirname_suffix " << dirname_suffix
        << " endswith " << endswith 
        ;
   
    for(std::vector<std::string>::iterator it=names.begin() ; it != names.end() ; it++ )
    {
        std::string name = *it ; 
        LOG(LEVEL) << name << " " << m_comptype ; 

        GPropertyMap<double>* pmap = GPropertyMap<double>::load( dir.c_str(), name.c_str(), m_comptype );
        if(pmap)
        {
            LOG(LEVEL) << name << " " << m_comptype << " num properties:" << pmap->getNumProperties() ; 
            dst.push_back(pmap);
        }
    }
}



void GPropertyLib::dumpRaw(const char* msg) const 
{
    unsigned int nraw = m_raw.size();
    LOG(info) << "[ nraw " << nraw << " " << msg ; 
    for(unsigned int i=0 ; i < nraw ; i++)
    {
        GPropertyMap<double>* pmap = m_raw[i] ;
        LOG(info) << " component " << pmap->getName() ;
        LOG(info) << " table " << pmap->make_table() ;
    }
    LOG(info) << "] nraw " << nraw << " " << msg ; 
}


 
void GPropertyLib::saveRaw()
{
    std::string dir = getCacheDir(); 
    unsigned num_raw = m_raw.size();
    LOG(LEVEL) << "[ " << dir << " num_raw " << num_raw ; 
    for(unsigned i=0 ; i < num_raw ; i++)
    {
        GPropertyMap<double>* pmap = m_raw[i] ;
        pmap->save(dir.c_str());
    }
    LOG(LEVEL) << "]" ; 
}

void GPropertyLib::saveRawOriginal()
{
    std::string dir = getCacheDir(); 
    unsigned num_raw_original = m_raw_original.size();
    LOG(LEVEL) << "[ " << dir << " num_raw_original " << num_raw_original ; 
    for(unsigned i=0 ; i < num_raw_original ; i++)
    {
        GPropertyMap<double>* pmap = m_raw_original[i] ;
        pmap->save(dir.c_str());
    }
    LOG(LEVEL) << "]" ; 
}






void GPropertyLib::dumpNames(const char* msg) const 
{
    LOG(info) << msg ; 
   
    GItemList* names_ = getNames();
    if(names_ == NULL) 
    {
        LOG(warning) << "GPropertyLib::dumpNames names NULL " ; 
    }
    else
    {
        LOG(info) << "GPropertyLib::dumpNames non-null " ; 
        names_->dump("names_"); 
    } 
}



BMeta* GPropertyLib::CreateAbbrevMeta(const std::vector<std::string>& names )
{
    LOG(LEVEL) << "[" ; 

    SAbbrev abbrev(names); 
    assert( abbrev.abbrev.size() == names.size() ); 

    BMeta* abbrevmeta = new BMeta ; 
    for(unsigned i=0 ; i < names.size() ; i++)
    {
        const std::string& nm = names[i] ; 
        const std::string& ab = abbrev.abbrev[i] ; 
        abbrevmeta->set<std::string>(nm.c_str(), ab ) ; 
    }
    LOG(LEVEL) << "]" ; 
    return abbrevmeta ; 
}



/**
GPropertyLib::isSensorIndex
----------------------------

Checks for the presense of the index within m_sensor_indices, which 
is a pre-cache transient (non-persisted) vector of surface indices
from the GSurfaceLib subclass or material indices 
from GMaterialLib subclass.

**/

bool GPropertyLib::isSensorIndex(unsigned index) const 
{
    typedef std::vector<unsigned>::const_iterator UI ; 
    UI b = m_sensor_indices.begin(); 
    UI e = m_sensor_indices.end(); 
    UI i = std::find(b, e, index); 
    return i != e ; 
}

/**
GPropertyLib::addSensorIndex
------------------------------

Canonically invoked from GSurfaceLib::collectSensorIndices
based on finding non-zero EFFICIENCY property.

**/
void GPropertyLib::addSensorIndex(unsigned index)
{ 
    m_sensor_indices.push_back(index); 
}
unsigned GPropertyLib::getNumSensorIndices() const
{
    return m_sensor_indices.size(); 
}
unsigned GPropertyLib::getSensorIndex(unsigned i) const 
{
    return m_sensor_indices[i] ; 
}
void GPropertyLib::dumpSensorIndices(const char* msg) const 
{
    unsigned ni = getNumSensorIndices() ; 
    std::stringstream ss ; 
    ss << " NumSensorIndices " << ni << " ( " ; 
    for(unsigned i=0 ; i < ni ; i++) ss << getSensorIndex(i) << " " ; 
    ss << " ) " ; 
    std::string desc = ss.str();   
    LOG(info) << msg << " " << desc ; 
}
  


template GGEO_API unsigned    GPropertyLib::getBufferMeta(const char* key, const char* fallback) const ; 
template GGEO_API int         GPropertyLib::getBufferMeta(const char* key, const char* fallback) const ; 
template GGEO_API float       GPropertyLib::getBufferMeta(const char* key, const char* fallback) const ; 
template GGEO_API double      GPropertyLib::getBufferMeta(const char* key, const char* fallback) const ; 
template GGEO_API std::string GPropertyLib::getBufferMeta(const char* key, const char* fallback) const ; 


