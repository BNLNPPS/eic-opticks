#pragma once
/**
s_mock_curand.h
=================

Mocking *curand_uniform* enables code developed for use 
with the standard CUDA *curand_uniform* to be tested on CPU without change, 
other than switching headers. 

TODO: 
   provide option to hook into the precooked randoms (see SRngSpec)
   so the "generated" values actually match curand_uniform on device  
   
   * this requires setting an index like OpticksRandom.hh does 

HMM: instanciation API does not match the real one, does that matter ?

**/

#include <random>

struct curandStateXORWOW
{
    std::mt19937_64 engine ;

    std::uniform_real_distribution<float>   fdist ; 
    std::uniform_real_distribution<double>  ddist ; 
    double fake ; 

    curandStateXORWOW(unsigned seed_); 

    void set_fake(double fake_); 
    float  generate_float(); 
    double generate_double(); 

}; 


inline curandStateXORWOW::curandStateXORWOW(unsigned seed_) 
    : 
    fdist(0,1), 
    ddist(0,1),
    fake(-1.)
{ 
    engine.seed(seed_) ; 
}

inline void curandStateXORWOW::set_fake(double fake_){ fake = fake_ ; } 
inline float  curandStateXORWOW::generate_float(){  return fake >= 0.f ? fake : fdist(engine) ; } 
inline double curandStateXORWOW::generate_double(){ return fake >= 0. ?  fake : ddist(engine) ; } 


typedef curandStateXORWOW curandState_t ; 
float curand_uniform(curandState_t* state ){         return state->generate_float() ; }
double curand_uniform_double(curandState_t* state ){ return state->generate_double() ; }



