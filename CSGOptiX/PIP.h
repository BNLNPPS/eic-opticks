#pragma once
/**
PIP : OptiX 7 Ray Trace Program Pipeline 
==========================================

Aiming to keep this geometry independent 

This is used by CSGOptiX.cc and SBT.cc

**/

#include "plog/Severity.h"

struct PIP
{
    static const plog::Severity LEVEL ; 
    static const int MAX_TRACE_DEPTH ; 

    unsigned max_trace_depth ; 
    unsigned num_payload_values ; 
    unsigned num_attribute_values ; 

    OptixPipelineCompileOptions pipeline_compile_options = {};
    OptixProgramGroupOptions program_group_options = {};

    OptixModule module = nullptr;

    OptixProgramGroup raygen_pg   = nullptr;
    OptixProgramGroup miss_pg     = nullptr;
    OptixProgramGroup hitgroup_pg = nullptr;

    OptixPipeline pipeline = nullptr;


    static bool OptiXVersionIsSupported(); 
    static OptixCompileDebugLevel        DebugLevel(const char* option); 
    static const char *                  DebugLevel_( OptixCompileDebugLevel debugLevel ); 

#if OPTIX_VERSION == 70000
    static constexpr const char* OPTIX_COMPILE_DEBUG_LEVEL_NONE_     = "OPTIX_COMPILE_DEBUG_LEVEL_NONE" ; 
    static constexpr const char* OPTIX_COMPILE_DEBUG_LEVEL_LINEINFO_ = "OPTIX_COMPILE_DEBUG_LEVEL_LINEINFO" ; 
    static constexpr const char* OPTIX_COMPILE_DEBUG_LEVEL_FULL_     = "OPTIX_COMPILE_DEBUG_LEVEL_FULL" ; 
#elif OPTIX_VERSION == 70500
    static constexpr const char* OPTIX_COMPILE_DEBUG_LEVEL_DEFAULT_  = "OPTIX_COMPILE_DEBUG_LEVEL_DEFAULT" ; 
    static constexpr const char* OPTIX_COMPILE_DEBUG_LEVEL_NONE_     = "OPTIX_COMPILE_DEBUG_LEVEL_NONE" ; 
    static constexpr const char* OPTIX_COMPILE_DEBUG_LEVEL_MINIMAL_  = "OPTIX_COMPILE_DEBUG_LEVEL_MINIMAL" ; 
    static constexpr const char* OPTIX_COMPILE_DEBUG_LEVEL_MODERATE_ = "OPTIX_COMPILE_DEBUG_LEVEL_MODERATE" ; 
    static constexpr const char* OPTIX_COMPILE_DEBUG_LEVEL_FULL_     = "OPTIX_COMPILE_DEBUG_LEVEL_FULL" ; 
#endif

    static OptixCompileOptimizationLevel OptimizationLevel(const char* option) ; 
    static const char* OptimizationLevel_( OptixCompileOptimizationLevel optLevel ); 
    static constexpr const char* OPTIX_COMPILE_OPTIMIZATION_DEFAULT_ = "OPTIX_COMPILE_OPTIMIZATION_DEFAULT" ;
    static constexpr const char* OPTIX_COMPILE_OPTIMIZATION_LEVEL_0_ = "OPTIX_COMPILE_OPTIMIZATION_LEVEL_0" ;
    static constexpr const char* OPTIX_COMPILE_OPTIMIZATION_LEVEL_1_ = "OPTIX_COMPILE_OPTIMIZATION_LEVEL_1" ;
    static constexpr const char* OPTIX_COMPILE_OPTIMIZATION_LEVEL_2_ = "OPTIX_COMPILE_OPTIMIZATION_LEVEL_2" ; 
    static constexpr const char* OPTIX_COMPILE_OPTIMIZATION_LEVEL_3_ = "OPTIX_COMPILE_OPTIMIZATION_LEVEL_3" ;


    static OptixExceptionFlags           ExceptionFlags_(const char* opt) ; 
    static unsigned                      ExceptionFlags(const char* options); 
    static const char*                   ExceptionFlags__(OptixExceptionFlags excFlag) ; 
    static const char* OPTIX_EXCEPTION_FLAG_NONE_ ;
    static const char* OPTIX_EXCEPTION_FLAG_STACK_OVERFLOW_ ;
    static const char* OPTIX_EXCEPTION_FLAG_TRACE_DEPTH_ ;
    static const char* OPTIX_EXCEPTION_FLAG_USER_ ;
    static const char* OPTIX_EXCEPTION_FLAG_DEBUG_ ;


    static const char*                 CreatePipelineOptions_exceptionFlags ; 
    static OptixPipelineCompileOptions CreatePipelineOptions(unsigned numPayloadValues, unsigned numAttributeValues );
    static OptixProgramGroupOptions CreateProgramGroupOptions();

    static const char* CreateModule_debugLevel ; 
    static const char* CreateModule_optLevel ; 
    static OptixModule CreateModule(const char* ptx_path, OptixPipelineCompileOptions& pipeline_compile_options );

    PIP(const char* ptx_path_); 
    const char* desc() const ; 

    void init(); 
    void createRaygenPG(const char* rg);
    void createMissPG(const char* ms);
    void createHitgroupPG(const char* is, const char* ch, const char* ah );

    static const char* linkPipeline_debugLevel ; 
    void linkPipeline(unsigned max_trace_depth);
    void configureStack(); 

}; 


