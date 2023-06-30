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


#include <climits>
#include <cassert>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <iomanip>

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#include "NP.hh"

#include "NGLM.hpp"
#include "NPY.hpp"
#include "Opticks.hh"
#include "sdomain.h"


#include "GVector.hh"
#include "GItemList.hh"
#include "GAry.hh"
#include "GDomain.hh"
#include "GProperty.hh"
#include "GPropertyMap.hh"

#include "GMaterialLib.hh"
#include "GSurfaceLib.hh"
#include "GBnd.hh"
#include "GBndLib.hh"

#include "SLOG.hh"


const plog::Severity GBndLib::LEVEL = SLOG::EnvLevel("GBndLib", "DEBUG") ; 

const GBndLib* GBndLib::INSTANCE = NULL ; 
const GBndLib* GBndLib::GetInstance(){ return INSTANCE ; }

unsigned GBndLib::MaterialIndexFromLine( unsigned line ) 
{
    assert( INSTANCE ) ; 
    return INSTANCE->getMaterialIndexFromLine(line) ;
}

// for GPropertyLib base
BMeta* GBndLib::createMeta()
{
    return NULL ;
}

void GBndLib::save()
{
    saveIndexBuffer();  
}

/**
GBndLib::load
---------------

Hmm, finebndtex appears to be done here postcache ?
It surely makes for sense to define a finer domain to use precache.

**/

GBndLib* GBndLib::load(Opticks* ok, bool constituents)
{
    LOG(LEVEL) << "[" ; 
    GBndLib* blib = new GBndLib(ok);

    LOG(verbose) ;

    blib->loadIndexBuffer();

    LOG(verbose) << "indexBuffer loaded" ; 
    blib->importIndexBuffer();


    if(constituents)
    {
        GMaterialLib* mlib = GMaterialLib::load(ok);
        GSurfaceLib* slib = GSurfaceLib::load(ok);
        GDomain<double>* finedom = ok->hasOpt("finebndtex") 
                            ?
                                mlib->getStandardDomain()->makeInterpolationDomain(sdomain::FINE_DOMAIN_STEP) 
                            :
                                NULL 
                            ;

        //assert(0); 

        if(finedom)
        {
            LOG(warning) << "--finebndtex option triggers interpolation of material and surface props "  ;
            GMaterialLib* mlib2 = new GMaterialLib(mlib, finedom );    
            GSurfaceLib* slib2 = new GSurfaceLib(slib, finedom );    

            mlib2->setBuffer(mlib2->createBuffer());
            slib2->setBuffer(slib2->createBuffer());

            blib->setStandardDomain(finedom);
            blib->setMaterialLib(mlib2);
            blib->setSurfaceLib(slib2);

            blib->setBuffer(blib->createBuffer()); 
        }
        else
        {
            blib->setMaterialLib(mlib);
            blib->setSurfaceLib(slib);
        } 
    }

    LOG(LEVEL) << "]" ; 

    return blib ; 
}

void GBndLib::loadIndexBuffer()
{
    LOG(LEVEL) ; 

    std::string dir = getCacheDir(); 
    std::string name = getBufferName("Index");

    LOG(verbose) << "GBndLib::loadIndexBuffer" 
               << " dir " << dir
               << " name " << name 
                ; 


    NPY<unsigned int>* indexBuf = NPY<unsigned int>::load(dir.c_str(), name.c_str()); 


    LOG(verbose) << "GBndLib::loadIndexBuffer" 
               << " indexBuf " << indexBuf
               ;


    setIndexBuffer(indexBuf); 

    if(indexBuf == NULL) 
    {
        LOG(warning) << "GBndLib::loadIndexBuffer setting invalid " ; 
        setValid(false);
    }
    else
    {
        LOG(debug) << "GBndLib::loadIndexBuffer"
                  << " shape " << indexBuf->getShapeString() ;
    }
}


void GBndLib::saveIndexBuffer()
{
    NPY<unsigned int>* indexBuf = createIndexBuffer();
    setIndexBuffer(indexBuf);

    saveToCache(indexBuf, "Index") ; 
}

void GBndLib::saveOpticalBuffer()
{
    NPY<unsigned int>* optical_buffer = createOpticalBuffer();
    setOpticalBuffer(optical_buffer);
    saveToCache(optical_buffer, "Optical") ; 
}




void GBndLib::dumpOpticalBuffer() const 
{
    LOG(error) << "." ; 


    if( m_optical_buffer )
    {
        m_optical_buffer->dump("dumpOpticalBuffer");     
    }

}





void GBndLib::createDynamicBuffers()
{
    // there is not much difference between this and doing a close ??? 

    GItemList* names = createNames();     // added Aug 21, 2018
    setNames(names); 

    NPY<double>* buf = createBuffer();  // createBufferForTex2d
    setBuffer(buf);

    NPY<unsigned int>* optical_buffer = createOpticalBuffer();
    setOpticalBuffer(optical_buffer);


    LOG(debug) << "GBndLib::createDynamicBuffers" 
              << " buf " << ( buf ? buf->getShapeString() : "NULL" )
              << " optical_buffer  " << ( optical_buffer ? optical_buffer->getShapeString() : "NULL" )
               ;

    // declare closed here ? 

}


NPY<unsigned int>* GBndLib::createIndexBuffer()
{
    NPY<unsigned int>* idx = NULL ;  
    if(m_bnd.size() > 0)
    { 
       idx = createUint4Buffer(m_bnd);
    } 
    else
    {
        LOG(error) << "GBndLib::createIndexBuffer"
                   << " BUT SIZE IS ZERO "
                   ;
  
    } 
    return idx ;
}

void GBndLib::importIndexBuffer()
{
    LOG(verbose) << "GBndLib::importIndexBuffer" ; 
    NPY<unsigned int>* ibuf = getIndexBuffer();

    if(ibuf == NULL)
    {
         LOG(warning) << "GBndLib::importIndexBuffer NULL buffer setting invalid" ; 
         setValid(false);
         return ;
    }

    LOG(debug) << "GBndLib::importIndexBuffer BEFORE IMPORT" 
              << " ibuf " << ibuf->getShapeString()
              << " m_bnd.size() " << m_bnd.size()
             ; 

    importUint4Buffer(m_bnd, ibuf );

    LOG(debug) << "GBndLib::importIndexBuffer AFTER IMPORT" 
              << " ibuf " << ibuf->getShapeString()
              << " m_bnd.size() " << m_bnd.size()
             ; 
}


void GBndLib::setMaterialLib(GMaterialLib* mlib)
{
    m_mlib = mlib ;  
}
void GBndLib::setSurfaceLib(GSurfaceLib* slib)
{
    m_slib = slib ;  
}
GMaterialLib* GBndLib::getMaterialLib()
{
    return m_mlib ; 
}
GSurfaceLib* GBndLib::getSurfaceLib()
{
    return m_slib ; 
}


unsigned int GBndLib::getNumBnd() const
{
    return m_bnd.size() ; 
}

NPY<unsigned int>* GBndLib::getIndexBuffer()
{
    return m_index_buffer ;
}

bool GBndLib::hasIndexBuffer()
{
    return m_index_buffer != NULL ; 
}

void GBndLib::setIndexBuffer(NPY<unsigned int>* index_buffer)
{
    m_index_buffer = index_buffer ;
}

NPY<unsigned int>* GBndLib::getOpticalBuffer() const 

{
    return m_optical_buffer ;
}
void GBndLib::setOpticalBuffer(NPY<unsigned int>* optical_buffer)
{
    m_optical_buffer = optical_buffer ;
}


NP* GBndLib::getOpticalBuf() const 
{
    assert( m_optical_buffer ); 

    NP* optical = m_optical_buffer->spawn() ;  
    std::string shape0 = optical->sstr() ; 

    assert( optical->shape.size() == 3 ); 

    unsigned ni = optical->shape[0] ; 
    unsigned nj = optical->shape[1] ; 
    unsigned nk = optical->shape[2] ; 

    assert( ni > 0 && nj == 4 && nk == 4 ); 

    optical->change_shape( ni*nj , nk );  
    LOG(LEVEL) << " changed optical shape from " << shape0  << " -> " << optical->sstr() ; 

    return optical ;
}




GBndLib::GBndLib(Opticks* ok, GMaterialLib* mlib, GSurfaceLib* slib)
   :
    GPropertyLib(ok, "GBndLib"),
    m_dbgbnd(ok->isDbgBnd()),
    m_mlib(mlib),
    m_slib(slib),
    m_index_buffer(NULL),
    m_optical_buffer(NULL),
    m_sensor_count(0)
{
    init();
}

GBndLib::GBndLib(Opticks* ok) 
   :
    GPropertyLib(ok, "GBndLib"),
    m_dbgbnd(ok->isDbgBnd()),
    m_mlib(NULL),
    m_slib(NULL),
    m_index_buffer(NULL),
    m_optical_buffer(NULL),
    m_sensor_count(0)
{
    init();
}


void GBndLib::init()
{
    INSTANCE=this ; 
    if(m_dbgbnd)
    {
        LOG(fatal) << "[--dbgbnd] " << m_dbgbnd ; 
    }
    assert(UNSET == GItemList::UNSET);
}


void GBndLib::closeConstituents()
{
    LOG(LEVEL) ; 
    if(m_mlib) m_mlib->close(); 
    if(m_slib) m_slib->close(); 
}


bool GBndLib::isDbgBnd() const 
{
    return m_dbgbnd ; 
}



bool GBndLib::contains(const char* spec, bool flip) const
{
    guint4 bnd = parse(spec, flip);
    return contains(bnd);
}



guint4 GBndLib::parse( const char* spec, bool flip) const
{
    GBnd b(spec, flip, m_mlib, m_slib, m_dbgbnd);
    return guint4( b.omat, b.osur, b.isur, b.imat ) ; 
}


/**
GBndLib::getBoundary
---------------------

Parses the spec returning the 0-based index of the boundary 
if present, otherwise returns UNSET which is 0xffffffff unsigned -1 

**/

unsigned GBndLib::getBoundary( const char* spec) const
{
    if( spec == NULL ) return UNSET ; 
    bool flip = false ; // omat/osur/isur/imat
    GBnd b(spec, flip, m_mlib, m_slib, m_dbgbnd);
    guint4 bnd = guint4( b.omat, b.osur, b.isur, b.imat );
    unsigned boundary = index(bnd) ;
    return boundary ; 
} 

/**
GBndLib::getSignedBoundary
---------------------------

Return signed 1-based boundary for the spec where negation is done 
when the first char is '-'

**/

int GBndLib::getSignedBoundary( const char* spec ) const 
{
    bool negate = spec != NULL && strlen(spec) > 1 && spec[0] == '-' ;  
    unsigned boundary = getBoundary(negate ? spec + 1 : spec ); 
    if( boundary == UNSET ) return 0 ; 
    return negate ? -(boundary+1) : (boundary+1) ; 
}


unsigned GBndLib::addBoundary( const char* spec, bool flip)
{
    // used by GMaker::makeFromCSG
    // hmm: when need to create surf, need the volnames ?

    GBnd b(spec, flip, m_mlib, m_slib, m_dbgbnd);


    guint4 bnd = guint4( b.omat, b.osur, b.isur, b.imat );
    add(bnd);
    unsigned boundary = index(bnd) ;

    if(m_dbgbnd)
    {
       LOG(info) << "[--dbgbnd] "
                 << " spec " << spec 
                 << " flip " << flip 
                 << " bnd " << bnd.description()
                 << " boundary " << boundary
                 ;
    }
    return boundary ; 
}

/**
GBndLib::addBoundary
-----------------------

The names of materials and surfaces given as arguments are used to lookup the corresponding 
material and surface indices in their libs allowing the formation of the uint4 bnd.
Only unique bnd are collected and the index of the bnd is returned.

**/
unsigned GBndLib::addBoundary( const char* omat, const char* osur, const char* isur, const char* imat)
{
    LOG(LEVEL) 
        << " omat " << ( omat ? omat : "-" )  
        << " osur " << ( osur ? osur : "-" ) 
        << " isur " << ( isur ? isur : "-" ) 
        << " imat " << ( imat ? imat : "-" )  
        ;

    guint4 bnd = add(omat, osur, isur, imat);
    unsigned boundary = index(bnd) ; 
    m_boundary_add_count[boundary] += 1 ; 
    return boundary ; 
}

guint4 GBndLib::add( const char* spec, bool flip)
{
    guint4 bnd = parse(spec, flip);
    add(bnd);
    return bnd ; 
}
guint4 GBndLib::add( const char* omat_ , const char* osur_, const char* isur_, const char* imat_ )
{
    unsigned omat = m_mlib->getIndex(omat_) ;   // these are 0-based indices or UINT_MAX when no match
    unsigned osur = m_slib->getIndex(osur_) ;
    unsigned isur = m_slib->getIndex(isur_) ;
    unsigned imat = m_mlib->getIndex(imat_) ;

    bool known_omat =  omat != UINT_MAX ; 
    bool known_imat =  imat != UINT_MAX ; 
 
    if(! (known_omat && known_imat))
    {
        LOG(fatal) 
            << " material name is not present in the mlib " 
            << " omat_ " << omat_
            << " omat " << omat
            << " imat_ " << imat_
            << " imat " << imat
            ;

        LOG(fatal) 
            << " mlib.desc " << std::endl 
            << m_mlib->desc()
             ; 
    } 

    assert( known_omat ); 
    assert( known_imat ); 

    return add(omat, osur, isur, imat);
}
guint4 GBndLib::add( unsigned omat , unsigned osur, unsigned isur, unsigned imat )
{
    guint4 bnd = guint4(omat, osur, isur, imat);
    add(bnd);
    return bnd ; 
}

void GBndLib::add(const guint4& bnd)  // all the adders invoke this
{
    if(!contains(bnd)) m_bnd.push_back(bnd);
}


bool GBndLib::contains(const guint4& bnd) const 
{
    typedef std::vector<guint4> G ;  
    G::const_iterator b = m_bnd.begin() ;
    G::const_iterator e = m_bnd.end() ;
    G::const_iterator i = std::find(b, e, bnd) ;
    return i != e ;
}

unsigned int GBndLib::index(const guint4& bnd) const 
{
    typedef std::vector<guint4> G ;  
    G::const_iterator b = m_bnd.begin() ;
    G::const_iterator e = m_bnd.end() ;
    G::const_iterator i = std::find(b, e, bnd) ;
    return i == e ? UNSET : std::distance(b, i) ; 
}

std::string GBndLib::description(const guint4& bnd) const
{
    unsigned int idx = index(bnd) ;
    std::string tag = idx == UNSET ? "-" : boost::lexical_cast<std::string>(idx) ; 

    unsigned omat = bnd[OMAT] ;
    unsigned osur = bnd[OSUR] ;
    unsigned isur = bnd[ISUR] ;
    unsigned imat = bnd[IMAT] ;

    std::stringstream ss ; 
    ss 
       << " ("   << std::setw(3) << tag << ")" 
       << " om:" << std::setw(25) << (omat == UNSET ? "OMAT-unset-ERROR" : m_mlib->getName(omat))
       << " os:" << std::setw(31) << (osur == UNSET ? ""                 : m_slib->getName(osur))  
       << " is:" << std::setw(31) << (isur == UNSET ? ""                 : m_slib->getName(isur)) 
       << " im:" << std::setw(25) << (imat == UNSET ? "IMAT-unset-ERROR" : m_mlib->getName(imat)) 
       << " ("   << std::setw(3) << tag << ")" 
       << "     "
       << " (" 
       << std::setw(2) << omat << ","
       << std::setw(2) << ( osur == UNSET ? -1 : (int)osur ) << ","
       << std::setw(2) << ( isur == UNSET ? -1 : (int)isur ) << ","
       << std::setw(2) << imat 
       << ")" 
       ;
    return ss.str();
}

std::string GBndLib::shortname(unsigned boundary) const 
{
    guint4 bnd = getBnd(boundary);
    return shortname(bnd);
}


std::string GBndLib::shortname(const guint4& bnd) const 
{
    std::stringstream ss ; 
    ss 
       << (bnd[OMAT] == UNSET ? "OMAT-unset-error" : m_mlib->getName(bnd[OMAT])) 
       << "/"
       << (bnd[OSUR] == UNSET ? "" : m_slib->getName(bnd[OSUR])) 
       << "/" 
       << (bnd[ISUR] == UNSET ? "" : m_slib->getName(bnd[ISUR]))  
       << "/" 
       << (bnd[IMAT] == UNSET ? "IMAT-unset-error" : m_mlib->getName(bnd[IMAT]))
       ;
    return ss.str();
}


void GBndLib::getBoundaryNames(std::vector<std::string>& boundaryNames) const 
{
    unsigned num_bnd = getNumBnd(); 
    for(unsigned boundary=0 ; boundary < num_bnd ; boundary++) 
    {
        const guint4& bnd = m_bnd[boundary];
        std::string spec = shortname(bnd); 
        boundaryNames.push_back(spec); 
    }
}


void GBndLib::getBnd(int& omat, int& osur, int& isur, int& imat, unsigned boundary) const 
{
    unsigned int ni = getNumBnd();
    assert(boundary < ni);
    const guint4& bnd = m_bnd[boundary];

    omat = bnd[OMAT] ;  
    osur = bnd[OSUR] ;  
    isur = bnd[ISUR] ;  
    imat = bnd[IMAT] ;  
}

bool GBndLib::isSameMaterialBoundary(unsigned boundary) const 
{
    int omat, osur, isur, imat ; 
    getBnd(omat, osur, isur, imat, boundary); 
    return omat == imat ; 
}



/**
GBndLib::isSensorBoundary
--------------------------

Canonically invoked from X4PhysicalVolume::convertNode 


**/

bool GBndLib::isSensorBoundary(unsigned boundary) const 
{
    const guint4& bnd = m_bnd[boundary];
    bool osur_sensor = m_slib->isSensorIndex(bnd[OSUR]); 
    bool isur_sensor = m_slib->isSensorIndex(bnd[ISUR]); 
    bool is_sensor = osur_sensor || isur_sensor ; 
    return is_sensor ; 
}

void GBndLib::countSensorBoundary(unsigned boundary)
{
    m_boundary_sensor_count[boundary] += 1 ; 
    m_sensor_count += 1 ; 
}

/**
GBndLib::getSensorCount (precache)
------------------------------------

**/
unsigned GBndLib::getSensorCount() const 
{
    return m_sensor_count ; 
}

std::string GBndLib::getSensorBoundaryReport() const 
{
    std::stringstream ss ; 
    typedef std::map<unsigned, unsigned>::const_iterator IT ; 

    unsigned sensor_total = 0 ; 
    for(IT it=m_boundary_sensor_count.begin() ; it != m_boundary_sensor_count.end() ; it++)
    {
        unsigned boundary = it->first ; 
        unsigned sensor_count = it->second ; 
        ss 
            << " boundary " << std::setw(3) << boundary
            << " b+1 " << std::setw(3) << boundary+1
            << " sensor_count " << std::setw(6) << sensor_count
            << " " << shortname(boundary)
            << std::endl 
            ;
        sensor_total += sensor_count ; 
    }

    ss 
        << "          " << "   "
        << "     " << "   "
        << " sensor_total " << std::setw(6) << sensor_total
        << std::endl 
        ;

    return ss.str(); 
}




std::string GBndLib::getAddBoundaryReport(const char* msg, unsigned edgeitems) const 
{
    unsigned num_boundary_add = m_boundary_add_count.size() ;  
    std::stringstream ss ; 
    ss << msg 
       << " edgeitems " << edgeitems 
       << " num_boundary_add " << num_boundary_add
       << std::endl
       ; 

    typedef std::map<unsigned, unsigned>::const_iterator IT ; 
    unsigned count = 0 ; 
    unsigned add_total = 0 ; 
    for(IT it=m_boundary_add_count.begin() ; it != m_boundary_add_count.end() ; it++)
    {
        unsigned boundary = it->first ; 
        unsigned add_count = it->second ; 
        if( count < edgeitems || count > num_boundary_add - edgeitems )
        {
            ss 
                << " boundary " << std::setw(3) << boundary
                << " b+1 " << std::setw(3) << boundary+1
                << " add_count " << std::setw(6) << add_count
                << " " << shortname(boundary)
                << std::endl 
                ;
        }
        add_total += add_count ; 
        count += 1 ; 
    }

    ss 
        << "          " << "   "
        << "     " << "   "
        << " add_total " << std::setw(6) << add_total
        << std::endl 
        ;

    return ss.str(); 
}





guint4 GBndLib::getBnd(unsigned boundary) const
{
    unsigned int ni = getNumBnd();
    assert(boundary < ni);
    const guint4& bnd = m_bnd[boundary];
    return bnd ;  
}

unsigned int GBndLib::getInnerMaterial(unsigned boundary) const 
{
    guint4 bnd = getBnd(boundary);
    return bnd[IMAT] ; 
}
unsigned int GBndLib::getOuterMaterial(unsigned boundary) const 
{
    guint4 bnd = getBnd(boundary);
    return bnd[OMAT] ; 
}
unsigned int GBndLib::getInnerSurface(unsigned boundary) const 
{
    guint4 bnd = getBnd(boundary);
    return bnd[ISUR] ; 
}
unsigned int GBndLib::getOuterSurface(unsigned boundary) const 
{
    guint4 bnd = getBnd(boundary);
    return bnd[OSUR] ; 
}

const char* GBndLib::getOuterMaterialName(unsigned boundary) const
{
    unsigned int omat = getOuterMaterial(boundary);
    return m_mlib->getName(omat);
}
const char* GBndLib::getInnerMaterialName(unsigned boundary) const 
{
    unsigned int imat = getInnerMaterial(boundary);
    return m_mlib->getName(imat);
}

const char* GBndLib::getOuterSurfaceName(unsigned boundary) const
{
    unsigned int osur = getOuterSurface(boundary);
    return m_slib->getName(osur);
}
const char* GBndLib::getInnerSurfaceName(unsigned boundary) const
{
    unsigned int isur = getInnerSurface(boundary);
    return m_slib->getName(isur);
}






const char* GBndLib::getOuterMaterialName(const char* spec) 
{
    unsigned int boundary = addBoundary(spec);
    return getOuterMaterialName(boundary);
}
const char* GBndLib::getInnerMaterialName(const char* spec) 
{
    unsigned int boundary = addBoundary(spec);
    return getInnerMaterialName(boundary);
}
const char* GBndLib::getOuterSurfaceName(const char* spec)
{
    unsigned int boundary = addBoundary(spec);
    return getOuterSurfaceName(boundary);
}
const char* GBndLib::getInnerSurfaceName(const char* spec)
{
    unsigned int boundary = addBoundary(spec);
    return getInnerSurfaceName(boundary);
}







GItemList* GBndLib::createNames()
{
    unsigned int ni = getNumBnd();
    GItemList* names = new GItemList(getType());
    for(unsigned int i=0 ; i < ni ; i++)      // over bnd
    {
        const guint4& bnd = m_bnd[i] ;
        names->add(shortname(bnd).c_str()); 
    }
    return names ; 
}



void GBndLib::dumpMaterialLineMap(std::map<std::string, unsigned int>& msu, const char* msg)
{ 
    LOG(info) << msg ; 
    typedef std::map<std::string, unsigned int> MSU ; 
    for(MSU::const_iterator it = msu.begin() ; it != msu.end() ; it++)
        LOG(info) << std::setw(5) << it->second 
                   << std::setw(30) << it->first 
                   ;
}

void GBndLib::fillMaterialLineMap( std::map<std::string, unsigned>& msu)
{
    // first occurence of a material within the boundaries
    // has its material line recorded in the MaterialLineMap

    unsigned num_bnd = getNumBnd() ; 
    LOG(LEVEL) << " num_bnd " << num_bnd ; 

    for(unsigned int i=0 ; i < num_bnd ; i++)    
    {
        const guint4& bnd = m_bnd[i] ;
        const char* omat = m_mlib->getName(bnd[OMAT]);
        const char* imat = m_mlib->getName(bnd[IMAT]);
        assert(imat && omat);
        if(msu.count(imat) == 0) msu[imat] = getLine(i, IMAT) ;
        if(msu.count(omat) == 0) msu[omat] = getLine(i, OMAT) ; 
    }

    if(m_ok->isMaterialDbg())
    {
        dumpMaterialLineMap(msu, "GBndLib::fillMaterialLineMap (--materialdbg) ");
    }
}

const std::map<std::string, unsigned int>& GBndLib::getMaterialLineMap()
{
    if(m_materialLineMap.size() == 0) fillMaterialLineMap(m_materialLineMap) ;
    return m_materialLineMap ;
}

void GBndLib::fillMaterialLineMap()
{
    if(m_materialLineMap.size() == 0) fillMaterialLineMap(m_materialLineMap) ;
}

const std::map<std::string, unsigned int>& GBndLib::getMaterialLineMapConst() const
{
    return m_materialLineMap ;
}


void GBndLib::dumpMaterialLineMap(const char* msg)
{
    LOG(info) << "GBndLib::dumpMaterialLineMap" ; 

    if(m_materialLineMap.size() == 0) fillMaterialLineMap(m_materialLineMap) ;


    LOG(info) << "GBndLib::dumpMaterialLineMap" 
              << " m_materialLineMap.size()  " << m_materialLineMap.size() 
              ; 

    dumpMaterialLineMap(m_materialLineMap, msg );
}






unsigned GBndLib::getMaterialLine(const char* shortname_)
{
    // used by App::loadGenstep for setting material line in TorchStep
    unsigned ni = getNumBnd();
    unsigned line = 0 ; 
    for(unsigned i=0 ; i < ni ; i++)    
    {
        const guint4& bnd = m_bnd[i] ;
        const char* omat = m_mlib->getName(bnd[OMAT]);
        const char* imat = m_mlib->getName(bnd[IMAT]);

        if(strncmp(imat, shortname_, strlen(shortname_))==0)
        { 
            line = getLine(i, IMAT);  
            break ;
        }
        if(strncmp(omat, shortname_, strlen(shortname_))==0) 
        { 
            line=getLine(i, OMAT); 
            break ;
        } 
    }

    LOG(verbose) << "GBndLib::getMaterialLine"
              << " shortname_ " << shortname_ 
              << " line " << line 
              ; 

    return line ;
}
unsigned GBndLib::getLine(unsigned ibnd, unsigned imatsur)
{
    assert(imatsur < NUM_MATSUR);  // NUM_MATSUR canonically 4
    return ibnd*NUM_MATSUR + imatsur ;   
}
unsigned GBndLib::getLineMin()
{
    unsigned lineMin = getLine(0, 0);
    return lineMin ; 
}
unsigned int GBndLib::getLineMax()
{
    unsigned numBnd = getNumBnd() ; 
    unsigned lineMax = getLine(numBnd - 1, NUM_MATSUR-1);   
    assert(lineMax == numBnd*NUM_MATSUR - 1 );
    return lineMax ; 
}

unsigned GBndLib::getMaterialIndexFromLine(unsigned line) const 
{
    unsigned numBnd = getNumBnd() ; 
    unsigned ibnd = line / NUM_MATSUR ; 
    assert( NUM_MATSUR == 4 ); 
    assert( ibnd < numBnd ) ;  
    unsigned imatsur = line - ibnd*NUM_MATSUR ; 

    LOG(error) 
        << " line " << line 
        << " ibnd " << ibnd 
        << " numBnd " << numBnd 
        << " imatsur " << imatsur
        ;

    assert( imatsur == 0 || imatsur == 3 ); 

    assert( m_optical_buffer );  
    glm::uvec4 optical = m_optical_buffer->getQuadU(ibnd, imatsur, 0 );  
 
    unsigned matIdx1 = optical.x ; 
    assert( matIdx1 >= 1 );   // 1-based index

    return matIdx1 - 1 ;  // 0-based index
}


bool GBndLib::canCreateBuffer() const
{
    NPY<double>* mat = m_mlib->getBuffer();
    return mat != nullptr  ; 
}


/**
GBndLib::createBuffer
----------------------

Q: is this canonically invoked postcache ?


**/

NPY<double>* GBndLib::createBuffer()
{
    return createBufferForTex2d() ;
}


/**
GBndLib::createBufferForTex2d
-------------------------------

GBndLib double buffer is a memcpy zip of the MaterialLib and SurfaceLib buffers
pulling together data based on the indices for the materials and surfaces 
from the m_bnd guint4 buffer

Typical dimensions : (128, 4, 2, 39, 4)   

           128 : boundaries, 
             4 : mat-or-sur for each boundary  
             2 : payload-categories corresponding to NUM_FLOAT4
            39 : wavelength samples
             4 : double4-values

The only dimension that can easily be extended is the middle payload-categories one, 
the low side is constrained by layout needed to OptiX tex2d<float4> as this 
buffer is memcpy into the texture buffer
high side is constained by not wanting to change texture line indices 

The 39 wavelength samples is historical. There is a way to increase this
to 1nm FINE_DOMAIN binning.

**/

NPY<double>* GBndLib::createBufferForTex2d()
{
    NPY<double>* mat = m_mlib->getBuffer();
    NPY<double>* sur = m_slib->getBuffer();

    LOG(LEVEL) 
        << " mat " << mat 
        << " sur " << sur
        ; 

    LOG_IF(fatal, mat == nullptr) << "NULL mat buffer" ;
    assert(mat);

    LOG_IF(warning, sur == nullptr) << "NULL sur buffer" ;
      

    unsigned int ni = getNumBnd();
    unsigned int nj = NUM_MATSUR ;    // om-os-is-im

    // the klm matches the Materials and Surface buffer layouts, so can memcpy in 
    unsigned int nk = NUM_FLOAT4 ;    
    unsigned int nl = getStandardDomainLength() ; 
    unsigned int nm = 4 ; 


    assert( nl == sdomain::COARSE_DOMAIN_LENGTH || nl == sdomain::FINE_DOMAIN_LENGTH ) ;
    bool fine = nl == sdomain::FINE_DOMAIN_LENGTH ;
    glm::vec4 dom = fine ? Opticks::GetFineDomainSpec() : Opticks::GetCoarseDomainSpec()  ;



    if( mat && sur )
    {
        assert( mat->getShape(1) == sur->getShape(1) );
        assert( mat->getShape(2) == sur->getShape(2) );
    }
    else if(mat)
    {
        assert( mat->getShape(2) == nl );
    } 
    else if(sur)
    { 
        assert( sur->getShape(1) == nk );
    }


    NPY<double>* wav = NPY<double>::make( ni, nj, nk, nl, nm) ;
    wav->fill( GSurfaceLib::SURFACE_UNSET ); 

    double domain_low = dom.x ; 
    double domain_high = dom.y ; 
    double domain_step = dom.z ; 
    double domain_range = dom.w ; 

    wav->setMeta("domain_low",   domain_low ); 
    wav->setMeta("domain_high",  domain_high ); 
    wav->setMeta("domain_step",  domain_step ); 
    wav->setMeta("domain_range", domain_range ); 



    LOG(debug) << "GBndLib::createBufferForTex2d"
               << " mat " << ( mat ? mat->getShapeString() : "NULL" )
               << " sur " << ( sur ? sur->getShapeString() : "NULL" )
               << " wav " << wav->getShapeString()
               ; 

    double* mdat = mat ? mat->getValues() : NULL ;
    double* sdat = sur ? sur->getValues() : NULL ;
    double* wdat = wav->getValues(); // destination

    for(unsigned int i=0 ; i < ni ; i++)      // over bnd
    {
        const guint4& bnd = m_bnd[i] ;
        for(unsigned j=0 ; j < nj ; j++)     // over imat/omat/isur/osur species
        {
            unsigned wof = nj*nk*nl*nm*i + nk*nl*nm*j ;

            if(j == IMAT || j == OMAT)    
            {
                unsigned midx = bnd[j] ;
                if(midx != UNSET)
                { 
                    unsigned mof = nk*nl*nm*midx ; 
                    memcpy( wdat+wof, mdat+mof, sizeof(double)*nk*nl*nm );  
                }
                else
                {
                    LOG(fatal) << "GBndLib::createBufferForTex2d"
                                 << " ERROR IMAT/OMAT with UNSET MATERIAL "
                                 << " i " << i  
                                 << " j " << j 
                                 ; 
                    assert(0);
                }
            }
            else if(j == ISUR || j == OSUR) 
            {
                unsigned sidx = bnd[j] ;
                if(sidx != UNSET)
                {
                    assert( sdat && sur );
                    unsigned sof = nk*nl*nm*sidx ;  
                    memcpy( wdat+wof, sdat+sof, sizeof(double)*nk*nl*nm );  
                }
            }

         }     // j
    }          // i
    return wav ; 
}



/**
GBndLib::createOpticalBuffer
-----------------------------

The optical buffer carries uint4 payload for every material/surface of every 
unique (omat,osur,isur,imat) boundary of the geometry. 

The optical buffer provides an easy way to access 1-based material and surface indices
starting from a texture line index. Crucially when there is no associated surface the 
surface index gives 0.  

Optical buffer can be derived from the m_bnd array of guint4. 
It contains omat-osur-isur-imat info, for materials just the material one based index, 
for surfaces the one based surface index and other optical surface parameters. 

As the optical buffer can be derived from the bnd buffer it is often not persisted.

NB creation dimensions of the optical buffer are::

   (num_bnd, NUM_MATSUR:4 , 4:unsigned ) 

But at point of use the top two dimensions are combined to give line indexing (not boundary indexing)::

   ( num_bnd*NUM_MATSUR,  4 ) 

This has been used GPU side in qsim::fill_state to give the the material and surface indices
of an intersect::

     266     const int line = boundary*_BOUNDARY_NUM_MATSUR ;      // now that are not signing boundary use 0-based
     267 
     268     const int m1_line = cosTheta > 0.f ? line + IMAT : line + OMAT ;
     269     const int m2_line = cosTheta > 0.f ? line + OMAT : line + IMAT ;
     270     const int su_line = cosTheta > 0.f ? line + ISUR : line + OSUR ;

     284     s.index.x = optical[m1_line].u.x ; // m1 index
     285     s.index.y = optical[m2_line].u.x ; // m2 index 
     286     s.index.z = optical[su_line].u.x ; // su index
     287     s.index.w = 0u ;                   // avoid undefined memory comparison issues


Q: Is there a direct geometry workflow (U4Tree/stree.h etc..) equivalent to this yet ? Where is is ?
A: NOT YET IMPLEMENTED U4Tree::initBoundary CURRENTLY EMPTY 

Note the primary thing in the optical buffer is the .x index

**/

NPY<unsigned>* GBndLib::createOpticalBuffer()
{
    bool one_based = true ; // surface and material indices 1-based, so 0 can stand for unset
    // hmm, the bnd itself is zero-based

    unsigned int ni = getNumBnd();
    unsigned int nj = NUM_MATSUR ;    // 4: om-os-is-im
    unsigned int nk = 4 ;             // 4: from uint4 payload size, NOT RELATED NUM_PROP

    NPY<unsigned>* optical = NPY<unsigned>::make( ni, nj, nk) ;
    optical->zero(); 

    unsigned* odat = optical->getValues();

    for(unsigned i=0 ; i < ni ; i++)      // over bnd
    {
        const guint4& bnd = m_bnd[i] ;

        for(unsigned j=0 ; j < nj ; j++)  // over imat/omat/isur/osur
        {
            unsigned offset = nj*nk*i+nk*j ;

            if(j == IMAT || j == OMAT)    // 0 or 3   
            {
                unsigned midx = bnd[j] ;
                assert(midx != UNSET);

                odat[offset+0] = one_based ? midx + 1 : midx  ; 
                odat[offset+1] = 0u ; 
                odat[offset+2] = 0u ; 
                odat[offset+3] = 0u ; 

            }
            else if(j == ISUR || j == OSUR)    // 1 or 2 
            {
                unsigned sidx = bnd[j] ;
                if(sidx != UNSET)
                {
                    guint4 os = m_slib->getOpticalSurface(sidx) ;

                    odat[offset+0] = one_based ? sidx + 1 : sidx  ; 
                 // TODO: enum these
                    odat[offset+1] = os.y ; 
                    odat[offset+2] = os.z ; 
                    odat[offset+3] = os.w ; 

                }
            }
        } 
    }
    return optical ; 

    // bnd indices originate during cache creation from the AssimpGGeo call of GBndLib::add with the shortnames 
    // this means that for order preferences ~/.opticks/GMaterialLib/order.json and ~/.opticks/GSurfaceLib/order.json
    // to be reflected need to rebuild the cache with ggv -G first 
}

/**

dbgtex.py optical buffer for 3 boundaries with just omat/imat set::

    Out[1]: 
    array([[[1, 0, 0, 0],
            [0, 0, 0, 0],
            [0, 0, 0, 0],
            [1, 0, 0, 0]],

           [[1, 0, 0, 0],
            [0, 0, 0, 0],
            [0, 0, 0, 0],
            [2, 0, 0, 0]],

           [[2, 0, 0, 0],
            [0, 0, 0, 0],
            [0, 0, 0, 0],
            [3, 0, 0, 0]]], dtype=uint32)

**/




void GBndLib::import()
{
    LOG(debug) << "GBndLib::import" ; 
    // does nothing as GBndLib needs dynamic buffers
}
void GBndLib::sort()
{
    LOG(debug) << "GBndLib::sort" ; 
}
void GBndLib::defineDefaults(GPropertyMap<double>* /*defaults*/)
{
    LOG(debug) << "GBndLib::defineDefaults" ; 
}
 
void GBndLib::Summary(const char* msg)
{
    unsigned int ni = getNumBnd();
    LOG(info) << msg << " NumBnd:" << ni ; 
}

void GBndLib::dump(const char* msg)
{
    LOG(info) << msg ;
    unsigned int ni = getNumBnd();
    LOG(info) << msg 
              << " ni " << ni 
               ; 

    for(unsigned int i=0 ; i < ni ; i++)
    {
        const guint4& bnd = m_bnd[i] ;
        //bnd.Summary(msg);
        std::cout << description(bnd) << std::endl ; 
    } 
}



void GBndLib::dumpBoundaries(std::vector<unsigned int>& boundaries, const char* msg)
{
    LOG(info) << msg ; 
    unsigned int nb = boundaries.size() ;
    for(unsigned int i=0 ; i < nb ; i++)
    {
        unsigned int boundary = boundaries[i];
        guint4 bnd = getBnd(boundary);
        std::cout << std::setw(3) << i 
                  << std::setw(5) << boundary 
                  << std::setw(20) << bnd.description() 
                  << " : " 
                  << description(bnd) << std::endl ; 
    }
}



void GBndLib::saveAllOverride(const char* dir)
{
    LOG(LEVEL) << "[ " << dir ;
 
    m_ok->setIdPathOverride(dir);

    save();             // only saves the guint4 bnd index
    saveToCache();      // save double buffer too for comparison with wavelength.npy from GBoundaryLib with GBndLibTest.npy 
    saveOpticalBuffer();

    m_ok->setIdPathOverride(NULL);

    LOG(LEVEL) << "]" ; 

}


