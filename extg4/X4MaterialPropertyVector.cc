#include "G4MaterialPropertyVector.hh"
#include "X4MaterialPropertyVector.hh"
#include "NPY.hpp"
#include "NP.hh"
#include "G4Version.hh"


G4double X4MaterialPropertyVector::GetMinLowEdgeEnergy( const G4MaterialPropertyVector* mpv ) // static 
{
#if G4VERSION_NUMBER < 1100
    return const_cast<G4MaterialPropertyVector*>(mpv)->GetMinLowEdgeEnergy(); 
#else
    return 0. ;   // ??
#endif   
}

G4double X4MaterialPropertyVector::GetMinLowEdgeEnergy() const
{
    return GetMinLowEdgeEnergy(vec); 
}

G4double X4MaterialPropertyVector::GetMaxLowEdgeEnergy( const G4MaterialPropertyVector* mpv ) // static 
{
#if G4VERSION_NUMBER < 1100
    return const_cast<G4MaterialPropertyVector*>(mpv)->GetMaxLowEdgeEnergy(); 
#else
    return 0. ;   // ??
#endif   
}

G4double X4MaterialPropertyVector::GetMaxLowEdgeEnergy() const
{
    return GetMaxLowEdgeEnergy(vec); 
}




G4bool X4MaterialPropertyVector::IsFilledVectorExist( const G4MaterialPropertyVector* mpv ) // static 
{
#if G4VERSION_NUMBER < 1100
    return mpv->IsFilledVectorExist() ; 
#else
    return mpv->GetVectorLength() > 0 ; 
#endif   
}

G4bool X4MaterialPropertyVector::IsFilledVectorExist() const 
{
    return IsFilledVectorExist(vec); 
}





G4MaterialPropertyVector* X4MaterialPropertyVector::FromArray(const NP* a ) // static 
{
    assert( a->uifc == 'f' && a->ebyte == 8 ); 

    size_t ni = a->shape[0] ;  
    size_t nj = a->shape[1] ;  
    assert( nj == 2 ); 

    G4double* energy = new G4double[ni] ;
    G4double* value = new G4double[ni] ;
 
    for(int i=0 ; i < int(ni) ; i++)
    {
        energy[i] = a->get<double>(i,0) ; 
        value[i] = a->get<double>(i,1) ; 
    }
    G4MaterialPropertyVector* vec = new G4MaterialPropertyVector(energy, value, ni);  
    return vec ; 
}


G4MaterialPropertyVector* X4MaterialPropertyVector::FromArray(const NPY<double>* arr) // static
{
    size_t ni = arr->getShape(0); 
    size_t nj = arr->getShape(1); 
    assert( nj == 2 ); 

    G4double* energy = new G4double[ni] ;
    G4double* value = new G4double[ni] ;
 
    for(int i=0 ; i < int(ni) ; i++)
    {
        energy[i] = arr->getValue(i, 0, 0 ); 
        value[i] = arr->getValue(i, 1, 0); 
    }

    G4MaterialPropertyVector* vec = new G4MaterialPropertyVector(energy, value, ni);  
    return vec ; 
}



X4MaterialPropertyVector::X4MaterialPropertyVector(const G4MaterialPropertyVector* vec_ )
    :
    vec(vec_)
{
}

template <typename T> NPY<T>* X4MaterialPropertyVector::convert() 
{
    return Convert<T>(vec); 
}

template <typename T> NPY<T>* X4MaterialPropertyVector::Convert(const G4MaterialPropertyVector* vec)   // static
{
    size_t num_val = vec->GetVectorLength() ; 
    NPY<T>* a = NPY<T>::make( num_val, 2 );  
    a->zero(); 
    int k=0 ; 
    int l=0 ; 
    for(size_t i=0 ; i < num_val ; i++)
    {   
        G4double energy = vec->Energy(i); 
        G4double value = (*vec)[i] ; 
        a->setValue(i, 0, k, l,  T(energy) );  
        a->setValue(i, 1, k, l,  T(value) );  
    }   
    return a ;   
}


NP* X4MaterialPropertyVector::ConvertToArray(const G4MaterialPropertyVector* v)   // static
{
    size_t num_val = v->GetVectorLength() ; 
    NP* a = NP::Make<double>( num_val, 2 );  
    double* aa = a->values<double>(); 
    for(size_t i=0 ; i < num_val ; i++)
    {   
        aa[2*i+0] = v->Energy(i) ; 
        aa[2*i+1] = (*v)[i] ; 
    }   
    return a ;  
}



template NPY<float>*  X4MaterialPropertyVector::convert() ; 
template NPY<double>* X4MaterialPropertyVector::convert() ; 

template NPY<float>*  X4MaterialPropertyVector::Convert(const G4MaterialPropertyVector* vec) ; 
template NPY<double>* X4MaterialPropertyVector::Convert(const G4MaterialPropertyVector* vec) ; 

