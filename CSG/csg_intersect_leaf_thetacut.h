#pragma once

/**
intersect_leaf_thetacut
--------------------------




           .       \             /       .
             .      \           /      .
               .     \         /     .
                 .    \       /    .
                   .   \     /   . 
        ------------0---1---2---3----------------
                       . \ / .
          - - - - - - - - O - - - - - - - - -   


                  



             - - - - - - . O .- - - - - - - -
                       .  / \  .
             --------0---1---2---3-----------
                   .    /     \    .
                 .     /       \     .
               .      /         \      .
             .       /           \       . 



There are lots of cases to consider, so 
it is highly preferable to not make assumptions
and keep all root alive and make decision of
which root to use once. 



Because its unbounded need some special
handling of MISS that would intersect with 
the "otherside" arc at infinity. 

As the relevant angles are geospecific it 
needs to be done here in the leaf. 
This is a bit similar to the handling of complemented solids 
but there is a difference in that the otherside intersects
of a complemented solid are not restricted in angle. 

Need a way to signal that an unbounded MISS 
should be promoted to an EXIT at infinity. 

See CSG/tests/CSGClassifyTest.cc for thoughts on how 
to do this.



                        unbounded
                            MISS
                              1
                              |
                    .         |         .
                              |     |      .
                 .    \       0     | /       .
                       \            |/
               .        \           1          .
                         \         /|
              .           \       / |            .
                           \     /  |
             .         - - -0- - 1 -|- - - - -   
                             \ /    |            .  
             .                O     |     0 - - - - - - - - - - 1
                            .   .   |           .           (unbounded MISS -> EXIT)
              .           .       . |
                        .           0         . 
                .     .             | .     
                    .               |   .   .
                  .                       .


Notice: 

1. rays from origin can never intersect the thetacut, at least not until infinity
2. thetacut is an unbounded shape : which means there is an "arc" at infinity 
 
The two cones are rotationally symmetric about Z axis : so consider (radial, vertical) (R,Z) 2D space

     Z
     |
     |            /
     |           /
     |          /
     |         /  
     |        +(sinTheta0, cosTheta0)  = (0,1) when theta0_pi = 0.
     |       /              
     |      /
     |     /     +  +  
     |    /   d.z| /  ( d_xy , d.z )   (radial, vertical) components of ray direction  
     |   /       |/
     |  /        +--+  
     | /             sqrt(d.x*d.x+d.y*d.y) = d_xy
     |/
     +- - - - - - - - - - -  R
     |\
     | \
     |  \
     |   \
     |    \
     |     \
     |      \
     |       \
     |        \
     |         + (sinTheta1, cosTheta1) = (0, -1)   when theta1_pi = 1.0 
     |          \
     |           \
    

2D cross product (embedded in 3D RZW space at W=0) between theta vectors shown 
above and ray direction is proportional to the sine of the angle between them.  

Using adhoc sign convention::

   (v0.xy, v0.z ) ^ ( d_xy, d.z ) =  v0.z*d_xy - v0.xy*d.z ; 

   cross0 =  (sinTheta0, cosTheta0 ) ^ (d_xy, d.z )  = cosTheta0*d_xy - sinTheta0*d.z  
   cross1 =  (sinTheta1, cosTheta1 ) ^ (d_xy, d.z )  = cosTheta1*d_xy - sinTheta1*d.z  

cross0 > 0.f && cross1 < 0.f 
    means the ray direction is between the theta0 and theta1 cone directions 
    so a "miss" would eventually exit the "otherside" of the unbounded thetacut at infinity
    (assuming the ray starts within the shape) 
    HMM: currently not checking are inside shape but it seems not be matter 
    (could check are inside by similar cross-trick with the ray_origin)

**/

LEAF_FUNC
bool intersect_leaf_thetacut(float4& isect, const quad& q0, const quad& q1, const float t_min, const float3& o, const float3& d)
{   
    const float& cosTheta0   = q0.f.x ; 
    const float& sinTheta0   = q0.f.y ;
    const float& cosTheta1   = q0.f.z ; 
    const float& sinTheta1   = q0.f.w ;
    const float& tanTheta0sq = q1.f.x ; 
    const float& tanTheta1sq = q1.f.y ; 

    // HMM: the signs are just used to check sign of the intersect indicates are not intersecting the mirror cone 
    //     so could just use cosTheta itself ? 
    const float cosTheta0Sign = cosTheta0 < 0.f ? -1.f : 1.f ;
    const float cosTheta1Sign = cosTheta1 < 0.f ? -1.f : 1.f ;
  

    const float dxx_dyy = d.x * d.x + d.y * d.y ;  
    const float d_xy = sqrt( dxx_dyy ); 
    const float cross0 = cosTheta0*d_xy - sinTheta0*d.z; 
    const float cross1 = cosTheta1*d_xy - sinTheta1*d.z; 
    bool unbounded_exit = cross0 > 0.f && cross1 < 0.f ;   

#ifdef DEBUG
    printf("//intersect_leaf_thetacut q0.f  (%10.4f %10.4f %10.4f %10.4f) %s \n" , q0.f.x, q0.f.y, q0.f.z, q0.f.w, "cosTheta0/sinTheta0/cosTheta1/sinTheta1"  ) ; 
    printf("//intersect_leaf_thetacut q1.f  (%10.4f %10.4f %10.4f %10.4f)\n" , q1.f.x, q1.f.y, q1.f.z, q1.f.w ) ; 
    printf("//intersect_leaf_thetacut dxx_dyy %10.4f d_xy %10.4f d.z %10.4f \n", dxx_dyy, d_xy, d.z  ); 
    printf("//intersect_leaf_thetacut cross0 %10.4f cosTheta0*d_xy - sinTheta0*d.z \n", cross0 );
    printf("//intersect_leaf_thetacut cross1 %10.4f cosTheta1*d_xy - sinTheta1*d.z\n", cross1 );
    printf("//intersect_leaf_thetacut unbounded_exit %d \n", unbounded_exit ); 
#endif


    // quadratic coefficients for intersection of ray with theta0 cone     
    float dd  = dxx_dyy - d.z * d.z * tanTheta0sq ;
    float od  = o.x * d.x + o.y * d.y - o.z * d.z * tanTheta0sq ;
    float oo  = o.x * o.x + o.y * o.y - o.z * o.z * tanTheta0sq ;
    float disc = od * od - oo * dd ;
    bool intersects = disc > 0.f; 
    float discRoot = intersects ? sqrt(disc) : 0.f; 

    float t_cand = intersects ? (-od + discRoot) / dd : RT_DEFAULT_MAX;
    float t0     = intersects ? (-od - discRoot) / dd : RT_DEFAULT_MAX;


    // intersect on z-mirror cone  or too close   
    if (cosTheta0Sign * (t_cand * d.z + o.z) < 0.f  || t_cand <= t_min) t_cand = RT_DEFAULT_MAX;   

    // intersect not on z-mirror cone and not too close 
    if (cosTheta0Sign * (t0     * d.z + o.z) > 0.f  && t0 > t_min     ) t_cand = fminf(t_cand, t0); 


    /*
    THIS IS TRYING TO AVOID KEEPING ALL THE  ROOTS ALIVE AT ONCE TO REDUCE RESOURCES : 
    BUT IN THE PROCESS IT MAKES ASSUMPTIONS THAT MAY NOT ALWAYS BE TRUE.

    TO WORK IN CSG COMBINATION IT MUST BE POSSIBLE FOR t_min CUTTING 
    TO INVALIDATE ANY ROOT : SO IT IS WRONG TO TRY TO CHOOSE A 
    ROOT FROM ONE CONE BEFORE CONSIDERING THE ROOTS FROM THE OTHER 

    TODO: try to tickle this itch by choosing an appropriate t_min, ray_origin, ray_direction 
    */

    // modify quadratic coefficients to hop to the other cone 
    dd += d.z * d.z * (tanTheta0sq - tanTheta1sq );
    od += o.z * d.z * (tanTheta0sq - tanTheta1sq );
    oo += o.z * o.z * (tanTheta0sq - tanTheta1sq );
    disc = od * od - oo * dd ;

    intersects = disc > 0.f;
    discRoot = intersects ? sqrt(disc) : 0.f;

    t0 =             intersects ? (-od + discRoot) / dd : RT_DEFAULT_MAX;
    const float t1 = intersects ? (-od - discRoot) / dd : RT_DEFAULT_MAX;

    if (cosTheta1Sign * (t0 * d.z + o.z) > 0.f && t0 > t_min) t_cand = fminf(t_cand, t0);
    if (cosTheta1Sign * (t1 * d.z + o.z) > 0.f && t1 > t_min) t_cand = fminf(t_cand, t1);

    /*

         n   = [0,0,1]  normal the plane : for when cones degenerate into plane 
         p   = o + t*d 
         p.z = o.z + t*d.z = 0                 


          -------*------------- z = 0 
                /
               /
              /        t_plane = -o.z /d.z 
             /
            o

    */

    const float t_plane = -o.z / d.z;
    const bool plane = cosTheta0 * cosTheta1 == 0.0 && t_plane > t_min && t_cand > t_plane ;

/**
plane:true
    one of the cones has degenerated to a plane (theta 0.5) and has a candidate intersect 
    (cosTheta0/1 are arranged to be precisely zero for angle 0.5) 

hmm: thats a bit funny the imprecise intersect from the degenerate cone may be competing 
here with the one from the more precise plane 
**/

    const bool valid = t_cand < RT_DEFAULT_MAX || plane;

    if (valid) {
        const bool side = t_cand == t0 || t_cand == t1; 
        // when t_cand is equal to the current t0 or t1 it means that the itersect is with the theta1 cone and not the theta0 cone

        // XY cross section of the two cones are two circles : with .xy components of normals radially outwards and inwards    
        isect.x = plane ? 0.f                             : (side ?  cosTheta1Sign * (o.x + t_cand * d.x)                : -cosTheta0Sign * (o.x + t_cand * d.x));
        isect.y = plane ? 0.f                             : (side ?  cosTheta1Sign * (o.y + t_cand * d.y)                : -cosTheta0Sign * (o.y + t_cand * d.y));
        isect.z = plane ? (cosTheta0 == 0.f ? 1.f : -1.f) : (side ? -cosTheta1Sign * (o.z + t_cand * d.z) * tanTheta1sq  :  cosTheta0Sign * (o.z + t_cand * d.z) * tanTheta0sq );
        isect = normalize(isect);   
        // SCB: normalizing a float4 : unfounded assumption that isect.w = 0 

        isect.w = plane ? t_plane : t_cand;
    }
    else if( unbounded_exit )
    {
        isect.y = -isect.y ;  // -0.f signflip signalling that can promote MISS to EXIT at infinity 
        // TODO:maybe better return an int not a bool, so can signal more clearly  
    }

#ifdef DEBUG
    printf("//intersect_leaf_thetacut isect (%10.4f %10.4f %10.4f %10.4f) valid %d  \n" , isect.x, isect.y, isect.z, isect.w, valid ) ; 
#endif
    return valid ; 
}


/**
SCB comments on intersect_leaf_thetacut_lucas

1. normalize(isect) a float4 is a bug : you are requiring isect.w to be zero 

2. you say same maths as intersect_node_cone (now intersect_leaf_cone)
   but you use entirely different language 

3. invalidate candidates by setting to t_min is needed for the shape to 
   work in CSG combinations as need expected behaviour as t_min is varied



intersect_leaf_thetacut_lucas
--------------------------------
Based on same maths behind intersect_node_cone, see there for explanation.

WORKS FOR 0 <= THETA <= 180 BUT BEWARE: USER NEEDS TO BE CAREFUL WHEN DEFINING QUAD, MUST BE SET
    //    q.f.x = theta0 == 0.5 ? 0.0 : cos(theta0 * pi ) / abs(cos(theta0 * pi));
    //    q.f.y = theta0 == 0.5 ? 0.0 : tan(theta0 * pi) * tan(theta0 * pi);
    //    q.f.z = theta1 == 0.5 ? 0.0 :  cos(theta1 * pi) / abs(cos(theta1 * pi));
    //    q.f.w = theta1 == 0.5 ? 0.0 : tan(theta1 * pi) * tan(theta1 * pi);
    // if .x and .z are not set 0.0 cos(...) float inaccuracy will mean plane not recognised.
    // if .y and .w are not set 0.0 magnitudes will give wacky values, not worth the risk.
    
**/
LEAF_FUNC
bool intersect_leaf_thetacut_lucas(float4& isect, const quad& thetaDat, const float t_min, const float3& rayOrigin, const float3& rayDirection)
{   //thetaData contains x = cos(theta0)/abs(cos(theta0)), y = tan^2 (theta0), z = cos(theta1)/abs(cos(theta1)), w = tan^2 (theta1)

    float dirMag = rayDirection.x * rayDirection.x + rayDirection.y * rayDirection.y - rayDirection.z * rayDirection.z * thetaDat.f.y;
    float originDirMag = rayOrigin.x * rayDirection.x + rayOrigin.y * rayDirection.y - rayOrigin.z * rayDirection.z * thetaDat.f.y;
    float originMag = rayOrigin.x * rayOrigin.x + rayOrigin.y * rayOrigin.y - rayOrigin.z * rayOrigin.z * thetaDat.f.y;
    float disc = originDirMag * originDirMag - originMag * dirMag;

    bool intersects = disc > 0.f; 
    float discRoot = intersects ? sqrt(disc) : 0.f; //avoids sqrt(NEGATIVE)

    float t_cand = intersects ? (-originDirMag + discRoot) / dirMag : RT_DEFAULT_MAX; //beginning on t_cand saves defining extra variable

    if (thetaDat.f.x * (t_cand * rayDirection.z + rayOrigin.z) < 0.f || t_cand <= t_min) t_cand = RT_DEFAULT_MAX; //eliminates bad t_cand/mirror cone 

    float t0 = intersects ? (-originDirMag - discRoot) / dirMag : RT_DEFAULT_MAX;
    if (thetaDat.f.x * (t0 * rayDirection.z + rayOrigin.z) > 0.f && t0 > t_min) t_cand = fminf(t_cand, t0); 
    //works here since t_cand will already be either valid or INF

    dirMag += rayDirection.z * rayDirection.z * (thetaDat.f.y - thetaDat.f.w);
    originDirMag += rayOrigin.z * rayDirection.z * (thetaDat.f.y - thetaDat.f.w);
    originMag += rayOrigin.z * rayOrigin.z * (thetaDat.f.y - thetaDat.f.w);
    disc = originDirMag * originDirMag - originMag * dirMag;

    intersects = disc > 0.f;
    discRoot = intersects ? sqrt(disc) : 0.f;

    t0 = intersects ? (-originDirMag + discRoot) / dirMag : RT_DEFAULT_MAX;
    if (thetaDat.f.z * (t0 * rayDirection.z + rayOrigin.z) > 0.f && t0 > t_min) t_cand = fminf(t_cand, t0);

    const float t1 = intersects ? (-originDirMag - discRoot) / dirMag : RT_DEFAULT_MAX;
    if (thetaDat.f.z * (t1 * rayDirection.z + rayOrigin.z) > 0.f && t1 > t_min) t_cand = fminf(t_cand, t1);


    const float t_plane = -rayOrigin.z / rayDirection.z;
    const bool plane = thetaDat.f.x * thetaDat.f.z == 0.0 && t_plane > t_min && t_cand > t_plane;
    // SCB                                 ^^^^^^^^^^^^^^^^^  
    const bool valid = t_cand < RT_DEFAULT_MAX || plane;

    if (valid) {
        const bool side = t_cand == t0 || t_cand == t1; //corrects normals for both cones/planes around 90 degrees

        isect.x = plane ? 0.0 : (side ? thetaDat.f.z * (rayOrigin.x + t_cand * rayDirection.x)
                                       : - thetaDat.f.x * (rayOrigin.x + t_cand * rayDirection.x));

        //SCB            ^^^^ ALLWAYS 0.f OTHERWISE POINTLESS DOUBLES : BAD FOR PERFORMANCE ON GPU  

        isect.y = plane ? 0.0 : (side ? thetaDat.f.z * (rayOrigin.y + t_cand * rayDirection.y)
                                       : - thetaDat.f.x * (rayOrigin.y + t_cand * rayDirection.y));

        //SCB              ^^^^^^^^  : DITTO
        isect.z = plane ? (thetaDat.f.x == 0.0 ? 1.0 : -1.0)
        //SCB                         ^^^^^^^^^^^^^^^^^^^^^^^^^^^ DITTO
        //SCB         using the special setting of zero to determine which of the cones degenerated to the plane 

                        : ( side ? - thetaDat.f.z * (rayOrigin.z + t_cand * rayDirection.z) * thetaDat.f.w
                                 : thetaDat.f.x * (rayOrigin.z + t_cand * rayDirection.z) * thetaDat.f.y );
        isect = normalize(isect);
        isect.w = plane ? t_plane : t_cand;
    }


#ifdef DEBUG
    const quad& q0 = thetaDat ; 
    printf("//intersect_leaf_thetacut_lucas q0.f (%10.4f %10.4f %10.4f %10.4f) valid %d  isect  (%10.4f %10.4f %10.4f %10.4f) \n" , 
           q0.f.x, q0.f.y, q0.f.z, q0.f.z, valid, isect.x, isect.y, isect.z, isect.w ) ; 
#endif


    return valid;
}


