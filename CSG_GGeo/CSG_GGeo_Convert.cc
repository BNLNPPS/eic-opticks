#include <iostream>
#include <iomanip>
#include <vector>

#include "stree.h"

#include "SStr.hh"
#include "SSys.hh"
#include "SSim.hh"
#include "NP.hh"
#include "scuda.h"
#include "sqat4.h"
#include "saabb.h"
#include "SLOG.hh"
#include "SGeoConfig.hh"
#include "SName.h"

#include "NGLMExt.hpp"
#include "GLMFormat.hpp"

#include "GGeo.hh"
#include "GGeoLib.hh"
#include "GParts.hh"
#include "GMergedMesh.hh"
#include "GBndLib.hh"
#include "GScintillatorLib.hh"

#include "CSGFoundry.h"
#include "CSGSolid.h" 
#include "CSGPrim.h" 
#include "CSGNode.h" 


#include "CSG_GGeo_Convert.h"
const plog::Severity CSG_GGeo_Convert::LEVEL = SLOG::EnvLevel("CSG_GGeo_Convert", "DEBUG"); 

/**
CSG_GGeo_Convert::Translate
------------------------------

The stree is used from CSG_GGeo_Convert::addInstances to lookup_sensor_identifier

**/

CSGFoundry* CSG_GGeo_Convert::Translate(const GGeo* ggeo )
{
    CSGFoundry* fd = new CSGFoundry  ; 
    LOG(LEVEL) << "[ convert ggeo " ; 
    CSG_GGeo_Convert conv(fd, ggeo ) ; 
    conv.convert(); 

    bool ops = SSys::getenvbool("ONE_PRIM_SOLID"); 
    if(ops) conv.addOnePrimSolid(); 

    bool ons = SSys::getenvbool("ONE_NODE_SOLID"); 
    if(ons) conv.addOneNodeSolid(); 

    bool dcs = SSys::getenvbool("DEEP_COPY_SOLID"); 
    if(dcs) conv.addDeepCopySolid(); 

    bool ksb = SSys::getenvbool("KLUDGE_SCALE_PRIM_BBOX"); 
    if(ksb) conv.kludgeScalePrimBBox();  


    LOG(LEVEL) << "] convert ggeo " ; 
    return fd ; 
}



CSG_GGeo_Convert::CSG_GGeo_Convert(CSGFoundry* foundry_, const GGeo* ggeo_ ) 
    : 
    foundry(foundry_),
    ggeo(ggeo_),
    sim(SSim::Get()),
    tree(sim ? sim->get_tree() : nullptr),   // stree.h instance
    reverse(SSys::getenvbool("REVERSE")),
    dump_ridx(SSys::getenvint("DUMP_RIDX", -1)),
    meta(nullptr)
{
    LOG_IF(fatal, !sim) << "sim nullptr, must instanciate SSim before CSG_GGeo_Convert" ; 
    assert(sim); 
    LOG(LEVEL) 
        << " reverse " << reverse
        << " dump_ridx (DUMP_RIDX) " << dump_ridx
        ;  

    init(); 
}





void CSG_GGeo_Convert::init()
{
    ggeo->getMeshNames(foundry->meshname); 
    ggeo->getMergedMeshLabels(foundry->mmlabel); 
    // boundary names now travel with the NP bnd.names 

    SGeoConfig::GeometrySpecificSetup(foundry->id);

    const char* cxskiplv = SGeoConfig::CXSkipLV() ; 
    const char* cxskiplv_idxlist = SGeoConfig::CXSkipLV_IDXList() ;  
    foundry->setMeta<std::string>("cxskiplv", cxskiplv ? cxskiplv : "-" ); 
    foundry->setMeta<std::string>("cxskiplv_idxlist", cxskiplv_idxlist ? cxskiplv_idxlist : "-" ); 
    LOG(LEVEL) 
        << " cxskiplv  " << cxskiplv 
        << " cxskiplv   " << cxskiplv
        << " foundry.meshname.size " << foundry->meshname.size()
        << " foundry.id.getNumName " << foundry->id->getNumName()
        ; 
}


void CSG_GGeo_Convert::convert()
{
    LOG(LEVEL) << "[" ; 
    convertGeometry(); 
    convertSim(); 
    LOG(LEVEL) << "]" ; 
}

/**
CSG_GGeo_Convert::convertGeometry
------------------------------------


**/

void CSG_GGeo_Convert::convertGeometry(int repeatIdx,  int primIdx, int partIdxRel ) // all -1 is the default that calls convertAllSolid
{
    LOG(LEVEL) << "[" ; 
    if( repeatIdx > -1 || primIdx > -1 || partIdxRel > -1 )
    {
        LOG(info) 
            << " CAUTION : partial geometry conversions are for debugging only " 
            << " repeatIdx " << repeatIdx
            << " primIdx " << primIdx
            << " partIdxRel " << partIdxRel
            ; 
    }

    if( repeatIdx > -1 && primIdx > -1 && partIdxRel > -1 )  
    {   
        LOG(info) << "fully defined : convert just a single node " ; 
        const GParts* comp = ggeo->getCompositeParts(repeatIdx) ; 
        convertNode(comp, primIdx, partIdxRel);
    }
    else if( repeatIdx > -1 && primIdx > -1  )   
    {   
        LOG(info) << "convert all nodes in a single Prim " ; 
        const GParts* comp = ggeo->getCompositeParts(repeatIdx) ; 
        convertPrim(comp, primIdx); // Creates and adds a CSGPrim and its CSGNode(s) to CSGFoundry and returns the CSGPrim pointer. 

    }
    else if( repeatIdx > -1 )  
    {   
        LOG(info) << " convert all Prim in a single repeat composite Solid " ; 
        convertSolid(repeatIdx);
    }
    else                
    { 
        LOG(LEVEL) << "convert all solids (default)" ; 
        convertAllSolid();
    }
    LOG(LEVEL) << "]" ; 
} 

void CSG_GGeo_Convert::convertAllSolid()  // default 
{
    unsigned numRepeat = ggeo->getNumMergedMesh(); 
    LOG(LEVEL) << "[ numRepeat " << numRepeat ; 
    for(unsigned repeatIdx=0 ; repeatIdx < numRepeat ; repeatIdx++)
    {
        if(SGeoConfig::IsEnabledMergedMesh(repeatIdx))
        {
            LOG(LEVEL) << "proceeding with convert for repeatIdx " << repeatIdx ;  
            convertSolid(reverse ? numRepeat - 1 - repeatIdx : repeatIdx ); 
        }
        else
        {
            LOG(error) << "skipping convert for repeatIdx " << repeatIdx ;  
        }
    }
    LOG(LEVEL) << "] numRepeat " << numRepeat ; 
}

/**
CSG_GGeo_Convert::convertSim
--------------------------------

BndLib, ScintillatorLib, Prop, MultiFilm are converted from GGeo in NP arrays
that are added into SSim by GGeo::convertSim

**/

void CSG_GGeo_Convert::convertSim() 
{
    ggeo->convertSim() ; 
}


/**
CSG_GGeo_Convert::addInstances
---------------------------------

Invoked from tail of CSG_GGeo_Convert::convertSolid which 
gets called for all repeatIdx, including global repeatIdx 0 

Notice the flattening effect, this is consolidating the 
transforms from all GMergedMesh into one foundry->inst vector of qat4.

This handles the mapping between sensor_index and sensor_identifier
as passes the sensor info to CSGFoundry for inclusion into the 
instance transforms 4th column. 

**/

void CSG_GGeo_Convert::addInstances(unsigned repeatIdx )
{
    unsigned nmm = ggeo->getNumMergedMesh(); 
    assert( repeatIdx < nmm ); 
    const GMergedMesh* mm = ggeo->getMergedMesh(repeatIdx); 
    unsigned num_inst = mm->getNumITransforms() ;
    LOG(LEVEL) << " repeatIdx " << repeatIdx << " num_inst " << num_inst << " nmm " << nmm  ; 

    NPY<unsigned>* iid = mm->getInstancedIdentityBuffer(); 
    LOG(LEVEL) << " iid " << ( iid ? iid->getShapeString() : "-"  ) ;  

    assert(tree); 

    bool one_based_index = true ;   // CAUTION : OLD WORLD 1-based sensor_index 
    std::vector<int> sensor_index ;   
    mm->getInstancedIdentityBuffer_SensorIndex(sensor_index, one_based_index ); 
    LOG(LEVEL) << " sensor_index.size " << sensor_index.size() ;  


    bool lookup_verbose = LEVEL == info ;  
    std::vector<int> sensor_id ;
    tree->lookup_sensor_identifier(sensor_id, sensor_index, one_based_index, lookup_verbose );

    LOG(LEVEL) << " sensor_id.size " << sensor_id.size() ;  
    LOG(LEVEL) << stree::DescSensor( sensor_id, sensor_index ) ; 

    unsigned ni = iid->getShape(0); 
    unsigned nj = iid->getShape(1); 
    unsigned nk = iid->getShape(2); 
    assert( ni == sensor_index.size() ); 
    assert( num_inst == sensor_index.size() ); 
    assert( num_inst == sensor_id.size() ); 
    assert( nk == 4 ); 
    
    LOG(LEVEL) 
        << " repeatIdx " << repeatIdx
        << " num_inst (GMergedMesh::getNumITransforms) " << num_inst 
        << " iid " << ( iid ? iid->getShapeString() : "-"  )
        << " ni " << ni 
        << " nj " << nj 
        << " nk " << nk 
        ;

    //LOG(LEVEL) << " nmm " << nmm << " repeatIdx " << repeatIdx << " num_inst " << num_inst ; 

    for(unsigned i=0 ; i < num_inst ; i++)
    {
        int s_identifier = sensor_id[i] ;  
        int s_index_1 = sensor_index[i] ;    // 1-based sensor index, 0 meaning not-a-sensor 
        int s_index_0 = s_index_1 - 1 ;      // 0-based sensor index, -1 meaning not-a-sensor
        // this simple correction relies on consistent invalid index, see GMergedMesh::Get3DFouthColumnNonZero

        glm::mat4 it = mm->getITransform_(i); 
    
        const float* tr16 = glm::value_ptr(it) ; 
        unsigned gas_idx = repeatIdx ; 
        foundry->addInstance(tr16, gas_idx, s_identifier, s_index_0 ); 
    }
}



/**
CSG_GGeo_Convert::CountSolidPrim
-----------------------------------

Invoked from CSG_GGeo_Convert::convertSolid for the GParts comp from 
each repeatIdx. Where there are no skips the count is the same as 
GParts::getNumPrim of the comp. 

**/

unsigned CSG_GGeo_Convert::CountSolidPrim( const GParts* comp )
{
    unsigned solidPrim = 0 ; 
    unsigned numPrim = comp->getNumPrim();
    for(unsigned primIdx=0 ; primIdx < numPrim ; primIdx++) 
    {   
        unsigned meshIdx   = comp->getMeshIndex(primIdx);   // from idxBuffer aka lvIdx 
        bool cxskip = SGeoConfig::IsCXSkipLV(meshIdx); 
        if(cxskip)
        {
            LOG(LEVEL) << " cxskip meshIdx " << meshIdx  ; 
        }
        else
        {
            solidPrim += 1 ; 
        }
    }
    LOG(LEVEL) << " numPrim " << numPrim  << " solidPrim " << solidPrim ; 
    return solidPrim ; 
}

/**
CSG_GGeo_Convert::convertSolid NB this "solid" corresponds to ggeo/GMergedMesh
----------------------------------------------------------------------------------

1. declare ahead the number of prim(~G4VSolid) in the "solid"(repeatIdx ~MergedMesh)
2. CSG_GGeo_Convert::convertPrim using the primIdx 
3. collect the prim.AABB 
4. set the solid center_extent using the combination of the prim.AABB
5. CSG_GGeo_Convert::addInstances for the repeatIdx
6. return the solid 

**/


CSGSolid* CSG_GGeo_Convert::convertSolid( unsigned repeatIdx )
{
    LOG(LEVEL) << "[" ; 
    unsigned nmm = ggeo->getNumMergedMesh(); 
    assert( repeatIdx < nmm ); 
    const GMergedMesh* mm = ggeo->getMergedMesh(repeatIdx); 
    unsigned num_inst = mm->getNumITransforms() ;

    const GParts* comp = ggeo->getCompositeParts(repeatIdx) ;  
    assert( comp ); 
    unsigned numPrim = comp->getNumPrim();
    std::string rlabel = CSGSolid::MakeLabel('r',repeatIdx) ; 

    bool dump = dump_ridx > -1 && dump_ridx == int(repeatIdx) ;  
    unsigned solidPrim = CountSolidPrim(comp);  

    LOG(LEVEL)
        << " repeatIdx " << repeatIdx 
        << " nmm " << nmm
        << " numPrim(GParts.getNumPrim) " << numPrim
        << " solidPrim " << solidPrim 
        << " rlabel " << rlabel 
        << " num_inst " << num_inst 
        << " dump_ridx " << dump_ridx
        << " dump " << dump  
        ;   


    CSGSolid* so = foundry->addSolid(solidPrim, rlabel.c_str() );  // primOffset captured into CSGSolid 
    assert(so); 

    AABB bb = {} ;

    unsigned solidPrimChk = 0 ; 

    // over the "layers/volumes" of the solid (eg multiple vols of PMT) 
    for(unsigned primIdx=0 ; primIdx < numPrim ; primIdx++) 
    {   
        unsigned meshIdx   = comp->getMeshIndex(primIdx);   // from idxBuffer aka lvIdx 
        const char* mname = foundry->getName(meshIdx);      //  
        bool cxskip = SGeoConfig::IsCXSkipLV(meshIdx); 
        
        LOG(LEVEL) << " cxskip " << cxskip << " meshIdx " << meshIdx << " mname " << mname ;   
        if(cxskip) 
        {
            LOG(error) << " cxskip " << cxskip << " meshIdx " << meshIdx << " mname " << mname ;   
            continue ; 
        }

        CSGPrim* prim = convertPrim(comp, primIdx); // Creates and adds a CSGPrim and its CSGNode(s) to CSGFoundry and returns the CSGPrim pointer. 
        bb.include_aabb( prim->AABB() );

        unsigned sbtIdx = prim->sbtIndexOffset() ;  // from CSGFoundry::addPrim
        //assert( sbtIdx == primIdx  );    // HMM: not with skips
        assert( sbtIdx == solidPrimChk  );   

        prim->setRepeatIdx(repeatIdx); 
        prim->setPrimIdx(primIdx); // TODO: Check, this primIdx is not contiguous when cxskip used : SO NOT CONSISTENT WITH REAL primIdx

        solidPrimChk += 1 ; 
    }   
    // NB when SGeoConfig::IsCXSkipLV skips are used the primIdx set by CSGPrim::setPrimIdx will not be contiguous   
    // Q: Does the OptiX identity machinery accomodate this assigned primIdx  ?
    // A: I think the answer is currently NO 
    //
    //    I suspect the above dis-contiguous primIdx is not used for anything. 
    //    The real primIdx as used by the optix identity machinery 
    //    is the value returned from optixGetPrimitiveIndex, which is the 0-based index of the bbox within the GAS plus a bias 
    //    that is passed into the GAS and currently comes from CSGSolid so->primOffset which is just the number of 
    //    primitives so far collected. 
    //  


    assert( solidPrim == solidPrimChk ); 

    so->center_extent = bb.center_extent() ;  

    addInstances(repeatIdx); 

    LOG(LEVEL) << " numPrim " << numPrim ; 
    LOG(LEVEL) << " solid.bb " <<  bb ;
    LOG(LEVEL) << " solid.desc " << so->desc() ;
    LOG(LEVEL) << "]" ; 

    return so ; 
}


/**
CSG_GGeo_Convert::convertPrim
--------------------------------

Creates and adds a CSGPrim and its CSGNode(s) to CSGFoundry and returns the CSGPrim pointer. 

A CSGPrim (tree of nodes) corresponds to a non-composite GParts selected from a 
composite with *primIdx*. The CSGPrim is often(but not always) one-to-one related with a G4VSolid. 

The AABB of the CSGPrim are vital for GAS construction.

Previously a naive node-by-node bbox combination was used to obtain the bbox of the CSGPrim.
For example subtraction of JUNO Acrylic sphere which by positivization appears as an intersect 
with a ginormous sphere led to a huge bbox which had a terrible impact on ray tracing performance.   

The bbox combination used below has now been made somewhat CSG aware by using CSGNode::AncestorTypeMask
and CSGNode::is_complemented_leaf to avoid inappropriate bloating of the bbox. 
However the approach is still somewhat ad-hoc and could be better grounded in CSG-bbox "theory"
to improve generality : that means new geometry may still end up with bloated bbox. 

To know if the primitives bbox should be included requires analysis of the 
entire tree to see if the primitive ends up being negated. 
For example could have a double difference.
Hmm this would suggest that should always positivize the tree, 
not just for overlarge ones that get balanced.  

Then can just exclude the primitives that end up 
being complemented.

* THIS IS DONE : TREES ARE NOW STANDARDLY CONVERTED TO POSITIVE FORM AT NPY LEVEL
* Q: Where exactly ? 
* A: See npy/NTreePositive.hpp

Also previously the placeholder zero node bbox were not excluded from inclusion 
in the CSGPrim bbox which sometimes unnecessarily increased bbox in any direction by up to to 100mm, 
for some geometry with node trees that include placeholder zeros. Basically this issue meant that 
the bbox could never approach the origin closer than 100mm in some solids.  

TODO: On adding a prim the node/tran/plan offsets are captured into the Prim 
from the sizes of the foundry vectors. Suspect tran and plan offsets are not used, 
and should be removed as are now always using absolute tran and plan addressing 

**/

CSGPrim* CSG_GGeo_Convert::convertPrim(const GParts* comp, unsigned primIdx )
{
    LOG(LEVEL) << "[" ; 

    unsigned numPrim = comp->getNumPrim();  // size of prim_buffer
    assert( primIdx < numPrim ); 

    unsigned numParts  = comp->getNumParts(primIdx) ;   // from primBuffer
    unsigned meshIdx   = comp->getMeshIndex(primIdx);   // from idxBuffer aka lvIdx 
    unsigned partOffset = comp->getPartOffset(primIdx) ;

    // gymnastics hops from prim->part GParts::getTypeMask is informative for this
    unsigned root_partIdxRel = 0 ; 
    unsigned root_partIdx = partOffset + root_partIdxRel ; 

    int root_typecode  = comp->getTypeCode(root_partIdx) ; 
    int root_subNum    = comp->getSubNum(root_partIdx) ;    
    int root_subOffset = comp->getSubOffset(root_partIdx) ;  
    bool root_is_compound = CSG::IsCompound((OpticksCSG_t)root_typecode); 

    bool operators_only = true ; 
    unsigned mask = comp->getTypeMask(primIdx, operators_only); 
    bool positive = CSG::IsPositiveMask( mask ); 
    nbbox xbb = comp->getBBox(primIdx);  
    bool has_xbb = xbb.is_empty() == false ; 

    LOG(LEVEL)
        << " primIdx " << std::setw(4) << primIdx
        << " meshIdx " << std::setw(4) << meshIdx
        << " numParts " << std::setw(4) << numParts
        << " root_subNum " << std::setw(4) << root_subNum
        << " root_subOffset " << std::setw(4) << root_subOffset
        << " root_typecode " << CSG::Name(root_typecode)
        << " comp.getTypeMask " << mask 
        << " CSG::TypeMask " << CSG::TypeMask(mask)
        << " CSG::IsPositiveMask " << positive
        << " has_xbb " << has_xbb
        ;

    LOG(LEVEL) << "GParts::getBBox(primIdx) xbb " << xbb.desc() ; 


    assert( foundry->last_added_solid ); 

    int last_ridx = foundry->last_added_solid->get_ridx();  
    bool dump = last_ridx == dump_ridx ; 

    int nodeOffset_ = -1 ; 
    CSGPrim* prim = foundry->addPrim(numParts, nodeOffset_ );   
    prim->setMeshIdx(meshIdx);   
    assert(prim) ; 

    AABB bb = {} ;

    LOG_IF(LEVEL, dump)
        << " primIdx " << primIdx 
        << " numPrim " << numPrim
        << " numParts " << numParts 
        << " meshIdx " << meshIdx 
        << " last_ridx " << last_ridx
        << " dump " << dump
        ; 
   
    CSGNode* root = nullptr ; 

    for(unsigned partIdxRel=0 ; partIdxRel < numParts ; partIdxRel++ )
    {
        CSGNode* n = convertNode(comp, primIdx, partIdxRel); 

        if(root == nullptr) root = n ;   // first nodes becomes root 

        if(n->is_zero()) continue;  

        bool negated = n->is_complemented_primitive();   

        bool bbskip = negated ; 

          /*
        if(dump || bbskip) 
            LOG(LEVEL)
                << " partIdxRel " << std::setw(3) << partIdxRel 
                << " " << n->desc() 
                << " negated " << negated
                << " bbskip " << bbskip 
                ;

        */ 

        float* naabb = n->AABB();  

        if(!bbskip) bb.include_aabb( naabb );  
    }


    assert( root ); 

    // HMM: usually numParts is the number of tree nodes : but not always following introduction of list-nodes
    // TODO: fix the conversion to pass along the subNum from GGeo level 

    LOG(LEVEL) 
        << " meshIdx " << std::setw(4) << meshIdx 
        << " numParts " << std::setw(4) << numParts
        << " root_subNum " << std::setw(4) << root_subNum
        << " root_subOffset " << std::setw(4) << root_subOffset
        << " root_is_compound " << root_is_compound
        ; 

    if(root_is_compound)
    {
        assert( numParts > 1 ); 
        bool tree = int(root_subNum) == int(numParts) ; 

        if( tree == false )
        {
           LOG(error) 
               << " non-tree nodes detected, eg with list-nodes "
               << " root_subNum " << root_subNum
               << " root_subOffset " << root_subOffset
               << " numParts " << numParts
               ;
        } 

        root->setSubNum( root_subNum ); 
        root->setSubOffset( root_subOffset ); 
    }
    else
    {
        assert( numParts == 1 ); 
        assert( root_subNum == -1 ); 
        assert( root_subOffset == -1 ); 
    }


    LOG(LEVEL)
        << " ridx " << std::setw(2) << last_ridx
        << " primIdx " << std::setw(3) << primIdx 
        << " numParts " << std::setw(3) << numParts 
        ;

 
    if( has_xbb == false )
    {
        const float* bb_data = bb.data(); 
        LOG(LEVEL)
            << " has_xbb " << has_xbb 
            << " (using self defined BB for prim) "
            << " AABB \n" << AABB::Desc(bb_data) 
            ; 
        prim->setAABB( bb_data ); 
    } 
    else
    {
        LOG(LEVEL)
            << " has_xbb " << has_xbb 
            << " (USING EXTERNAL BB for prim, eg from G4VSolid) "
            << " xbb \n" << xbb.desc()
            ;
 
        prim->setAABB( xbb.min.x, xbb.min.y, xbb.min.z, xbb.max.x, xbb.max.y, xbb.max.z ); 
    }

    LOG(LEVEL) << "]" ; 
    return prim ; 
}

/**
CSG_GGeo_Convert::convertNode
-------------------------------

Canonically invoked from CSG_GGeo_Convert::convertPrim which is invoked from CSG_GGeo_Convert::convertSolid


Add node to foundry and returns pointer to it

primIdx0:numPrim-1 identifying the Prim (aka layer) within the composite 

partIdxRel 
    relative part(aka node) index 0:numPart-1 within the Prim, used together with 
    the partOffset of the Prim to yield the absolute partIdx within the composite

Recall that GParts are concatenated to form the composite.

Note that repeatedly getting the same gtran for different part(aka node) 
will repeatedly add that same transform to the foundry even when its the identity transform.
So this is profligate in duplicated transforms.

Observing many complemented nodes that appear should not be complemented, even in some 
single node Prim such as "r1/4" and 31 node "r8/0".


CSG_CONVEXPOLYHEDRON bbox special casing
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Although access to the original nconvexpolyhedron to get the bbox 
can not be assumed as may be postcache for example, what is possible is 
accessing the bbox param. Tracing where the bbox goes when nconvexpolyhedron 
NNode gets packed into GParts reveals that can just use GParts::getBBox
for CSG_CONVEXPOLYHEDRON. 

* X4Solid::Convert from G4VSolid to NNode tree
* NCSG::Adopt packs the NNode tree into an array using NNode::part 


TODO: CSG_CONVEXPOLYHEDRON and others need planes to be propagated too
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Compare with the old pre-7 workflow in OGeo::makeAnalyticGeometry
where the planes from GParts are passes into optix::Buffer planBuffer.

HMM: will have to rejig the plane indexing as it probably is currently using per MM buffers
HMM: how does the plane data get concatenated with GParts, and are the planIdx in the nodes 
     updated accordingly ?

**/

CSGNode* CSG_GGeo_Convert::convertNode(const GParts* comp, unsigned primIdx, unsigned partIdxRel )
{
    unsigned repeatIdx = comp->getRepeatIndex();  // set in GGeo::deferredCreateGParts
    unsigned partOffset = comp->getPartOffset(primIdx) ;
    unsigned partIdx = partOffset + partIdxRel ;
    unsigned idx = comp->getIndex(partIdx);
    assert( idx == partIdx ); 
    unsigned boundary = comp->getBoundary(partIdx); // EXPT

    std::string tag = comp->getTag(partIdx); 
    unsigned tc = comp->getTypeCode(partIdx);
    bool is_list = CSG::IsList((OpticksCSG_t)tc) ; 
    int subNum = is_list ? comp->getSubNum(partIdx) : -1 ;  
    int subOffset = is_list ? comp->getSubOffset(partIdx) : -1 ;  


    // TODO: transform handling in double, narrowing to float at the last possible moment 
    const Tran<float>* tv = nullptr ; 
    unsigned gtran = comp->getGTransform(partIdx);  // 1-based index, 0 means None
    if( gtran > 0 )
    {
        glm::mat4 t = comp->getTran(gtran-1,0) ;
        glm::mat4 v = comp->getTran(gtran-1,1); 
        tv = new Tran<float>(t, v); 
    }

    unsigned tranIdx = tv ?  1 + foundry->addTran(tv) : 0 ;   // 1-based index referencing foundry transforms

    // HMM: this is not using the higher level 
    // CSGFoundry::addNode with transform pointer argumnent 


    LOG(LEVEL) 
        << " primIdx " << std::setw(4) << primIdx 
        << " partIdxRel " << std::setw(4) << partIdxRel
        << " tag " << std::setw(6) << tag
        << " tc " << std::setw(10) << CSG::Name((OpticksCSG_t)tc) 
        << " tranIdx " << std::setw(4) << tranIdx
        << " is_list " << ( is_list ? "IS_LIST" : "not_list" )
        << " subNum " << std::setw(4) << subNum 
        << " subOffset " << std::setw(4) << subOffset
        ; 

    const float* param6 = comp->getPartValues(partIdx, 0, 0 );  
    bool complement = comp->getComplement(partIdx);

    bool has_planes = CSG::HasPlanes(tc); 
    std::vector<float4>* planes = has_planes ? GetPlanes(comp, primIdx, partIdxRel) : nullptr ; 

    const float* aabb = nullptr ;  
    CSGNode nd = CSGNode::Make(tc, param6, aabb ) ; 
    CSGNode* n = foundry->addNode(nd, planes );

    if( tc == CSG_THETACUT )
    {
        LOG(LEVEL); 
        CSGNode::Dump(n, 1, "CSG_GGeo_Convert::convertNode CSG_THETACUT before setIndex" );  
    }

    n->setIndex(partIdx); 

    nbbox bb = comp->getBBox(partIdx); 
    bool expect_external_bbox = CSG::ExpectExternalBBox( (OpticksCSG_t) tc );  
    // external bbox is expected for "higher level" solids : CSG_CONVEXPOLYHEDRON , CSG_CONTIGUOUS, CSG_DISCONTIGUOUS, CSG_OVERLAP

    if( bb.is_empty()  )
    {
        if(expect_external_bbox==true)
        {
            LOG(fatal) << " node of type " << CSG::Name(tc) << " are expected to have external bbox, but there is none " ; 
            assert(0); 
        }
        n->setAABBLocal(); // use params of each type to set the bbox 
    }
    else
    {
        if(expect_external_bbox==false)
        {
            LOG(error) << " not expecting bbox for node of type " << CSG::Name(tc) << " (maybe boolean tree for general sphere)" ; 
        }
        n->setAABB( bb.min.x, bb.min.y, bb.min.z, bb.max.x, bb.max.y, bb.max.z ); 
    }

    n->setTransform(tranIdx); 
    n->setComplement(complement); 
    n->setBoundary(boundary);       // EXPT

    bool dump_complement = false ; 
    if(complement && dump_complement) 
        std::cout 
            << "CSG_GGeo_Convert::convertNode"
            << " repeatIdx " << repeatIdx 
            << " primIdx " << primIdx 
            << " partIdxRel " << partIdxRel 
            << " complement " << complement
            << std::endl 
            ; 

    assert( n->is_complement() == complement ); 

    if(tranIdx > 0 )
    {    
        const qat4* q = foundry->getTran(tranIdx-1u) ;
        q->transform_aabb_inplace( n->AABB() );
    }

    return n ; 
}


std::vector<float4>* CSG_GGeo_Convert::GetPlanes(const GParts* comp, unsigned primIdx, unsigned partIdxRel ) // static
{
    unsigned partOffset = comp->getPartOffset(primIdx) ;
    unsigned partIdx = partOffset + partIdxRel ;
 
    unsigned tc = comp->getTypeCode(partIdx);
    bool has_planes = CSG::HasPlanes(tc); 
    assert(has_planes); 

    unsigned planIdx = comp->getPlanIdx(partIdx); 
    unsigned planNum = comp->getPlanNum(partIdx); 

    NPY<float>* pl_buf = comp->getPlanBuffer(); 

    bool pl_expect = pl_buf->hasShape(-1, 4) ;   // HUH: was (planNum, 4) surely should be (-1, 4)

    LOG_IF(fatal, !pl_expect)
        << " primIdx " << primIdx
        << " partOffset " << partOffset
        << " partIdx " << partIdx
        << " tc " << CSG::Name(tc)
        << " planIdx " << planIdx
        << " planNum " << planNum 
        << " unexpected pl_buf shape  " << pl_buf->getShapeString()
        ;

    assert( pl_expect ); 

    std::vector<float4>* planes = new std::vector<float4>(planNum) ; 
    unsigned num_bytes = sizeof(float)*4*planNum ; 

    memcpy( (char*)planes->data(),  pl_buf->getBytes(),  num_bytes );  

    LOG(info)
        << " has_planes " << has_planes 
        << " planIdx " << planIdx
        << " planNum " << planNum
        ;

    if(planes) DumpPlanes(*planes, "CSG_GGeo_Convert::GetPlanes" ); 

    return planes ; 
}

void CSG_GGeo_Convert::DumpPlanes( const std::vector<float4>& planes , const char* msg ) // static
{
    std::cout << msg << std::endl ; 
    for(unsigned i=0 ; i < planes.size() ; i++)
    {
        const float4& p = planes[i] ; 
        std::cout 
            << " i " << std::setw(4) << i 
            << " p ( " << std::setw(10) << std::fixed << std::setprecision(3) << p.x  
            << " " << std::setw(10) << std::fixed << std::setprecision(3) << p.y
            << " " << std::setw(10) << std::fixed << std::setprecision(3) << p.z  
            << " " << std::setw(10) << std::fixed << std::setprecision(3) << p.w
            << " )"
            << std::endl
            ;   
    }
}

/**
CSG_GGeo_Convert::addOnePrimSolid
-------------------------------------

CSGSolid typically have multiple Prim, in case of the remainder solid (solidIdx 0) 
there can even be thousands of Prim in a single "Solid".

For debugging purposes it is useful to add extra solids that each have only a single 
CSGPrim allowing rendering of each "layer" of a Solid without adding separate 
code paths to do this, just reusing the existing machinery applied to the extra  solids.

Note that repeatIdx/solidIdx/gasIdx are mostly referring to the same thing 
with nuances of which stage they are at. 
**/

void CSG_GGeo_Convert::addOnePrimSolid(unsigned solidIdx)
{
    const CSGSolid* orig = foundry->getSolid(solidIdx);    

    if(orig->numPrim == 1 ) 
    {
        LOG(info) << "already single prim solid, no point adding another" ; 
        return ; 
    }  

    for(unsigned primIdx=orig->primOffset ; primIdx < unsigned(orig->primOffset+orig->numPrim) ; primIdx++)  
    {
        unsigned numPrim = 1 ; 
        int primOffset_ = primIdx ;   // note absolute primIdx

        unsigned primIdxRel = primIdx - orig->primOffset ; 

        std::string rp_label = CSGSolid::MakeLabel('r', solidIdx, 'p', primIdxRel ) ;   

        // NB not adding Prim just reusing pre-existing in separate Solid
        CSGSolid* pso = foundry->addSolid(numPrim, rp_label.c_str(), primOffset_ ); 

        AABB bb = {} ;
        const CSGPrim* prim = foundry->getPrim(primIdx) ; //  note absolute primIdx
        bb.include_aabb( prim->AABB() );

        pso->center_extent = bb.center_extent() ;  

        pso->type = ONE_PRIM_SOLID ; 
        LOG(info) << pso->desc() ;  
    }   
}




void CSG_GGeo_Convert::addOnePrimSolid()
{
    unsigned num_solid_standard = foundry->getNumSolid(STANDARD_SOLID) ; 

    std::vector<unsigned> one_prim_solid ; 
    SStr::GetEVector(one_prim_solid, "ONE_PRIM_SOLID", "1,2,3,4,5,6,7,8" ); 

    LOG(error) << "foundry.desc before " << foundry->desc(); 
    for(unsigned i=0 ; i < one_prim_solid.size() ; i++)
    {
        unsigned solidIdx = one_prim_solid[i] ; 
        if( solidIdx < num_solid_standard )
        {
            addOnePrimSolid(solidIdx);       
        }
        else
        {
            LOG(error) << " requested ONE_PRIM_SOLID solidIdx out of range " << solidIdx ; 
        }
    }
    LOG(error) << "foundry.desc after " << foundry->desc(); 
}



/**
CSG_GGeo_Convert::addOneNodeSolid
------------------------------------

Unlike OnePrimSolid this needs to create 
both the solid and the Prim which references a pre-existing Node.

This makes sense for leaf nodes, but for operator nodes
a single node solid makes no sense : would need to construct subtrees in that case 
and also create new node sequences. This is because cannot partially reuse preexisting 
node complete binary trees to form subtrees because the node order 
would be wrong for subtrees.

**/

void CSG_GGeo_Convert::addOneNodeSolid()
{
    unsigned num_solid_standard = foundry->getNumSolid(STANDARD_SOLID) ; 

    std::vector<unsigned> one_node_solid ; 
    SStr::GetEVector(one_node_solid, "ONE_NODE_SOLID", "1,2,3,4,5,6,7,8" ); 

    LOG(error) << "foundry.desc before " << foundry->desc(); 

    for(unsigned i=0 ; i < one_node_solid.size() ; i++)
    {
        unsigned solidIdx = one_node_solid[i] ; 
        if( solidIdx < num_solid_standard )
        {
            addOneNodeSolid(solidIdx);       
        }
        else
        {
            LOG(error) << " requested ONE_NODE_SOLID solidIdx out of range " << solidIdx ; 
        }
    }
    LOG(error) << "foundry.desc after " << foundry->desc(); 

}


/**
CSG_GGeo_Convert::addOneNodeSolid
-------------------------------------

Invokes the below addOneNodeSolid for each prim of the original solidIdx 

**/

void CSG_GGeo_Convert::addOneNodeSolid(unsigned solidIdx)
{
    LOG(info) << " solidIdx " << solidIdx ; 
    const CSGSolid* orig = foundry->getSolid(solidIdx);    

    for(unsigned primIdx=orig->primOffset ; primIdx < unsigned(orig->primOffset+orig->numPrim) ; primIdx++)  
    {
        unsigned primIdxRel = primIdx - orig->primOffset ; 

        addOneNodeSolid(solidIdx, primIdx, primIdxRel);   
    }   
}


/**
CSG_GGeo_Convert::addOneNodeSolid
-----------------------------------

Adds solids for each non-operator leaf node of the Solid/Prim 
identified by solidIdx/primIdx/primIdxRel with label of form "R1P0N0" 
that shows the origin of this debugging solid. 

**/

void CSG_GGeo_Convert::addOneNodeSolid(unsigned solidIdx, unsigned primIdx, unsigned primIdxRel)
{
    const CSGPrim* prim = foundry->getPrim(primIdx);      

    for(unsigned nodeIdxRel=0 ; nodeIdxRel < unsigned(prim->numNode()) ; nodeIdxRel++ )
    {
        unsigned nodeIdx = prim->nodeOffset() + nodeIdxRel ; 

        const CSGNode* node = foundry->getNode(nodeIdx); 

        if(node->is_operator() || node->is_zero()) continue ; 

        AABB bb = {} ;
        bb.include_aabb( node->AABB() );   

        std::string rpn_label = CSGSolid::MakeLabel('R', solidIdx, 'P', primIdxRel, 'N', nodeIdxRel ) ;   

        unsigned numPrim = 1 ;  
        CSGSolid* rpn_solid = foundry->addSolid(numPrim, rpn_label.c_str()  ); 
        rpn_solid->center_extent = bb.center_extent() ;  

        int numNode = 1 ; 
        int nodeOffset = nodeIdx ;   // re-using the node 

        CSGPrim* rpn_prim = foundry->addPrim(numNode, nodeOffset ) ; 
        rpn_prim->setMeshIdx(prim->meshIdx()); 
        rpn_prim->setAABB( bb.data() ); 
        
        //rpn_prim->setMeshIdx(prim->meshIdx()); 
        rpn_prim->setRepeatIdx(prim->repeatIdx()); 
        rpn_prim->setPrimIdx(prim->primIdx()); 

        rpn_solid->type = ONE_NODE_SOLID ; 
        rpn_solid->center_extent = bb.center_extent() ;  

        LOG(info) << rpn_solid->desc() ;  
    }
}

/**
CSG_GGeo_Convert::addDeepCopySolid
-------------------------------------

Invoked from main CSG_GGeoTest.cc when DEEP_COPY_SOLID envvar is defined. 

**/
void CSG_GGeo_Convert::addDeepCopySolid()
{
    unsigned num_solid_standard = foundry->getNumSolid(STANDARD_SOLID) ; 

    std::vector<unsigned> deep_copy_solid ; 
    SStr::GetEVector(deep_copy_solid, "DEEP_COPY_SOLID", "1,2,3,4" ); 

    LOG(error) << "foundry.desc before " << foundry->desc(); 

    for(unsigned i=0 ; i < deep_copy_solid.size() ; i++)
    {
        unsigned solidIdx = deep_copy_solid[i] ; 
        if( solidIdx < num_solid_standard )
        {
            foundry->addDeepCopySolid(solidIdx);       
        }
        else
        {
            LOG(error) << " requested DEEP_COPY_SOLID solidIdx out of range " << solidIdx ; 
        }
    }
    LOG(error) << "foundry.desc after " << foundry->desc(); 
}


void CSG_GGeo_Convert::kludgeScalePrimBBox()
{
    const char* kludge_scale_prim_bbox = SSys::getenvvar("KLUDGE_SCALE_PRIM_BBOX", nullptr); 
    assert(kludge_scale_prim_bbox);  
    
    LOG(error) << "foundry.desc before " << foundry->desc(); 

    float dscale = 0.1f ; 
    foundry->kludgeScalePrimBBox(kludge_scale_prim_bbox, dscale );

    LOG(error) << "foundry.desc after " << foundry->desc(); 
}




