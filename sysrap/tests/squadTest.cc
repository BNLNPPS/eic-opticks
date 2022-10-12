// ./squadTest.sh 

#include "scuda.h"
#include "squad.h"

void test_qvals_float()
{
    float  v1 ; 
    float2 v2 ; 
    float3 v3 ; 
    float4 v4 ; 

    qvals( v1, "TMIN", "0.6" ); 
    qvals( v2, "CHK", "100.5   -200.1" ); 
    qvals( v3, "EYE", ".4,.2,.1" ); 
    qvals( v4, "LOOK", ".4,.2,.1,1" ); 

    std::cout << "v1 " << v1 << std::endl ; 
    std::cout << "v2 " << v2 << std::endl ; 
    std::cout << "v3 " << v3 << std::endl ; 
    std::cout << "v4 " << v4 << std::endl ; 
}


void test_qvals_int()
{
    int  v1 ; 
    int2 v2 ; 
    int3 v3 ; 
    int4 v4 ; 

    qvals( v1, "I1", "101" ); 
    qvals( v2, "I2", "101   -202" ); 
    qvals( v3, "I3", "101 202 303" ); 
    qvals( v4, "I4", "101 -202 +303 -404" ); 

    std::cout << "v1 " << v1 << std::endl ; 
    std::cout << "v2 " << v2 << std::endl ; 
    std::cout << "v3 " << v3 << std::endl ; 
    std::cout << "v4 " << v4 << std::endl ; 
}


void test_qvals_float3_x2()
{
    float3 mom ; 
    float3 pol ; 
    qvals(mom, pol, "MOM_POL", "1,0,0,0,1,0" ); 
   
    std::cout << "mom " << mom << std::endl ; 
    std::cout << "pol " << pol << std::endl ; 
}

void test_qvals_float4_x2()
{
    float4 momw ; 
    float4 polw ; 
    qvals(momw, polw, "MOMW_POLW", "1,0,0,1,0,1,0,1" ); 
   
    std::cout << "momw " << momw << std::endl ; 
    std::cout << "polw " << polw << std::endl ; 
}

void test_quad4_ephoton()
{
    quad4 p ; 
    p.ephoton(); 
    std::cout << p.desc() << std::endl ;  
}

void test_qenvint()
{
   int num = qenvint("NUM", "-1"); 
   std::cout << " num " << num << std::endl ; 
}

void test_quad4_normalize_mom_pol()
{
    quad4 p ; 
    p.zero() ;
    p.q1.f = make_float4( 1.f, 1.f, 1.f, 1.f ); 
    p.q2.f = make_float4( 1.f, 1.f, 0.f, 1.f ); 
 
    std::cout << p.desc() << std::endl ;  
    p.normalize_mom_pol(); 
    std::cout << p.desc() << std::endl ;  

}

void test_quad2_eprd()
{
    quad2 prd = quad2::make_eprd(); 
    std::cout << " prd.desc " << prd.desc() << std::endl ;  
}

void test_qvals_float4_vec(bool normalize_)
{
    std::vector<float4> v ; 
    qvals(v, "SQUADTEST_F4V", "0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4.5,4.5,4.5,4.5", normalize_ ); 
    for(unsigned i=0 ; i < v.size() ; i++) std::cout << v[i] << std::endl ; 
}

void test_quad4_set_flags_get_flags()
{
    quad4 p ; 
    p.zero(); 

    unsigned MISSING = ~0u ; 
    unsigned boundary0 = MISSING ; 
    unsigned identity0 = MISSING ; 
    unsigned idx0 = MISSING ; ; 
    unsigned flag0 = MISSING ; ; 
    float orient0 = 0.f ; 

    p.get_flags(boundary0, identity0, idx0, flag0, orient0 ); 

    assert( boundary0 == 0u ); 
    assert( identity0 == 0u ); 
    assert( idx0 == 0u ); 
    assert( flag0 == 0u ); 
    assert( orient0 == 1.f );  

    unsigned boundary1, identity1, idx1, flag1 ;
    float orient1 ; 

    // test maximum values of the fields  
    boundary1 = 0xffffu ; 
    identity1 = 0xffffffffu ; 
    idx1      = 0x7fffffffu ;  // bit 31 used for orient  
    flag1     = 0xffffu ; 
    orient1   = -1.f ; 

    p.set_flags(boundary1, identity1, idx1, flag1, orient1 ); 

    unsigned boundary2, identity2, idx2, flag2  ;
    float orient2 ; 
 
    p.get_flags(boundary2 , identity2 , idx2 , flag2, orient2  );

    std::cout 
        << " idx1 " << std::hex << idx1 
        << " idx2 " << std::hex << idx2
        << std::dec
        << std::endl 
        ; 

    assert( boundary2 == boundary1 ); 
    assert( identity2 == identity1 ); 
    assert( idx2 == idx1 );
    assert( flag2 == flag1 );  
    assert( orient2 == orient1 );  
}

void test_quad4_set_flag_get_flag()
{
    quad4 p ; 
    p.zero(); 

    unsigned flag0[2] ; 
    flag0[0] = 1024 ; 

    p.set_flag( flag0[0] ); 
    p.get_flag( flag0[1] ); 
    assert( flag0[0] == flag0[1] ); 
    assert( p.q3.u.w == 1024 ); 

    unsigned flag1[2] ; 
    flag1[0] = 2048 ; 

    p.set_flag( flag1[0] ); 
    p.get_flag( flag1[1] ); 
    assert( flag1[0] == flag1[1] ); 
    
    assert( p.q3.u.w == (1024 | 2048) ); 
}

void test_quad4_set_idx_set_prd_get_idx_get_prd()
{
    quad4 p ; 
    {
        p.zero(); 
        unsigned idx[2] ; 
        idx[0] = 0x7fffffff ; 
        p.set_idx(idx[0]); 
        p.get_idx(idx[1]); 
        assert( idx[0] == idx[1] ); 
    }
    {
        p.zero(); 
        unsigned idx[2] ; 
        unsigned boundary[2]; 
        unsigned identity[2]; 
        float orient[2] ; 

        idx[0] = 0x7fffffff ; 
        boundary[0] = 0xffff ; 
        identity[0] = 0xffffffff ; 
        orient[0] = -1.f ; 

        p.set_idx(idx[0]); 
        p.set_prd( boundary[0], identity[0], orient[0] ); 
        p.get_idx(idx[1]); 
        assert( idx[0] == idx[1] ); 

        p.get_prd( boundary[1], identity[1], orient[1] ); 
        assert( boundary[0] == boundary[1] ); 
        assert( identity[0] == identity[1] ); 
        assert( orient[0] == orient[1] ); 

        p.get_idx(idx[1]); 
        assert( idx[0] == idx[1] ); 
    }
}

void test_quad4_idx_orient()
{
    quad4 p ; 
    p.zero(); 

    unsigned idx[2] ; 
    float orient[2] ; 
    
    for(unsigned i=0 ; i < 1000 ; i++)
    {
        idx[0] = i ; 
        idx[1] = 0 ; 

        orient[0] = i % 2 == 0 ? -1.f : 1.f ; 
        orient[1] = 0.f ; 

        p.set_idx(idx[0]); 
        p.set_orient( orient[0] ); 

        p.get_idx(idx[1]); 
        p.get_orient(orient[1]); 

        assert( idx[0] == idx[1] ); 
        assert( orient[0] == orient[1] ); 
    }
}

void test_qselector()
{
    unsigned hitmask = 0xdeadbeef ;
    qselector<quad4> selector(hitmask) ; 

    quad4 p ; 
    p.zero(); 

    bool select_0 = selector(p) ; assert( select_0 == false );  

    p.q3.u.w = ~0u ;                                              // all bits set 
    bool select_1 = selector(p) ; assert( select_1 == true );  

    p.q3.u.w = hitmask  ; 
    bool select_2 = selector(p) ; assert( select_2 == true );  

    p.q3.u.w = hitmask & 0x7fffffff ;                            // knock out one bit from the 0xd
    bool select_3 = selector(p) ; assert( select_3 == false );  
}

void test_union1()
{
    quad q ; 

    q.f.x = 0.f ;  
    q.f.y = 0.f ;  
    q.f.z = 0.f ;  
    q.f.w = 1.f ;  

    std::cout 
        << " q.f.x " << std::setw(10) << q.f.x 
        << " q.i.x " << std::setw(10) << q.i.x
        << std::endl 
        << " q.f.y " << std::setw(10) << q.f.y 
        << " q.i.y " << std::setw(10) << q.i.y
        << std::endl 
        << " q.f.z " << std::setw(10) << q.f.z 
        << " q.i.z " << std::setw(10) << q.i.z
        << std::endl 
        << " q.f.w " << std::setw(10) << q.f.w 
        << " q.i.w " << std::setw(10) << q.i.w
        << std::endl 
        ;

    assert( q.i.w == 1065353216  ); 
    assert( q.u.w == 1065353216u ); 




}

int main(int argc, char** argv)
{
    /*
    test_qvals_float(); 
    test_qvals_int(); 
    test_qvals_float3_x2(); 
    test_qvals_float4_x2(); 
    test_qenvint(); 
    test_quad4_normalize_mom_pol(); 
    test_quad4_ephoton(); 
    test_quad2_eprd(); 
    test_qvals_float4_vec(false); 
    test_qvals_float4_vec(true); 
    test_quad4_set_flags_get_flags(); 
    test_quad4_set_flag_get_flag(); 
    test_quad4_set_idx_set_prd_get_idx_get_prd(); 
    test_quad4_idx_orient(); 
    test_qselector(); 
    test_qphoton(); 
    */

    test_union1(); 



    return 0 ; 
}
