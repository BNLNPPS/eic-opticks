#include "MWrap.hh"

#include <iostream>
#include <iomanip>

#include "GLMPrint.hpp"

#include <boost/log/trivial.hpp>
#define LOG BOOST_LOG_TRIVIAL
// trace/debug/info/warning/error/fatal

#include <OpenMesh/Core/IO/MeshIO.hh>
#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>
typedef OpenMesh::TriMesh_ArrayKernelT<>  MyMesh;


template <typename MeshT>
void MWrap<MeshT>::copyIn(float* vdata, unsigned int num_vertices, int* fdata, unsigned int num_faces )  
{
    MeshT* mesh = m_mesh ; 

    typedef typename MeshT::VertexHandle VH ; 
    typedef typename MeshT::Point P ; 

    VH* vh = new VH[num_vertices] ;

    for(unsigned int i=0 ; i < num_vertices ; i++)
    {
        vh[i] = mesh->add_vertex(P(*(vdata), *(vdata+1), *(vdata+2)));
        vdata += 3 ; 
    } 

    std::vector<VH> fvh;

    for(unsigned int i=0 ; i < num_faces ; i++)
    {
        fvh.clear();

        int v0 = *(fdata + 0) ; 
        int v1 = *(fdata + 1) ; 
        int v2 = *(fdata + 2) ; 
        fdata += 3 ; 

        //printf( "f %4d : v %3d %3d %3d \n", i, v0, v1, v2 ); 
        fvh.push_back(vh[v0]);
        fvh.push_back(vh[v1]);
        fvh.push_back(vh[v2]);

        mesh->add_face(fvh);
    }
    delete[] vh ; 
}




template <typename MeshT>
int MWrap<MeshT>::labelConnectedComponents()
{
    MeshT* mesh = m_mesh ; 

    OpenMesh::VPropHandleT<int> component ; 
    assert(true == mesh->get_property_handle(component, "component"));

    typedef typename MeshT::VertexHandle VH ;
    typedef typename MeshT::VertexIter VI ; 
    typedef typename MeshT::VertexVertexIter VVI ;

    for( VI vi=mesh->vertices_begin() ; vi != mesh->vertices_end(); ++vi ) 
         mesh->property(component, *vi) = -1 ;

    VI seed = mesh->vertices_begin();
    VI end  = mesh->vertices_end();

    int componentIndex = -1 ; 
    while(true)
    {
        // starting from current look for unvisited "-1" vertices
        bool found_seed(false) ; 
        for(VI vi=seed ; vi != end ; vi++)
        {
            if(mesh->property(component, *vi) == -1) 
            {
                componentIndex += 1 ; 
                mesh->property(component, *vi) = componentIndex ;  
                seed = vi ; 
                found_seed = true ; 
                break ;  
            }
        }

        if(!found_seed) break ;  // no more unvisited vertices

        std::vector<VH> vstack ;
        vstack.push_back(*seed);

        // stack based recursion spreading the componentIndex to all connected vertices

        while(vstack.size() > 0)
        {
            VH current = vstack.back();
            vstack.pop_back();
            for (VVI vvi=mesh->vv_iter( current ); vvi ; ++vvi)
            {
                if(mesh->property(component, *vvi) == -1) 
                {
                    mesh->property(component, *vvi) = componentIndex ; 
                    vstack.push_back( *vvi );
                }
            }
        }
    }
    return componentIndex + 1 ; 
}




template <typename MeshT>
void MWrap<MeshT>::calcFaceCentroids(const char* fpropname)
{
    MeshT* mesh = m_mesh ; 

    typedef typename MeshT::FaceIter FI ; 
    typedef typename MeshT::ConstFaceVertexIter FVI ; 
    typedef typename MeshT::Point P ; 

    OpenMesh::FPropHandleT<P> centroid;
    mesh->add_property(centroid, fpropname);

    for( FI f=mesh->faces_begin() ; f != mesh->faces_end(); ++f ) 
    {
        P cog ;
        mesh->calc_face_centroid( *f, cog);
        mesh->property(centroid,*f) = cog ; 
    }
}


template <typename MeshT>
int MWrap<MeshT>::labelSpatialPairs(MeshT* a, MeshT* b, glm::vec4 delta, const char* fposprop, const char* fpropname)  
{
    typedef typename MeshT::Point P ; 
    typedef typename MeshT::FaceIter FI ; 

   // check the input positional face property

    OpenMesh::FPropHandleT<P> a_fposprop ;
    assert(a->get_property_handle(a_fposprop, fposprop));

    OpenMesh::FPropHandleT<P> b_fposprop ;
    assert(b->get_property_handle(b_fposprop, fposprop));

    // setup the output boolean face property 

    OpenMesh::FPropHandleT<bool> a_paired ;
    a->add_property(a_paired, fpropname); 

    OpenMesh::FPropHandleT<bool> b_paired ;
    b->add_property(b_paired, fpropname); 

    for( FI af=a->faces_begin() ; af != a->faces_end(); ++af ) 
         a->property(a_paired, *af) = false ; 

    for( FI bf=b->faces_begin() ; bf != b->faces_end(); ++bf ) 
         b->property(b_paired, *bf) = false ; 

    // very inefficent approach, calculating all pairings 
    // but geometry surgery is a once only endeavor

    unsigned int npair(0);

    for( FI af=a->faces_begin() ; af != a->faces_end(); ++af ) 
    {
        int fa = af->idx(); 
        P ap = a->property(a_fposprop, *af) ;
        P an = a->normal(*af);

        for( FI bf=b->faces_begin() ; bf != b->faces_end(); ++bf ) 
        { 
            int fb = bf->idx(); 
            P bp = b->property(b_fposprop, *bf) ;
            P dp = bp - ap ; 
            P bn = b->normal(*bf);

            float adotb = OpenMesh::dot(an,bn); 

            bool close = fabs(dp[0]) < delta.x && fabs(dp[1]) < delta.y && fabs(dp[2]) < delta.z ;
            bool backtoback = adotb < delta.w ;  

            if(close && backtoback) 
            {
#ifdef DEBUG
                 std::cout 
                       << std::setw(3) << npair 
                       << " (" << std::setw(3) << fa
                       << "," << std::setw(3) << fb 
                       << ")" 
                       << " an " << std::setprecision(3) << std::fixed << std::setw(20)  << an 
                       << " bn " << std::setprecision(3) << std::fixed << std::setw(20)  << bn 
                       << " a.b " << std::setprecision(3) << std::fixed << std::setw(10) << adotb
                       << " dp " << std::setprecision(3) << std::fixed << std::setw(20)  << dp
                       << std::endl ;  
#endif
                 npair++ ; 

                 // mark the paired faces
                 a->property(a_paired, *af ) = true ; 
                 b->property(b_paired, *bf ) = true ; 
            }
        }
    }


    LOG(info) << "MWrap<MeshT>::labelSpatialPairs"
              << " fposprop " << fposprop  
              << " fpropname " << fpropname 
              << " npair " << npair
             ; 

    return npair ; 
}









template <typename MeshT>
unsigned int MWrap<MeshT>::deleteFaces(const char* fpredicate_name )
{
    MeshT* mesh = m_mesh ;

    typedef typename MeshT::FaceIter FI ; 

    OpenMesh::FPropHandleT<bool> fpredicate ;
    assert(mesh->get_property_handle(fpredicate, fpredicate_name)); 

    unsigned int count(0);
    for( FI f=mesh->faces_begin() ; f != mesh->faces_end(); ++f )
    {
        if(mesh->property(fpredicate, *f)) 
        {
           // std::cout << f->idx() << " " ; 
            bool delete_isolated_vertices = true ; 
            mesh->delete_face( *f, delete_isolated_vertices );
            count++ ; 
        }
    }
    //std::cout << std::endl ; 

    mesh->garbage_collection();
    LOG(info) << "deleteFaces " << fpredicate_name << " " << count ; 
    return count ; 
}

template <typename MeshT>
void MWrap<MeshT>::copyTo(MeshT* dst, std::map<typename MeshT::VertexHandle, typename MeshT::VertexHandle>& src2dst )
{
     partialCopyTo( dst, NULL, 0, src2dst);
}

template <typename MeshT>
void MWrap<MeshT>::partialCopyTo(MeshT* dst, const char* ivpropname, int ivpropval, std::map<typename MeshT::VertexHandle, typename MeshT::VertexHandle>& src2dst )
{
    typedef typename MeshT::VertexIter VI ; 
    typedef typename MeshT::FaceIter FI ; 
    typedef typename MeshT::VertexFaceIter VFI ; 
    typedef typename MeshT::VertexHandle VH ; 
    typedef typename MeshT::FaceHandle FH ; 
    typedef typename MeshT::Point P ; 
    typedef typename MeshT::ConstFaceVertexIter FVI ; 

    MeshT* mesh = m_mesh ;  

    OpenMesh::VPropHandleT<int> ivprop ;
    bool has_ivprop = ivpropname ? mesh->get_property_handle(ivprop, ivpropname) : false ;

    OpenMesh::FPropHandleT<bool> copied;
    mesh->add_property(copied); 

    for( FI f=mesh->faces_begin() ; f != mesh->faces_end(); ++f ) 
         mesh->property(copied, *f) = false ;


    enum { VERT, FACE, NUM_PASS } ;

    for(unsigned int pass=VERT ; pass < NUM_PASS ; pass++) 
    {
        for( VI v=mesh->vertices_begin() ; v != mesh->vertices_end(); ++v )
        { 
            if(has_ivprop && mesh->property(ivprop, *v) != ivpropval ) continue ; 

            switch(pass)
            {
                case VERT:
                     src2dst[*v] = dst->add_vertex(mesh->point(*v)) ;
                     break ; 
                case FACE:
                     for(VFI f=mesh->vf_iter(*v) ; f ; f++) 
                     {
                         if(mesh->property(copied, *f) == true) continue ;                 
                         mesh->property(copied, *f) = true ; 

                         // collect handles of the vertices of this fresh face 
                         std::vector<VH>  fvh ;
                         for(FVI fv=mesh->cfv_iter(*f) ; fv ; fv++) fvh.push_back( src2dst[*fv] );
                         dst->add_face(fvh);
                     }  
                     break ; 
                default:
                     assert(0);
                     break ; 
            }
       }
    }

    dst->request_face_normals();
    dst->request_vertex_normals();
    dst->update_normals();

    // the request has to be called before a vertex/face/edge can be deleted. it grants access to the status attribute
    //  http://www.openmesh.org/Daily-Builds/Doc/a00058.html
    dst->request_face_status();
    dst->request_edge_status();
    dst->request_vertex_status();
}


template <typename MeshT>
void MWrap<MeshT>::findBounds()
{
    typedef typename MeshT::Point P ; 
    typedef typename MeshT::ConstVertexIter VI ; 

    BBox& bb = m_bbox ; 
    MeshT* mesh = m_mesh ; 

    bb.min = glm::vec3(FLT_MAX);
    bb.max = glm::vec3(-FLT_MAX);

    for( VI v=mesh->vertices_begin() ; v != mesh->vertices_end(); ++v )
    {
        P p = mesh->point(*v) ; 

        bb.min.x = std::min( bb.min.x, p[0]);  
        bb.min.y = std::min( bb.min.y, p[1]);  
        bb.min.z = std::min( bb.min.z, p[2]);

        bb.max.x = std::max( bb.max.x, p[0]);  
        bb.max.y = std::max( bb.max.y, p[1]);  
        bb.max.z = std::max( bb.max.z, p[2]);
    }
}

template <typename MeshT>
void MWrap<MeshT>::dumpBounds(const char* msg)
{
    LOG(info) << msg ; 
    findBounds();
    BBox& bb = m_bbox ; 
    print( bb.max , "bb.max"); 
    print( bb.min , "bb.min"); 
    print( bb.max - bb.min , "bb.max - bb.min"); 
}





template <typename MeshT>
void MWrap<MeshT>::dumpStats(const char* msg)
{
    MeshT* mesh = m_mesh ; 

    unsigned int nface = std::distance( mesh->faces_begin(), mesh->faces_end() );
    unsigned int nvert = std::distance( mesh->vertices_begin(), mesh->vertices_end() );
    unsigned int nedge = std::distance( mesh->edges_begin(), mesh->edges_end() );

    unsigned int n_face = mesh->n_faces(); 
    unsigned int n_vert = mesh->n_vertices(); 
    unsigned int n_edge = mesh->n_edges();

    assert( nface == n_face );
    assert( nvert == n_vert );
    assert( nedge == n_edge );

    LOG(info) << msg  
              << " nface " << nface 
              << " nvert " << nvert 
              << " nedge " << nedge 
              << " V - E + F = " << nvert - nedge + nface 
              << " (should be 2 for Euler Polyhedra) "   
              ; 
}
 

template <typename MeshT>
void MWrap<MeshT>::dumpFaces(const char* msg, unsigned int detail)
{
    typedef typename MeshT::FaceIter FI ; 
    typedef typename MeshT::ConstFaceVertexIter FVI ; 
    typedef typename MeshT::Point P ; 

    MeshT* mesh = m_mesh ; 

    OpenMesh::FPropHandleT<P> centroid ;
    bool fcentroid = mesh->get_property_handle(centroid, "centroid");

    LOG(info) << msg << " nface " << mesh->n_faces() ;

    for( FI f=mesh->faces_begin() ; f != mesh->faces_end(); ++f ) 
    {
        int f_idx = f->idx() ;  
        std::cout << " f " << std::setw(4) << *f 
                  << " i " << std::setw(3) << f_idx 
                  << " v " << std::setw(3) << mesh->valence(*f) 
                  << " : " 
                  ; 

        if(fcentroid)
        {
            std::cout << " c "
                      << std::setprecision(3) << std::fixed << std::setw(20) 
                      << mesh->property(centroid,*f)
                      << "  " 
                      ;
        } 

        // over points of the face 
        for(FVI fv=mesh->cfv_iter(*f) ; fv ; fv++) 
             std::cout << std::setw(3) << *fv << " " ;

        for(FVI fv=mesh->cfv_iter(*f) ; fv ; fv++) 
             std::cout 
                       << std::setprecision(3) << std::fixed << std::setw(20) 
                       << mesh->point(*fv) << " "
                       ;

         std::cout 
              << " n " 
              << std::setprecision(3) << std::fixed << std::setw(20) 
              << mesh->normal(*f)
              << std::endl ;  
    }
}
 
template <typename MeshT>
void MWrap<MeshT>::dumpVertices(const char* msg, unsigned int detail)
{
    typedef typename MeshT::VertexIter VI ; 
    typedef typename MeshT::VertexFaceIter VFI ; 
    typedef typename MeshT::ConstFaceVertexIter FVI ; 

    MeshT* mesh = m_mesh ; 
    LOG(info) << msg << " nvert " << mesh->n_vertices() ;
 
    for( VI v=mesh->vertices_begin() ; v != mesh->vertices_end(); ++v )
    {
         std::cout << " v " << std::setw(3) << *v << " # " << std::setw(3) << mesh->valence(*v) << " : "  ;  
         // all faces around a vertex, fans are apparent
         for(VFI vf=mesh->vf_iter(*v)  ; vf ; vf++) 
             std::cout << " " << std::setw(3) << *vf ;   
         std::cout << std::endl ;  

         if(detail > 1)
         {
             for(VFI vf=mesh->vf_iter(*v)  ; vf ; vf++) 
             {
                 // over points of the face 
                std::cout << "     "  ;  
                for(FVI fv=mesh->cfv_iter(*vf) ; fv ; fv++) 
                     std::cout << std::setw(3) << *fv << " " ;

               for(FVI fv=mesh->cfv_iter(*vf) ; fv ; fv++) 
                      std::cout 
                       << std::setprecision(3) << std::fixed << std::setw(20) 
                       << mesh->point(*fv) << " "
                       ;

                std::cout 
                    << " n " 
                    << std::setprecision(3) << std::fixed << std::setw(20) 
                    << mesh->normal(*vf)
                    << std::endl ;  
             } 
         } 
    }
}


template <typename MeshT>
void MWrap<MeshT>::dump(const char* msg, unsigned int detail)
{
    dumpStats(msg);
    if(detail > 0)
    {
        dumpFaces(msg, detail);
        dumpVertices(msg, detail);
        dumpBounds(msg);
    }
}



template <typename MeshT>
unsigned int MWrap<MeshT>::collectBoundaryLoop()  
{
    typedef typename MeshT::FaceIter FI ; 
    typedef typename MeshT::VertexIter VI ; 
    typedef typename MeshT::VertexHandle VH ;
    typedef typename MeshT::HalfedgeHandle HH ;
    typedef typename MeshT::Point P ; 

    MeshT* mesh = m_mesh ; 
    std::vector<VH>& vbnd = m_boundary ; 

    VI v ; 
    VI v_end = mesh->vertices_end();

    for( v=mesh->vertices_begin(); v != v_end; ++v) if (mesh->is_boundary(*v)) break;

    if( v == v_end )
    {
        LOG(warning) << "collectBoundaryLoop : No boundary found\n";
        return 0 ;
    }
    // collect boundary loop
    HH hh0 = mesh->halfedge_handle(*v) ;
    HH hh = hh0 ;
    do 
    {
        vbnd.push_back(mesh->to_vertex_handle(hh));
        hh = mesh->next_halfedge_handle(hh);
    }
    while (hh != hh0);

    return vbnd.size();
}


template <typename MeshT>
void MWrap<MeshT>::dumpBoundaryLoop(const char* msg)  
{
    typedef typename MeshT::VertexHandle VH ;
    typedef typename MeshT::Point P ; 

    MeshT* mesh = m_mesh ; 
    std::vector<VH>& vbnd = m_boundary ; 
    LOG(info) << msg << " nvert " << vbnd.size() ; 
    
    for(unsigned int i=1 ; i < vbnd.size() ; i++)
    {
        int a_idx = vbnd[i-1].idx();
        int b_idx = vbnd[i].idx();
        P a = mesh->point(vbnd[i-1]) ;
        P b = mesh->point(vbnd[i]) ;  
        P d = b - a ; 

        std::cout 
                << std::setw(3) << i
                << " (" << std::setw(3) << a_idx
                << "->" << std::setw(3) << b_idx 
                << ")" 
                << " a " << std::setprecision(3) << std::fixed << std::setw(20)  << a
                << " b " << std::setprecision(3) << std::fixed << std::setw(20)  << b 
                << " d " << std::setprecision(3) << std::fixed << std::setw(20)  << d
                << std::endl ;  
    }
}

template <typename MeshT>
std::map<typename MeshT::VertexHandle, typename MeshT::VertexHandle> MWrap<MeshT>::findBoundaryVertexMap(MWrap<MeshT>* wa, MWrap<MeshT>* wb)
{
    LOG(info) << "findBoundaryVertexMapping" ; 

    typedef typename MeshT::VertexHandle VH ;
    typedef typename MeshT::Point P ; 
    typedef typename std::vector<VH>::iterator VHI ; 

    std::map<VH,VH> a2b ; 
 
    MeshT* a = wa->getMesh();
    MeshT* b = wb->getMesh();

    std::vector<VH>& abnd = wa->getBoundaryLoop();
    std::vector<VH>& bbnd = wb->getBoundaryLoop();

    assert(abnd.size() == bbnd.size()); 

    for(VHI av=abnd.begin() ; av != abnd.end() ; av++ )
    {
        P ap = a->point(*av);
        int ai = av->idx();

        float bmin(FLT_MAX);
        VH bclosest = bbnd[0] ; 

        for(VHI bv=bbnd.begin() ; bv != bbnd.end() ; bv++ )
        {
            P bp = b->point(*bv);
            int bi = bv->idx();

            P dp = bp - ap ; 
            float dpn = dp.norm();
          
            if( dpn < bmin )
            {
                bmin = dpn ; 
                bclosest = *bv ; 
            }   
#ifdef DEBUG
            std::cout 
                << " (" << std::setw(3) << ai
                << "->" << std::setw(3) << bi 
                << ")" 
                << " ap " << std::setprecision(3) << std::fixed << std::setw(20)  << ap
                << " bp " << std::setprecision(3) << std::fixed << std::setw(20)  << bp 
                << " dp " << std::setprecision(3) << std::fixed << std::setw(20)  << dp
                << " dpn " << std::setprecision(3) << std::fixed << std::setw(10)  << dpn
                << std::endl ;  
#endif
        } 
        
        P bpc = b->point(bclosest);
        P dpc = bpc - ap ; 
        int bic = bclosest.idx();

        a2b[*av] = bclosest ; 

        std::cout 
                << " (" << std::setw(3) << ai
                << "->" << std::setw(3) << bic 
                << ")" 
                << " ap " << std::setprecision(3) << std::fixed << std::setw(20)  << ap
                << " bpc " << std::setprecision(3) << std::fixed << std::setw(20)  << bpc 
                << " dpc " << std::setprecision(3) << std::fixed << std::setw(20)  << dpc
                << " dpcn " << std::setprecision(3) << std::fixed << std::setw(10)  << dpc.norm()
                << std::endl ;  

    }
    return a2b ; 
}


template <typename MeshT>
void MWrap<MeshT>::createWithWeldedBoundary(MWrap<MeshT>* wa, MWrap<MeshT>* wb, std::map<typename MeshT::VertexHandle, typename MeshT::VertexHandle>& a2b) 
{ 
 /* 
   Difficult to draw diagonals not drawn

        +---+---+       +---+---+
    A   |   |   |       |   |   |  C 
        |   |   |       |   |   |
        +---+---+       +---+---+
    gap . \ . / .       |   |   |     
        +---+---+       +---+---+
        |   |   |       |   |   |
    B   |   |   |       |   |   |
        +---+---+       +---+---+


    Assume the gap is small enough to not need any new vertices, 
    just some new faces to fill out the flange.

    1 direct edge across to nearest, 
    1 diagonal across to next along other meshes boundary  
    2 tri-faces for each pair-of-pairs of verts 

  */

    typedef typename MeshT::VertexHandle VH ;
    typedef typename std::vector<VH> VHV ;
    typedef typename std::map<VH,VH> VHM ;

    LOG(info) << "createWithWeldedBoundary " << a2b.size() ; 

    MeshT* c = m_mesh ; 

    VHM a2c ;
    wa->copyTo(c, a2c );

    VHM b2c ;
    wb->copyTo(c, b2c );

    VHV& abnd = wa->getBoundaryLoop();   // ordered vh around the A border  

    for(typename VHV::iterator v=abnd.begin() ; v != abnd.end() ; v++)
    {
        VH av = *v ; 
        VH bv = a2b[av] ;

        // convert handles from a and b lingo into c lingo
        VH avc = a2c[av] ;  
        VH bvc = b2c[bv] ; 

        std::cout 
             << "(" << av.idx() << "->" << bv.idx() << ")" 
             << "(" << avc.idx() << "->" << bvc.idx() << ")" 
             << std::endl ; 
    }

    /*
           |           |           |
       A   |           |           |
        --a0----------a1----------a2
           .  f0   .   .           
           .  .   f1   .
        --b0----------b1----
      B    |           |
           |           |

    */

    unsigned int N = abnd.size() ;

    for(unsigned int i=0 ; i < N ; i++)
    {
         // special case i=0 to join with N-1, around the boundary loop
         VH a0 = i == 0 ? abnd[N-1] : abnd[i-1] ;   
         VH a1 = abnd[i] ; 

         VH b0 = a2b[a0] ; 
         VH b1 = a2b[a1] ; 

         VH a0c = a2c[a0] ; 
         VH a1c = a2c[a1]   ; 

         VH b0c = b2c[b0] ; 
         VH b1c = b2c[b1]   ; 

         // opposite winding order causes lots of complex edge errors
         c->add_face( a0c, a1c, b0c );
         c->add_face( b1c, b0c, a1c );
        
#ifdef DEBUG 
        std::cout 
             << " a0 " << std::setw(3) << a0.idx() 
             << " a1 " << std::setw(3) << a1.idx() 
             << " b0 " << std::setw(3) << b0.idx() 
             << " b1 " << std::setw(3) << b1.idx() 
             << " a0c " << std::setw(3) << a0c.idx() 
             << " a1c " << std::setw(3) << a1c.idx() 
             << " b0c " << std::setw(3) << b0c.idx() 
             << " b1c " << std::setw(3) << b1c.idx() 
             << std::endl ; 
#endif
    }

    c->request_face_normals();
    c->update_normals();

}




template <typename MeshT>
void MWrap<MeshT>::write(const char* tmpl, unsigned int index)
{
    MeshT* mesh = m_mesh ; 

    char path[128] ;
    snprintf( path, 128, tmpl, index );

    LOG(info) << "write " << path ; 
    try
    {
       if ( !OpenMesh::IO::write_mesh(*mesh, path) )
       {
           std::cerr << "Cannot write mesh to file" << std::endl;
       }
    }
    catch( std::exception& x )
    {
        std::cerr << x.what() << std::endl;
    }
}





template class MWrap<MyMesh>;
