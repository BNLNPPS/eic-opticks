#pragma once
// High level view of OpIndexer
//
//     indexes history and material sequences from the sequence buffer
//     (each photon has unsigned long long 64-bit ints containg step-by-step flags and material indices)   
//     The sequences are sparse histogrammed (essentially an index of the popularity of each sequence)
//     creating a small lookup table that is then applied to all photons to create the phosel and 
//     recsel arrays containg the popularity index.
//     This index allows fast selection of all photons from the top 32 categories, this
//     can be used both graphically and in analysis.
//     The recsel buffer repeats the phosel values maxrec times to provide fast access to the
//     selection at record level.
//
//
// optixwrap-/OBuf 
//       optix buffer wrapper
// 
// thrustrap-/TBuf 
//       thrust buffer wrapper, with download to NPY interface 
//
// thrustrap-/TSparse
//       GPU Thrust sparse histogramming 
//
// cudawrap-/CBufSpec   
//       lightweight struct holding device pointer
//
// cudawrap-/CBufSlice  
//       lightweight struct holding device pointer and slice addressing begin/end
//
// cudawrap-/CResource  
//       OpenGL buffer made available as a CUDA Resource, 
//       mapGLToCUDA returns CBufSpec struct 
//
// OBuf and TBuf have slice methods that return CBufSlice structs 
// identifying a view of the GPU buffers 
//
//
// NB hostside allocation deferred for these
// note the layering here, this pattern will hopefully facilitate moving 
// from OpenGL backed to OptiX backed for COMPUTE mode
// although this isnt a good example as indexSequence is not 
// necessary in COMPUTE mode 
//
// CAUTION: the lookup resides in CUDA constant memory, not in the TSparse 
// object so must apply the lookup before making another 


#include <cstddef>
class OBuf ; 
class TBuf ; 
class NumpyEvt ; 
struct CBufSlice ; 
struct CBufSpec ; 
template <typename T> class NPY ;
template <typename T> class TSparse ;

class Timer ; 

class OpIndexer {
   public:
      OpIndexer();
      void setEvt(NumpyEvt* evt);
      void setSeq(OBuf* seq);
      void setNumPhotons(unsigned int num_photons);
   public:
      void indexSequence();
   private:
      void init();
      void updateEvt();

      void indexSequenceThrust();
      void indexSequenceThrust(const CBufSlice& seqh, const CBufSlice& seqm, bool verbose );

      void indexSequenceOptiXGLThrust();
      void indexSequenceGLThrust();
      void indexSequenceGLThrust(const CBufSlice& seqh, const CBufSlice& seqm, bool verbose );

      void indexSequenceThrust(
           TSparse<unsigned long long>& seqhis, 
           TSparse<unsigned long long>& seqmat, 
           const CBufSpec& rps,
           const CBufSpec& rrs,
           bool verbose 
      );

      void dump(const TBuf& tphosel, const TBuf& trecsel);
      void dumpHis(const TBuf& tphosel, const TSparse<unsigned long long>& seqhis) ;
      void dumpMat(const TBuf& tphosel, const TSparse<unsigned long long>& seqhis) ;
      void saveSel();
   private:
      // resident
      Timer*                   m_timer ; 
   private:
      // externally set 
      OBuf*                    m_seq ; 
      NumpyEvt*                m_evt ;
   private:
      // transients updated by updateEvt at indexSequence
      NPY<unsigned long long>* m_sequence ;
      NPY<unsigned char>*      m_recsel ;
      NPY<unsigned char>*      m_phosel ;
      unsigned int             m_maxrec ; 
      unsigned int             m_num_photons ; 

};

inline OpIndexer::OpIndexer()  
   :
     m_seq(NULL),
     m_evt(NULL),
     m_sequence(NULL),
     m_recsel(NULL),
     m_phosel(NULL),
     m_maxrec(0),
     m_num_photons(0)
{
   init(); 
}

inline void OpIndexer::setSeq(OBuf* seq)
{
    m_seq = seq ; 
}




