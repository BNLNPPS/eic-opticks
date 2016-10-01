#pragma once

#include <string>
#include <map>

class OpticksHub ; 
class GGeo ; 
class GSurLib ; 
class CSurLib ; 
class CG4 ; 
class Opticks ; 
template <typename T> class OpticksCfg ; 
class CDetector ; 
class CPropLib ; 
class CMaterialTable ; 
class CMaterialBridge ; 


#include "CFG4_API_EXPORT.hh"

class CFG4_API CGeometry 
{
   public:
       CGeometry(OpticksHub* hub);
       bool hookup(CG4* g4);
       void postinitialize();   // invoked by CG4::postinitialize after Geant4 geometry constructed
       CPropLib*  getPropLib();
       CDetector* getDetector();
       CMaterialBridge* getMaterialBridge();
       std::map<std::string, unsigned>& getMaterialMap();        
   private:
       void init();
   private:
       OpticksHub*          m_hub ; 
       GGeo*                m_ggeo ; 
       GSurLib*             m_surlib ; 
       CSurLib*             m_csurlib ; 

       Opticks*             m_ok ; 
       OpticksCfg<Opticks>* m_cfg ; 
       CDetector*           m_detector ; 
       CPropLib*            m_lib ; 
       CMaterialTable*      m_material_table ; 
       CMaterialBridge*     m_material_bridge ; 

};



