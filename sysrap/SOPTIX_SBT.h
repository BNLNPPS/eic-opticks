#pragma once
/**
SOPTIX_SBT.h
==================

Good explanatiom of SBT Shader Binding Table

* https://www.willusher.io/graphics/2019/11/20/the-sbt-three-ways

**/

#include "SOPTIX_Binding.h"

struct SOPTIX_SBT
{ 
    SOPTIX_Pipeline& pip ;     

    CUdeviceptr   d_raygen ;
    CUdeviceptr   d_miss ;
    CUdeviceptr   d_hitgroup ;

    OptixShaderBindingTable sbt = {};  

    SOPTIX_SBT( SOPTIX_Pipeline& pip ); 

    void init(); 
    void initRaygen(); 
    void initMiss(); 
    void initHitgroup(); 
};


inline SOPTIX_SBT::SOPTIX_SBT(
       SOPTIX_Pipeline& _pip
    )
    :
    pip(_pip)
{
    init(); 
}
 
inline void SOPTIX_SBT::init()
{
    initRaygen(); 
    initMiss(); 
    initHitgroup(); 
}

inline void SOPTIX_SBT::initRaygen()
{
    CUdeviceptr  raygen_record ; 
   
    const size_t raygen_record_size = sizeof( SOPTIX_RaygenRecord );
    CUDA_CHECK( cudaMalloc( reinterpret_cast<void**>( &raygenRecord ), raygen_record_size ) );
    
    SOPTIX_RaygenRecord rg_sbt;
    OPTIX_CHECK( optixSbtRecordPackHeader( pip.raygen_pg, &rg_sbt ) );

    CUDA_CHECK( cudaMemcpy(
                reinterpret_cast<void*>(raygen_record),
                &rg_sbt,
                raygen_record_size,
                cudaMemcpyHostToDevice
                ) );

    sbt.raygenRecord = raygen_record ; 

}

inline void SOPTIX_SBT::initMiss()
{
    CUdeviceptr miss_record ;

    const size_t miss_record_size = sizeof( SOPTIX_MissRecord );
    CUDA_CHECK( cudaMalloc( reinterpret_cast<void**>( &miss_record ), miss_record_size ) );
    
    SOPTIX_MissRecord ms_sbt;
    OPTIX_CHECK( optixSbtRecordPackHeader( pip.miss_pg, &ms_sbt ) );

    CUDA_CHECK( cudaMemcpy(
                reinterpret_cast<void*>( miss_record ),
                &ms_sbt,
                miss_record_size,
                cudaMemcpyHostToDevice
                ) );

    sbt.missRecordBase = miss_record;
    sbt.missRecordStrideInBytes = static_cast<uint32_t>( miss_record_size );
    sbt.missRecordCount = 1 ; 
}


/**
SOPTIX_SBT::initHitgroup
-------------------------

**/

inline void SOPTIX_SBT::initHitgroup()
{
    std::vector<SOPTIX_HitgroupRecord> hitgroup_records;

    SOPTIX_HitgroupRecord hg_sbt;
    OPTIX_CHECK( optixSbtRecordPackHeader( pip.hitgroup_pg, &hg_sbt ) );

    hitgroup_records.push_back(hg_sbt);






    CUdeviceptr hitgroup_record_base ;
    const size_t hitgroup_record_size = sizeof( SOPTIX_HitgroupRecord );
    CUDA_CHECK( cudaMalloc( 
                reinterpret_cast<void**>( &hitgroup_record_base ), 
                hitgroup_record_size*hitgroup_records.size() ) );

    CUDA_CHECK( cudaMemcpy(
                reinterpret_cast<void*>( hitgroup_record_base ),
                hitgroup_records.data(),
                hitgroup_record_size*hitgroup_records.size(),
                cudaMemcpyHostToDevice
                ) );

    sbt.hitgroupRecordBase = hitgroup_record_base ; 
    sbt.hitgroupRecordStrideInBytes = static_cast<uint32_t>( hitgroup_record_size );
    sbt.hitgroupRecordCount         = static_cast<uint32_t>( hitgroup_records.size() );
}


/**
SOPTIX_SBT::initHitgroup_no_instance
--------------------------------------

**/

inline void SOPTIX_SBT::initHitgroup_no_instance()
{
    CUdeviceptr hitgroup_record;
    const size_t hitgroup_record_size = sizeof( SOPTIX_HitgroupRecord );
    CUDA_CHECK( cudaMalloc( reinterpret_cast<void**>( &hitgroup_record ), hitgroup_record_size ) );

    SOPTIX_HitgroupRecord hg_sbt;
    OPTIX_CHECK( optixSbtRecordPackHeader( pip.hitgroup_pg, &hg_sbt ) );

    CUDA_CHECK( cudaMemcpy(
                reinterpret_cast<void*>( hitgroup_record ),
                &hg_sbt,
                hitgroup_record_size,
                cudaMemcpyHostToDevice
                ) );

    sbt.hitgroupRecordBase = hitgroup_record;
    sbt.hitgroupRecordStrideInBytes = static_cast<uint32_t>( hitgroup_record_size ); 
    sbt.hitgroupRecordCount = 1; 
}



