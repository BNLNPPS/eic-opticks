#pragma once

#include "csg_intersect_leaf_phi_wedge.h"

LEAF_FUNC
float distance_leaf_zsphere(const float3& pos, const quad& q0, const quad& q1 )
{
    const float startPhi = q0.f.x;
    const float deltaPhi = q0.f.y;
    const float radius = q0.f.w;
    const float z2 = q1.f.y;
    const float z1 = q1.f.x;

    float sd_sphere = length(pos) - radius;
    float sd_capslab = fmaxf(pos.z - z2, z1 - pos.z);
    float sd = fmaxf(sd_capslab, sd_sphere); // CSG intersect

    if (csg_has_phi_wedge(deltaPhi))
        sd = fmaxf(sd, csg_distance_phi_wedge(pos.x, pos.y, startPhi, deltaPhi));

    return sd ; 
}

/**
intersect_leaf_zsphere
------------------------

HMM: rays that look destined to land near to "apex" have a rare (order 1 in 300k) 
problem of missing the zsphere.  This is probably arising from the upper cap 
implementation acting effectively like cutting a pinhole at the apex. 

When there is no upper cap perhaps can avoid the problem by setting zmax to beyond the 
apex ? Or could have a different imp for zsphere with lower cap but no upper cap. 

Note that zsphere with no upper cap is used a lot for PMTs so a simpler imp
for zsphere without upper cut does make sense.  

NB "z2sph <= zmax" changed from "z2sph < zmax" Aug 29, 2022

The old inequality caused rare unexpected MISS for rays that would
have been expected to intersect close to the apex of the zsphere  

See : notes/issues/unexpected_zsphere_miss_from_inside_for_rays_that_would_be_expected_to_intersect_close_to_apex.rst

**/


LEAF_FUNC
void intersect_leaf_zsphere(bool& valid_isect, float4& isect, const quad& q0, const quad& q1, const float& t_min, const float3& ray_origin, const float3& ray_direction )
{
    float3      O = ray_origin;
    float3 D = ray_direction;
    const float radius = q0.f.w;
    const float startPhi = q0.f.x;
    const float deltaPhi = q0.f.y;
    const bool  has_phi_wedge = csg_has_phi_wedge(deltaPhi);

    float b = dot(O, D);               // t of closest approach to sphere center
    float c = dot(O, O)-radius*radius; // < 0. indicates ray_origin inside sphere

#ifdef DEBUG_RECORD
    printf("//[intersect_leaf_zsphere radius %10.4f b %10.4f c %10.4f \n", radius, b, c); 
#endif

    if( c > 0.f && b > 0.f )
    {
        valid_isect = false;
        return;
    }
    // Cannot intersect when ray origin outside sphere and direction away from sphere.
    // Whether early exit speeds things up (or slows things down) is another question ...

    const float zmax = q1.f.y;
    const float zmin = q1.f.x;

#ifdef DEBUG_RECORD
    bool with_upper_cut = zmax < radius ; 
    bool with_lower_cut = zmin > -radius ; 
    printf("// intersect_leaf_zsphere radius %10.4f zmax %10.4f zmin %10.4f  with_upper_cut %d with_lower_cut %d  \n", radius, zmax, zmin, with_upper_cut, with_lower_cut ); 
#endif


    float d = dot(D, D);               // NB NOT assuming normalized ray_direction

    float t1sph, t2sph, disc, sdisc ;    
    robust_quadratic_roots(t1sph, t2sph, disc, sdisc, d, b, c); //  Solving:  d t^2 + 2 b t +  c = 0 

    float z1sph = ray_origin.z + t1sph*ray_direction.z ;  // sphere z intersects
    float z2sph = ray_origin.z + t2sph*ray_direction.z ; 

#ifdef DEBUG_RECORD
    printf("// intersect_leaf_zsphere t1sph %10.4f t2sph %10.4f sdisc %10.4f \n", t1sph, t2sph, sdisc ); 
    printf("// intersect_leaf_zsphere z1sph %10.4f z2sph %10.4f zmax %10.4f zmin %10.4f sdisc %10.4f \n", z1sph, z2sph, zmax, zmin, sdisc ); 
#endif

    float idz = 1.f/ray_direction.z ; 
    float t_QCAP = (zmax - ray_origin.z)*idz ;   // upper cap intersects
    float t_PCAP = (zmin - ray_origin.z)*idz ;   // lower cap intersect 


    float t1cap = fminf( t_QCAP, t_PCAP ) ;      // order cap intersects along the ray 
    float t2cap = fmaxf( t_QCAP, t_PCAP ) ;      // t2cap > t1cap 

#ifdef DEBUG_RECORD
    bool t1cap_disqualify = t1cap < t1sph || t1cap > t2sph ; 
    bool t2cap_disqualify = t2cap < t1sph || t2cap > t2sph ;  
    printf("//intersect_leaf_zsphere t1sph %7.3f t2sph %7.3f t_QCAP %7.3f t_PCAP %7.3f t1cap %7.3f t2cap %7.3f  \n", t1sph, t2sph, t_QCAP, t_PCAP, t1cap, t2cap ); 
    printf("//intersect_leaf_zsphere  t1cap_disqualify %d t2cap_disqualify %d \n", t1cap_disqualify, t2cap_disqualify  ); 
#endif

    // disqualify plane intersects outside sphere t range
    if (t1cap < t1sph || t1cap > t2sph)
        t1cap = t_min;
    if (t2cap < t1sph || t2cap > t2sph)
        t2cap = t_min;

    if (t1cap > t_min && !csg_in_phi_wedge(O.x + t1cap * D.x, O.y + t1cap * D.y, startPhi, deltaPhi))
        t1cap = t_min;
    if (t2cap > t_min && !csg_in_phi_wedge(O.x + t2cap * D.x, O.y + t2cap * D.y, startPhi, deltaPhi))
        t2cap = t_min;
    // hmm somehow is seems unclean to have to use both z and t language

    float t_phi_start = CUDART_INF_F;
    float t_phi_end = CUDART_INF_F;
    float n_start_x = 0.f;
    float n_start_y = 0.f;
    float n_end_x = 0.f;
    float n_end_y = 0.f;
    bool  phi_start_ok = false;
    bool  phi_end_ok = false;

    if (has_phi_wedge)
    {
        phi_start_ok = csg_intersect_phi_wall(t_phi_start, n_start_x, n_start_y, startPhi, true, O.x, O.y, D.x, D.y);
        if (phi_start_ok)
        {
            const float x = O.x + t_phi_start * D.x;
            const float y = O.y + t_phi_start * D.y;
            const float z = O.z + t_phi_start * D.z;
            phi_start_ok = x * x + y * y + z * z <= radius * radius && z > zmin && z < zmax;
        }

        phi_end_ok = csg_intersect_phi_wall(t_phi_end, n_end_x, n_end_y, startPhi + deltaPhi, false, O.x, O.y, D.x, D.y);
        if (phi_end_ok)
        {
            const float x = O.x + t_phi_end * D.x;
            const float y = O.y + t_phi_end * D.y;
            const float z = O.z + t_phi_end * D.z;
            phi_end_ok = x * x + y * y + z * z <= radius * radius && z > zmin && z < zmax;
        }
    }

    float t_cand = t_min;
    if (sdisc > 0.f)
    {
        if (t1sph > t_min && z1sph > zmin && z1sph <= zmax && csg_in_phi_wedge(O.x + t1sph * D.x, O.y + t1sph * D.y, startPhi, deltaPhi))
            t_cand = t1sph;
        else if( t1cap > t_min )                                   t_cand = t1cap ;  // t1cap qualifies -> t1cap 
        else if( t2cap > t_min )                                   t_cand = t2cap ;  // t2cap qualifies -> t2cap
        else if (t2sph > t_min && z2sph > zmin && z2sph <= zmax && csg_in_phi_wedge(O.x + t2sph * D.x, O.y + t2sph * D.y, startPhi, deltaPhi))
            t_cand = t2sph;
    }

    if (t_phi_start > t_min && phi_start_ok && (t_cand <= t_min || t_phi_start < t_cand))
        t_cand = t_phi_start;
    if (t_phi_end > t_min && phi_end_ok && (t_cand <= t_min || t_phi_end < t_cand))
        t_cand = t_phi_end;

    valid_isect = t_cand > t_min ;
#ifdef DEBUG_RECORD
    printf("//intersect_leaf_zsphere valid_isect %d t_min %7.3f t1sph %7.3f t1cap %7.3f t2cap %7.3f t2sph %7.3f t_cand %7.3f \n", valid_isect, t_min, t1sph, t1cap, t2cap, t2sph, t_cand ); 
#endif

    if(valid_isect)
    {
        isect.w = t_cand ;
        if( t_cand == t1sph || t_cand == t2sph)
        {
            isect.x = (O.x + t_cand*D.x)/radius ; // normalized by construction
            isect.y = (O.y + t_cand*D.y)/radius ;
            isect.z = (O.z + t_cand*D.z)/radius ;
        }
        else if (t_cand == t_PCAP || t_cand == t_QCAP)
        {
            isect.x = 0.f ;
            isect.y = 0.f ;
            isect.z = t_cand == t_PCAP ? -1.f : 1.f ;
        }
        else
        {
            const bool start = t_cand == t_phi_start;
            isect.x = start ? n_start_x : n_end_x;
            isect.y = start ? n_start_y : n_end_y;
            isect.z = 0.f;
        }
    }

#ifdef DEBUG_RECORD
    printf("//]intersect_leaf_zsphere valid_isect %d \n", valid_isect ); 
#endif
}
