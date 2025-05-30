
#include <csignal>
#include "scuda.h"
#include "sqat4.h"
#include "ssys.h"

#ifdef WITH_S_BB
#include "s_bb.h"
#else
#include "saabb.h"
#endif

#include "SLOG.hh"

#include "SBitSet.h"
#include "OpticksCSG.h"

#include "CSGFoundry.h"
#include "CSGSolid.h"
#include "CSGPrim.h"
#include "CSGNode.h"

#include "CSGCopy.h"


const plog::Severity CSGCopy::LEVEL = SLOG::EnvLevel("CSGCopy", "DEBUG" ); 
const int CSGCopy::DUMP_RIDX = ssys::getenvint("DUMP_RIDX", -1) ; 
const int CSGCopy::DUMP_NPS = ssys::getenvint("DUMP_NPS", 0) ; 



CSGFoundry* CSGCopy::Clone(const CSGFoundry* src )
{
    CSGCopy cpy(src, nullptr); 
    cpy.copy(); 
    LOG(info) << cpy.desc(); 
    return cpy.dst ; 
}

/**
CSGCopy::Select
------------------

Used from CSGFoundry::CopySelect 

Q: How to detect a "no-selection" SBitSet ? 
A: SBitSet::is_all_set indicates all bits are set, hence no selection

**/

CSGFoundry* CSGCopy::Select(const CSGFoundry* src, const SBitSet* elv )
{
    CSGCopy cpy(src, elv); 
    cpy.copy(); 
    LOG(LEVEL) << cpy.desc(); 
    return cpy.dst ; 
}

CSGCopy::CSGCopy(const CSGFoundry* src_, const SBitSet* elv_)
    :
    src(src_),
    sNumSolid(src->getNumSolid()),
    solidMap(new int[sNumSolid]), 
    sSolidIdx(~0u), 
    elv(elv_),
    identical(elv ? elv->is_all_set() : true),
    identical_bbox_cheat(identical && true),
    dst(new CSGFoundry)
{
}

std::string CSGCopy::desc() const 
{
    std::stringstream ss ; 
    ss 
        << std::endl 
        << "src:" 
        << src->desc() 
        << std::endl 
        << "dst:" 
        << dst->desc()
        << std::endl 
        ; 
    std::string s = ss.str(); 
    return s ; 
}

CSGCopy::~CSGCopy()
{
    delete [] solidMap ; 
}

unsigned CSGCopy::Dump( unsigned sSolidIdx )
{
    return ( DUMP_RIDX >= 0 && unsigned(DUMP_RIDX) == sSolidIdx ) ? DUMP_NPS : 0u ; 
}

/**
CSGCopy::copy
--------------

The point of cloning is to provide an easily verifiable starting point 
to implementing Prim selection so must not descend to very low level cloning.

* this has some similarities with CSGFoundry::addDeepCopySolid and CSG_GGeo/CSG_GGeo_Convert.cc

* Blind copying makes no sense as offsets will be different with selection  
  only some metadata referring back to the GGeo geometry is appropriate for copying

* When selecting numPrim will sometimes be less in dst, sometimes even zero, 
  so need to count selected Prim before adding the solid.

* ALSO selections can change the number of solids 

**/

void CSGCopy::copy()
{ 
    CSGFoundry::CopyNames(dst, src );  
    // No accounting for selection changing the number of names
    // as these are LV level names. The LV idx are regarded as 
    // external and unchanged no matter the selection, unlike gas_idx. 
 
    for(unsigned i=0 ; i < sNumSolid ; i++)
    {
        sSolidIdx = i ; 
        solidMap[sSolidIdx] = -1 ; 

        unsigned dump_ = Dump(sSolidIdx); 
        bool dump_solid = dump_ & 0x1 ; 
        LOG_IF(info, dump_solid) 
            << "sSolidIdx " << sSolidIdx 
            << " DUMP_RIDX " << DUMP_RIDX  
            << " DUMP_NPS " << DUMP_NPS 
            << " dump_solid " << dump_solid  
            ;   

        const CSGSolid* sso = src->getSolid(sSolidIdx);
        unsigned numSelectedPrim = src->getNumSelectedPrimInSolid(sso, elv );  
        const std::string& solidMMLabel = src->getSolidMMLabel(sSolidIdx); 

        char sIntent = sso->getIntent(); 

        LOG(LEVEL) 
            << " sSolidIdx/sNumSolid/numSelectedPrim"
            << std::setw(2) << sSolidIdx 
            << "/" 
            << std::setw(2) << sNumSolid 
            << "/" 
            << std::setw(4) << numSelectedPrim 
            << " [" << sIntent << "] "
            << ": "
            << solidMMLabel
            ;

        LOG_IF(LEVEL, dump_solid) << " sso " << sso->desc() << " numSelectedPrim " << numSelectedPrim << " solidMMLabel " << solidMMLabel ; 

        if( numSelectedPrim == 0 ) continue ;  

        dst->addSolidMMLabel( solidMMLabel.c_str() );  

        unsigned dSolidIdx = dst->getNumSolid() ; // index before adding (0-based)
        if( elv == nullptr ) assert( dSolidIdx == sSolidIdx ); 

        CSGSolid* dso = dst->addSolid(numSelectedPrim, sso->label );   
        int dPrimOffset = dso->primOffset ;       
        assert( dPrimOffset == int(dst->prim.size()) );  

        CSGSolid::CopyIntent( dso, sso ); 



        solidMap[sSolidIdx] = dSolidIdx ; 

#ifdef WITH_S_BB
        s_bb solid_bb = {} ; 
#else
        AABB solid_bb = {} ;
#endif

        copySolidPrim(solid_bb, dPrimOffset, sso);  

        unsigned numSelectedPrimCheck = dst->prim.size() - dPrimOffset ; 
        bool numSelectedPrim_expect = numSelectedPrim == numSelectedPrimCheck ;
        assert( numSelectedPrim_expect );  
        if(!numSelectedPrim_expect) std::raise(SIGINT); 

        if(identical_bbox_cheat) // only admissable when no selection 
        { 
            dso->center_extent = sso->center_extent ;  
        }
        else
        {
#ifdef WITH_S_BB
            float* ce = &(dso->center_extent.x) ; 
            solid_bb.center_extent( ce ) ;  
#else
            dso->center_extent = solid_bb.center_extent(); 
#endif
        }

        LOG_IF(LEVEL, dump_solid) << " dso " << dso->desc() ; 

    }   // over solids of the entire geometry 

    copySolidInstances(); 
}





/**
CSGCopy::copySolidPrim
------------------------

See the AABB mechanics at the tail of CSGFoundry::addDeepCopySolid

**/

#ifdef WITH_S_BB
void CSGCopy::copySolidPrim(s_bb& solid_bb, int dPrimOffset, const CSGSolid* sso )
#else
void CSGCopy::copySolidPrim(AABB& solid_bb, int dPrimOffset, const CSGSolid* sso )
#endif
{
    unsigned dump_ = Dump(sSolidIdx); 
    bool dump_prim = ( dump_ & 0x2 ) != 0u ; 

    for(int primIdx=sso->primOffset ; primIdx < sso->primOffset+sso->numPrim ; primIdx++)
    {
         const CSGPrim* spr = src->getPrim(primIdx); 
         unsigned meshIdx = spr->meshIdx() ; 
         unsigned repeatIdx = spr->repeatIdx() ; 
         bool selected = elv == nullptr ? true : elv->is_set(meshIdx) ; 
         if( selected == false ) continue ; 

         unsigned numNode = spr->numNode()  ;  // not envisaging node selection, so this will be same in src and dst 
         unsigned dPrimIdx_global = dst->getNumPrim() ;            // destination numPrim prior to prim addition
         unsigned dPrimIdx_local = dPrimIdx_global - dPrimOffset ; // make the PrimIdx local to the solid 

         CSGPrim* dpr = dst->addPrim(numNode, -1 ); 
         if( identical ) assert( dpr->nodeOffset() == spr->nodeOffset() ); 

         dpr->setMeshIdx(meshIdx);    
         dpr->setRepeatIdx(repeatIdx); 
         dpr->setPrimIdx(dPrimIdx_local); 

#ifdef WITH_S_BB
         s_bb prim_bb = {} ;
#else
         AABB prim_bb = {} ;
#endif
         copyPrimNodes(prim_bb, spr ); 

         if(identical_bbox_cheat)  // only admissable when no selection
         {
             dpr->setAABB( spr->AABB() );
         }
         else
         {
#ifdef WITH_S_BB
             prim_bb.write<float>( dpr->AABB_() ); 
#else
             dpr->setAABB( prim_bb.data() ); 
#endif
         } 

         unsigned mismatch = 0 ; 
         std::string cf = AABB::Compare(mismatch, spr->AABB(), dpr->AABB(), 1, 1e-6 ) ; 
         if ( dump_prim && mismatch > 0 )
         {
             std::cout << std::endl ;  
             std::cout << "spr " << spr->desc() << std::endl ; 
             std::cout << "dpr " << dpr->desc() << std::endl ; 
             std::cout << "prim_bb " << std::setw(20) << " " << prim_bb.desc() << std::endl ; 
             std::cout << " AABB::Compare " << cf << std::endl ; 
         }

         solid_bb.include_aabb(prim_bb.data()); 
    }   // over prim of the solid
}


/**
CSGCopy::copyPrimNodes
-------------------------

**/

#ifdef WITH_S_BB
void CSGCopy::copyPrimNodes(s_bb& prim_bb, const CSGPrim* spr )
#else
void CSGCopy::copyPrimNodes(AABB& prim_bb, const CSGPrim* spr )
#endif
{
    for(int nodeIdx=spr->nodeOffset() ; nodeIdx < spr->nodeOffset()+spr->numNode() ; nodeIdx++)
    {
        copyNode( prim_bb, nodeIdx ); 
    }   
}

/**
CSGCopy::copyNode
--------------------

see tail of CSG_GGeo_Convert::convertNode which uses qat4::transform_aabb_inplace to change CSGNode aabb
also see tail of CSGFoundry::addDeepCopySolid

**/

#ifdef WITH_S_BB
void CSGCopy::copyNode(s_bb& prim_bb, unsigned nodeIdx )
#else
void CSGCopy::copyNode(AABB& prim_bb, unsigned nodeIdx )
#endif
{
    unsigned dump_ = Dump(sSolidIdx); 
    bool dump_node = ( dump_ & 0x4 ) != 0u ; 

    const CSGNode* snd = src->getNode(nodeIdx); 
    unsigned stypecode = snd->typecode(); 
    unsigned sTranIdx = snd->gtransformIdx(); 
    bool complement = snd->is_complement();  
    bool has_planes = CSG::HasPlanes(stypecode) ; 
    bool has_transform = sTranIdx > 0u ; 

    std::vector<float4> splanes ; 
    src->getNodePlanes(splanes, snd); 

    unsigned dTranIdx = 0u ; 
    const qat4* tra = nullptr ; 
    const qat4* itr = nullptr ; 

    if(has_transform)
    {
        tra = src->getTran(sTranIdx-1u) ; 
        itr = src->getItra(sTranIdx-1u) ; 
        dTranIdx = 1u + dst->addTran( tra, itr ) ;
    }

    CSGNode nd = {} ; 
    CSGNode::Copy(nd, *snd ); // dumb straight copy : so need to fix transform and plan references   

    CSGNode* dnd = dst->addNode(nd, &splanes, nullptr);

    dnd->setTransformComplement( dTranIdx, complement ); 

    if( identical )
    {
        assert( dnd->planeNum() == snd->planeNum() );  
        assert( dnd->planeIdx() == snd->planeIdx() ); 
    }

    bool negated = dnd->is_complemented_primitive();
    bool zero = dnd->typecode() == CSG_ZERO ; 
    bool include_bb = negated == false && zero == false ; 

    float* naabb = dnd->AABB();   

    // negated nodes are not included into the prim bb
    if(include_bb)  
    {
        if( identical_bbox_cheat )
        {
            LOG(debug) << " identical_bbox_cheat : do nothing : as dumb CSGNode::Copy already copies bbox ";  
        }
        else
        {
            dnd->setAABBLocal() ; // reset to local with no transform applied
            if(tra) tra->transform_aabb_inplace( naabb );
#ifdef WITH_S_BB
            prim_bb.include_aabb_widen( naabb );
#else
            prim_bb.include_aabb( naabb );
#endif
        }
    } 

    //if(dump_node) LOG(LEVEL) 
    if(dump_node) std::cout 
        << " nd " << std::setw(6) << nodeIdx 
        << " tc " << std::setw(4) << stypecode 
        << " st " << std::setw(4) << sTranIdx 
        << " dt " << std::setw(4) << dTranIdx 
        << " cn " << std::setw(12) << CSG::Name(stypecode) 
        << " hp " << has_planes
        << " ht " << has_transform
        << " ng " << negated
        << " ib " << include_bb
#ifdef WITH_S_BB
        << " bb " << s_bb::Desc(naabb) 
#else
        << " bb " << AABB::Desc(naabb) 
#endif
        << std::endl 
        ; 
}


/**
CSGCopy::copySolidInstances
-------------------------------

As some solids may disappear as a result of Prim selection 
it is necessary to change potentially all the inst solid references. 

See ~/j/issues/cxr_scan_cxr_overview_ELV_scan_out_of_range_error.rst

Iterates over all source instances looking up the src gas_idx and 
checking if that solid is selected and hence is being copied into the dst 
geometry. Where instances correspond to selected solids the addInstance
is invoked on the destination geometry.  

**/

void CSGCopy::copySolidInstances()
{
    unsigned sNumInst = src->getNumInst(); 

    LOG(LEVEL) << " sNumInst " << sNumInst ;  

    for(unsigned i=0 ; i < sNumInst ; i++)
    {
        int sInstIdx = i ; 
        const qat4* ins = src->getInst(sInstIdx) ; 

        int ins_idx,  gas_idx, sensor_identifier, sensor_index ;
        ins->getIdentity(ins_idx,  gas_idx, sensor_identifier, sensor_index ); 

        LOG(debug)
            << " sInstIdx " << sInstIdx
            << " ins_idx " << ins_idx
            << " gas_idx " << gas_idx
            << " sensor_identifier " << sensor_identifier
            << " sensor_index " << sensor_index
            ;

        assert( ins_idx == sInstIdx ); 
        assert( gas_idx < int(sNumSolid) ); 

        int sSolidIdx = gas_idx ; 
        int dSolidIdx = solidMap[sSolidIdx] ; 
        // need to use dSolidIdx to prevent gas_refs going stale on selection

        if( dSolidIdx > -1 )
        {
            const float* tr16 = ins->cdata(); 
            bool firstcall = false ; // NOT FIRST CALL : SO DO NOT INCREMENT sensor_identifier
            dst->addInstance(tr16,  dSolidIdx, sensor_identifier, sensor_index, firstcall ); 
        }
    }
}

