#include <stdio.h>

#include "scuda.h"
#include "squad.h"
#include "srec.h"
#include "sphoton.h"
#include "sevent.h"

#include "iexpand.h"
#include "strided_range.h"
#include <thrust/device_vector.h>

/**
_QEvent_checkEvt
-----------------

Demonstrates using seed buffer to lookup genstep_id from photon_id 

**/

__global__ void _QEvent_checkEvt(sevent* evt, unsigned width, unsigned height)
{
    unsigned ix = blockIdx.x*blockDim.x + threadIdx.x;
    if( ix >= width ) return ;  

    unsigned photon_id = ix ; 
    unsigned genstep_id = evt->seed[photon_id] ; 
    const quad6& gs = evt->genstep[genstep_id] ; 
    int gencode = gs.q0.i.x ; 
    unsigned num_photon = evt->num_photon ; 

    printf("//_QEvent_checkEvt width %d height %d photon_id %3d genstep_id %3d  gs.q0.i ( %3d %3d %3d %3d )  gencode %d num_photon %d \n", 
       width,
       height,
       photon_id, 
       genstep_id, 
       gs.q0.i.x, 
       gs.q0.i.y,
       gs.q0.i.z, 
       gs.q0.i.w,
       gencode, 
       num_photon 
      );  
}

extern "C" void QEvent_checkEvt(dim3 numBlocks, dim3 threadsPerBlock, sevent* evt, unsigned width, unsigned height ) 
{
    printf("//QEvent_checkEvt width %d height %d \n", width, height );  
    _QEvent_checkEvt<<<numBlocks,threadsPerBlock>>>( evt, width, height  );
} 

/**
QEvent_count_genstep_photons
-------------------------------

NB this needs nvcc compilation due to the use of thrust but 
the method itself does not run on the device although the 
methods it invokes do run on the device. 

So the sevent* argument must be the CPU side instance 
which must be is holding GPU side pointers.

**/


//#ifdef DEBUG_QEVENT
struct printf_functor
{
    __host__ __device__ void operator()(int x){ printf("printf_functor %d\n", x); }
};
//#endif


/**
QEvent_count_genstep_photons
-----------------------------

Notice how using strided_range needs itemsize stride twice, 
because are grabbing single ints "numphoton" from each quad6 6*4 genstep 

**/


extern "C" unsigned QEvent_count_genstep_photons(sevent* evt)
{
    typedef typename thrust::device_vector<int>::iterator Iterator;

    thrust::device_ptr<int> t_gs = thrust::device_pointer_cast( (int*)evt->genstep ) ; 

#ifdef DEBUG_QEVENT
    printf("//QEvent_count_genstep_photons sevent::genstep_numphoton_offset %d  sevent::genstep_itemsize  %d  \n", 
            sevent::genstep_numphoton_offset, sevent::genstep_itemsize ); 
#endif

    strided_range<Iterator> gs_pho( 
        t_gs + sevent::genstep_numphoton_offset, 
        t_gs + evt->num_genstep*sevent::genstep_itemsize , 
        sevent::genstep_itemsize );    // begin, end, stride 

    evt->num_seed = thrust::reduce(gs_pho.begin(), gs_pho.end() );

#ifdef DEBUG_QEVENT
    //thrust::for_each( gs_pho.begin(), gs_pho.end(), printf_functor() );  
    printf("//QEvent_count_genstep_photons evt.num_genstep %d evt.num_seed %d evt.max_photon %d \n", evt->num_genstep, evt->num_seed, evt->max_photon ); 
#endif
    assert( evt->num_seed <= evt->max_photon ); 

    return evt->num_seed ; 
} 

/**
QEvent_fill_seed_buffer
-------------------------

Populates seed buffer using the numbers of photons per genstep from the genstep buffer.

See thrustrap/tests/iexpand_stridedTest.cu for the lead up to this

1. use GPU side genstep array to add the numbers of photons
   from each genstep giving the total number of photons and seeds *num_seeds*
   from all the gensteps

2. populate it by repeating genstep indices into it, 
   according to the number of photons in each genstep 
   
t_gs+sevent::genstep_numphoton_offset 
   q0.u.w of the quad6 genstep, which contains the number of photons 
   for this genstep


WARNING : SOMETHING HERE MESSES UP UNLESS THE SEED BUFFER IS ZEROED PRIOR TO THIS BEING CALLED

ACTUALLY THIS IS DUE TO A A LIMITATION OF IEXPAND, see sysrap/iexpand.h::

    NB the output device must be zeroed prior to calling iexpand. 
    This is because the iexpand is implemented ending with an inclusive_scan 
    to fill in the non-transition values which relies on initial zeroing.

**/

extern "C" void QEvent_fill_seed_buffer(sevent* evt )
{
#ifdef DEBUG_QEVENT
    printf("//QEvent_fill_seed_buffer evt.num_genstep %d evt.num_seed %d evt.max_photon %d \n", evt->num_genstep, evt->num_seed, evt->max_photon );      
#endif

    assert( evt->seed && evt->num_seed > 0 ); 
    assert( evt->num_seed <= evt->max_photon ); 

    thrust::device_ptr<int> t_seed = thrust::device_pointer_cast(evt->seed) ; 

    typedef typename thrust::device_vector<int>::iterator Iterator;

    thrust::device_ptr<int> t_gs = thrust::device_pointer_cast( (int*)evt->genstep ) ; 

    strided_range<Iterator> gs_pho( 
           t_gs + sevent::genstep_numphoton_offset, 
           t_gs + evt->num_genstep*sevent::genstep_itemsize, 
           sevent::genstep_itemsize );    // begin, end, stride 


    //thrust::for_each( gs_pho.begin(), gs_pho.end(), printf_functor() );  

    iexpand( gs_pho.begin(), gs_pho.end(), t_seed, t_seed + evt->num_seed );  

    //thrust::for_each( t_seed,  t_seed + evt->num_seed, printf_functor() );  

}



/**
QEvent_count_genstep_photons_and_fill_seed_buffer
---------------------------------------------------

This function does the same as the above two functions. 
It is invoked from QEvent::setGenstep

**/

extern "C" void QEvent_count_genstep_photons_and_fill_seed_buffer(sevent* evt )
{
    typedef typename thrust::device_vector<int>::iterator Iterator;

    thrust::device_ptr<int> t_gs = thrust::device_pointer_cast( (int*)evt->genstep ) ; 

#ifdef DEBUG_QEVENT
    printf("//QEvent_count_genstep_photons sevent::genstep_numphoton_offset %d  sevent::genstep_itemsize  %d  \n", 
            sevent::genstep_numphoton_offset, sevent::genstep_itemsize ); 
#endif


    strided_range<Iterator> gs_pho( 
        t_gs + sevent::genstep_numphoton_offset, 
        t_gs + evt->num_genstep*sevent::genstep_itemsize , 
        sevent::genstep_itemsize );    // begin, end, stride 

    evt->num_seed = thrust::reduce(gs_pho.begin(), gs_pho.end() );

#ifdef DEBUG_QEVENT
    printf("//QEvent_count_genstep_photons_and_fill_seed_buffer evt.num_genstep %d evt.num_seed %d evt.max_photon %d \n", evt->num_genstep, evt->num_seed, evt->max_photon );      
#endif

    bool expect_seed =  evt->seed && evt->num_seed > 0 ; 
    if(!expect_seed) printf("//QEvent_count_genstep_photons_and_fill_seed_buffer  evt.seed %s  evt.num_seed %d \n",  (evt->seed ? "YES" : "NO " ), evt->num_seed );  
    assert( expect_seed ); 

    bool num_seed_ok = evt->num_seed <= evt->max_photon ;

    if( num_seed_ok == false )
    {
        printf("//QEvent_count_genstep_photons_and_fill_seed_buffer FAIL evt.num_seed %d evt.max_photon %d num_seed_ok %d \n", evt->num_seed, evt->max_photon, num_seed_ok  ); 
    }

    assert( num_seed_ok ); 

    thrust::device_ptr<int> t_seed = thrust::device_pointer_cast(evt->seed) ; 

    //thrust::for_each( gs_pho.begin(), gs_pho.end(), printf_functor() );  

#ifdef DEBUG_QEVENT
    printf("//[QEvent_count_genstep_photons_and_fill_seed_buffer iexpand \n" );      
#endif


    iexpand( gs_pho.begin(), gs_pho.end(), t_seed, t_seed + evt->num_seed );  

    //thrust::for_each( t_seed,  t_seed + evt->num_seed, printf_functor() );  


#ifdef DEBUG_QEVENT
    printf("//]QEvent_count_genstep_photons_and_fill_seed_buffer iexpand \n" );      
#endif



}




