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

#pragma once

#include <string>
#include <vector>
#include <map>


template <typename T> class GPropertyMap ; 
template <typename T> class GProperty ; 
template <typename T> class GDomain ; 

class BMeta ; 
class GOpticalSurface ; 

#include "plog/Severity.h"

#include "GGEO_API_EXPORT.hh"
#include "GGEO_HEAD.hh"

/**

GPropertyMap<T>
==================

1. manages m_prop : a string keyed map of GProperty<T>


TODO: const correctness would be good, although painful to implement

**/

template <class T>
class GGEO_API GPropertyMap {

  static const plog::Severity LEVEL ;
  static const char* EFFICIENCY ;
  static const char* detect ;
  static const char* NOT_DEFINED ;
  typedef std::map<std::string,GProperty<T>*> GPropertyMap_t ;

  public:
      GPropertyMap(GPropertyMap* other, GDomain<T>* domain=NULL);  // used for interpolation when domain provided
      GPropertyMap(const char* name);
      GPropertyMap(const char* name, unsigned int index, const char* type, GOpticalSurface* optical_surface=NULL, BMeta* meta=NULL);

      virtual ~GPropertyMap();
  private:
      void init();
      void collectMeta();
  public:
      void dumpMeta(const char* msg="GPropertyMap::dumpMeta") const ;
      void save(const char* path);
      static GPropertyMap<T>* load(const char* path, const char* name, const char* type);
  public:
     // caller should free the char* returned after dumping 
      //char* ndigest();
      char* pdigest(int ifr, int ito) const ; 
      const char* getShortName() const ; 
      bool hasShortName(const char* name);
      bool hasDefinedName();
      bool hasNameEnding(const char* end);

      BMeta* getMeta() const ; 
      std::string getMetaDesc() const ; 

      template <typename S> 
      void setMetaKV(const char* key, S value);

      template <typename S> 
      S getMetaKV(const char* key, const char* fallback) const ;   

      bool hasMetaItem(const char* key ) const ;


      std::string getShortNameString() const ;
      std::string getPDigestString(int ifr, int ito) const ;
      std::string getKeysString() const ; 
      std::string description() const ;
      std::string prop_desc() const ;
  public:
      std::string desc_table() const ; 
      std::string make_table(unsigned int fwid=20, T dscale=1, bool dreciprocal=false);
  public:
      GPropertyMap<T>* spawn_interpolated(T nm=1.0f);
  public:
      static const char* FindShortName(const char* name, const char* prefix0, const char* prefix1 );
  private:
      // formerly prefix was defaulted to "__dd__Materials__" some old prefix
      void findShortName(const char* prefix=nullptr);

  public:
      std::string brief() const ; 
      std::string desc() const ; 
      const char* getName() const ;    // names like __dd__Materials__Nylon0xc3aa360 or __dd__Geometry__AdDetails__AdSurfacesNear__SSTWaterSurfaceNear2
      unsigned getIndex() const ;  // aiScene material index ("surfaces" and "materials" represented as Assimp materials)
      void setIndex(unsigned index);

      const char* getType() const ;


      void setSkinSurface(); 
      void setBorderSurface(); 

      void setImplicit(bool implicit); 
      bool isImplicit() const ; 

      bool isSurface() const ;
      bool isTestSurface() const ;
      bool isSkinSurface() const ;
      bool isBorderSurface() const ;

      bool isMaterial() const ;
      bool hasNonZeroProperty(const char* pname) ;

      void setOriginalDomain(); 
      bool hasOriginalDomain() const ; 
   public:
      // from metadata
      std::string getBPV1() const ; 
      std::string getBPV2() const ; 
      std::string getSSLV() const ; 
  public:

      void setSensor(bool sensor=true); // set in AssimpGGeo::convertSensors
      bool isSensor();  // non-zero "EFFICIENCY" or "detect" property

      void setValid(bool valid=true);
      bool isValid();

      void setOpticalSurface(GOpticalSurface* optical_surface);
      GOpticalSurface* getOpticalSurface(); 


      void dump(const char* msg="GPropertyMap::Summary", unsigned int nline=1);
      void Summary(const char* msg="GPropertyMap::Summary", unsigned int nline=1) const ;

  public:
      bool hasStandardDomain();
      void setStandardDomain(GDomain<T>* standard_domain=NULL);  // default of NULL -> default domain
      GDomain<T>* getStandardDomain();
      T getDomainLow();
      T getDomainHigh();
      T getDomainStep();

  public:
      bool setPropertyValues(const char* pname, T val); 
  public:
      void addConstantProperty(const char* pname, T value, const char* prefix=NULL);

      // when a standard domain is defined these methods interpolates the values provided onto that domain
      void addPropertyStandardized(const char* pname,  GProperty<T>* orig, const char* prefix=NULL);

       // this one does not interpolate  
      void addPropertyAsis(const char* pname, GProperty<T>* prop, const char* prefix=NULL);

   private:
      friend struct GMaterialTest ; 
      void addPropertyStandardized(const char* pname, T* values, T* domain, unsigned int length, const char* prefix=NULL);
  public:
      // adding other map of properties
      void addMapStandardized(GPropertyMap<T>* other, const char* prefix=NULL);
      void addMapAsis(        GPropertyMap<T>* other, const char* prefix=NULL);
  public:
      unsigned                  getNumProperties() const ;
      GProperty<T>*             getPropertyByIndex(int index) const ;
      const char*               getPropertyNameByIndex(int index) const ;
      GProperty<T>*             getProperty(const char* pname) const ;
      const GProperty<T>*       getPropertyConst(const char* pname) const  ;
      GProperty<T>*             getProperty(const char* pname, const char* prefix);
      bool                      hasProperty(const char* pname) const ;
      std::vector<std::string>& getKeys() ;
      unsigned                  size() const ; 
      std::string                dump_ptr() const ; 
  private:
      std::string m_name ;
      const char* m_shortname ; 
      std::string m_type ;

      unsigned int m_index ;
      bool         m_valid ;  

      GPropertyMap_t           m_prop ; 
      std::vector<std::string> m_keys ;  // key ordering

      GDomain<T>*      m_standard_domain ; 
      GOpticalSurface* m_optical_surface ; 
      BMeta*           m_meta ; 
      bool             m_original_domain ; 

};


#include "GGEO_TAIL.hh"

