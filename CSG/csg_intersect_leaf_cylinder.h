#pragma once

#include "csg_intersect_leaf_phi_wedge.h"
/**
intersect_leaf_cylinder : a much simpler approach than intersect_leaf_oldcylinder
-------------------------------------------------------------------------------------------

The two cylinder imps were compared with tests/CSGIntersectComparisonTest.cc.
Surface distance comparisons show the new imp is more precise and does
not suffer from near-axial spurious intersects beyond the ends.

intersect_leaf_cylinder

   * simple as possible approach, minimize the flops
   * axial special case removed, might need to put back if find some motivation to do that

intersect_leaf_oldcylinder

   * pseudo-general approach, based on implementation from book RTCD
   * had axial special case bolted on for unrecorded reason, some glitch presumably


There are up to six possible t

* 2 from curved sheet, obtained from solving quadratic, that must be within z1 z2 range
* 2 from endcaps that must be within r2 range
* 2 from radial phi walls when a partial-phi interval is present

Finding the intersect means finding the smallest t from the four that exceeds t_min

Current approach keeps changing t_cand, could instead collect all four potential t
into a float4 and then pick from that ?

**/

LEAF_FUNC
void intersect_leaf_cylinder( bool& valid_isect, float4& isect, const quad& q0, const quad& q1, const float t_min, const float3& ray_origin, const float3& ray_direction )
{
    const float& r = q0.f.w;
    const float& z1 = q1.f.x;
    const float& z2 = q1.f.y;
    const float  startPhi = q0.f.x;
    const float  deltaPhi = q0.f.y;
    const bool   has_phi_wedge = csg_has_phi_wedge(deltaPhi);
    const float& ox = ray_origin.x;
    const float& oy = ray_origin.y;
    const float& oz = ray_origin.z;
    const float& vx = ray_direction.x;
    const float& vy = ray_direction.y;
    const float& vz = ray_direction.z;

    const float r2 = r * r;
    const float a = vx * vx + vy * vy; // see the cylinder derivation notes
    const float b = ox*vx + oy*vy ; 
    const float c = ox*ox + oy*oy - r2 ; 

    float t_near, t_far, disc, sdisc ;   
    robust_quadratic_roots_disqualifying(t_min, t_near, t_far, disc, sdisc, a, b, c); //  Solving:  a t^2 + 2 b t +  c = 0 
    float z_near = oz+t_near*vz ; 
    float z_far  = oz+t_far*vz ; 

    const float t_z1cap = (z1 - oz)/vz ; 
    const float r2_z1cap = (ox+t_z1cap*vx)*(ox+t_z1cap*vx) + (oy+t_z1cap*vy)*(oy+t_z1cap*vy) ;  

    const float t_z2cap = (z2 - oz)/vz ;  
    const float r2_z2cap = (ox+t_z2cap*vx)*(ox+t_z2cap*vx) + (oy+t_z2cap*vy)*(oy+t_z2cap*vy) ;  

#ifdef DEBUG
    //printf("// t_z1cap %10.4f r2_z1cap %10.4f \n", t_z1cap, r2_z1cap ); 
    //printf("// t_z2cap %10.4f r2_z2cap %10.4f \n", t_z2cap, r2_z2cap ); 
#endif

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
        phi_start_ok = csg_intersect_phi_wall(t_phi_start, n_start_x, n_start_y, startPhi, true, ox, oy, vx, vy);
        if (phi_start_ok)
        {
            const float x = ox + t_phi_start * vx;
            const float y = oy + t_phi_start * vy;
            const float z = oz + t_phi_start * vz;
            phi_start_ok = x * x + y * y <= r2 && z > z1 && z < z2;
        }

        phi_end_ok = csg_intersect_phi_wall(t_phi_end, n_end_x, n_end_y, startPhi + deltaPhi, false, ox, oy, vx, vy);
        if (phi_end_ok)
        {
            const float x = ox + t_phi_end * vx;
            const float y = oy + t_phi_end * vy;
            const float z = oz + t_phi_end * vz;
            phi_end_ok = x * x + y * y <= r2 && z > z1 && z < z2;
        }
    }

    float t_cand = CUDART_INF_F ;
    if (t_near > t_min && z_near > z1 && z_near < z2 && csg_in_phi_wedge(ox + t_near * vx, oy + t_near * vy, startPhi, deltaPhi) && t_near < t_cand)
        t_cand = t_near;
    if (t_far > t_min && z_far > z1 && z_far < z2 && csg_in_phi_wedge(ox + t_far * vx, oy + t_far * vy, startPhi, deltaPhi) && t_far < t_cand)
        t_cand = t_far;
    if (t_z1cap > t_min && r2_z1cap <= r2 && csg_in_phi_wedge(ox + t_z1cap * vx, oy + t_z1cap * vy, startPhi, deltaPhi) && t_z1cap < t_cand)
        t_cand = t_z1cap;
    if (t_z2cap > t_min && r2_z2cap <= r2 && csg_in_phi_wedge(ox + t_z2cap * vx, oy + t_z2cap * vy, startPhi, deltaPhi) && t_z2cap < t_cand)
        t_cand = t_z2cap;
    if (t_phi_start > t_min && phi_start_ok && t_phi_start < t_cand)
        t_cand = t_phi_start;
    if (t_phi_end > t_min && phi_end_ok && t_phi_end < t_cand)
        t_cand = t_phi_end;

    valid_isect = t_cand > t_min && t_cand < CUDART_INF_F ; 
    if(valid_isect)
    {
        bool sheet = (t_cand == t_near || t_cand == t_far);
        bool cap = (t_cand == t_z1cap || t_cand == t_z2cap);
        if (sheet)
        {
            isect.x = (ox + t_cand * vx) / r;
            isect.y = (oy + t_cand * vy) / r;
            isect.z = 0.f;
        }
        else if (cap)
        {
            isect.x = 0.f;
            isect.y = 0.f;
            isect.z = t_cand == t_z1cap ? -1.f : 1.f;
        }
        else
        {
            const bool start = t_cand == t_phi_start;
            isect.x = start ? n_start_x : n_end_x;
            isect.y = start ? n_start_y : n_end_y;
            isect.z = 0.f;
        }
        isect.w = t_cand ;      
    }
}





/**
distance_leaf_cylinder
------------------------

* https://www.iquilezles.org/www/articles/distfunctions/distfunctions.htm

Capped Cylinder - exact

float sdCappedCylinder( vec3 p, float h, float r )
{
  vec2 d = abs(vec2(length(p.xz),p.y)) - vec2(h,r); 
  // dont follow  would expect h <-> r with radius to be on the first dimension and height on second
     
  return min(max(d.x,d.y),0.0) + length(max(d,0.0));
}


                      p
                      +
                      | 
                      |
                      | 
  - - - +---r----+----+---+ - - - - - -
        |        :        |
        h        :        +------+ p    
        |        :        |
        |        :        |
        +--------+--------+
        |        :        |
        |        :        |
        |        :        |
        |        :        |
  - - - +--------+--------+ - - - - - - - 





The SDF rules for CSG combinations::

    CSG union(l,r)     ->  min(l,r)
    CSG intersect(l,r) ->  max(l,r)
    CSG difference(l,r) -> max(l,-r)    [aka subtraction, corresponds to intersecting with a complemented rhs]


**/


LEAF_FUNC
float distance_leaf_cylinder( const float3& pos, const quad& q0, const quad& q1 )
{
    const float radius = q0.f.w;
    const float z1 = q1.f.x;
    const float z2 = q1.f.y; // z2 > z1
    const float startPhi = q0.f.x;
    const float deltaPhi = q0.f.y;

    float sd_capslab = fmaxf(pos.z - z2, z1 - pos.z);
    float sd_infcyl = sqrtf(pos.x * pos.x + pos.y * pos.y) - radius;
    float sd = fmaxf(sd_capslab, sd_infcyl);

    if (csg_has_phi_wedge(deltaPhi))
        sd = fmaxf(sd, csg_distance_phi_wedge(pos.x, pos.y, startPhi, deltaPhi));

#ifdef DEBUG
    printf("//distance_leaf_cylinder sd %10.4f \n", sd ); 
#endif
    return sd ; 
}


