/**
intersect_leaf_phi_wedge_test.cc
================================

Exercises phi intervals baked into centred ZSphere and Cylinder leaves. The
radial-wall cases are essential: filtering only curved roots would return the
wrong, later surface.
**/

#include <cmath>
#include <cstdio>

#include "scuda.h"
#include "squad.h"

#include "csg_intersect_leaf_head.h"
#include "csg_robust_quadratic_roots.h"

#include "csg_intersect_leaf_cylinder.h"
#include "csg_intersect_leaf_zsphere.h"

namespace
{
int failures = 0;

void check(const char* label, bool condition)
{
    std::printf("// %-62s %s\n", label, condition ? "PASS" : "FAIL");
    if (!condition)
        failures++;
}

bool close(float actual, float expected, float tolerance = 1.e-3f)
{
    return std::fabs(actual - expected) <= tolerance * (1.f + std::fabs(expected));
}

quad phi_primitive(float startPhi, float deltaPhi, float radius)
{
    quad q;
    q.f = make_float4(startPhi, deltaPhi, 0.f, radius);
    return q;
}

quad z_range(float z1, float z2)
{
    quad q;
    q.f = make_float4(z1, z2, 0.f, 0.f);
    return q;
}

struct Result
{
    bool   valid;
    float4 isect;
    float3 hit;
};

Result sphere(const quad& q0, const quad& q1, const float3& origin, const float3& direction)
{
    Result result = {false, make_float4(0.f, 0.f, 0.f, 0.f), make_float3(0.f, 0.f, 0.f)};
    intersect_leaf_zsphere(result.valid, result.isect, q0, q1, 0.f, origin, direction);
    result.hit = make_float3(
        origin.x + result.isect.w * direction.x,
        origin.y + result.isect.w * direction.y,
        origin.z + result.isect.w * direction.z);
    return result;
}

Result cylinder(const quad& q0, const quad& q1, const float3& origin, const float3& direction)
{
    Result result = {false, make_float4(0.f, 0.f, 0.f, 0.f), make_float3(0.f, 0.f, 0.f)};
    intersect_leaf_cylinder(result.valid, result.isect, q0, q1, 0.f, origin, direction);
    result.hit = make_float3(
        origin.x + result.isect.w * direction.x,
        origin.y + result.isect.w * direction.y,
        origin.z + result.isect.w * direction.z);
    return result;
}

void check_quarter_wedge()
{
    const float radius = 100.f;
    const float quarter = 0.5f * CUDART_PI_F;
    const quad  q0 = phi_primitive(0.f, quarter, radius);
    const quad  q1 = z_range(-radius, radius);
    const float c = std::cos(0.25f * CUDART_PI_F);
    const float s = std::sin(0.25f * CUDART_PI_F);

    Result sr = sphere(q0, q1, make_float3(2.f * radius * c, 2.f * radius * s, 0.f), make_float3(-c, -s, 0.f));
    check("quarter ZSphere hits its curved surface", sr.valid && close(sr.hit.x, radius * c) && close(sr.hit.y, radius * s));

    Result cr = cylinder(q0, q1, make_float3(2.f * radius * c, 2.f * radius * s, 0.f), make_float3(-c, -s, 0.f));
    check("quarter Cylinder hits its curved surface", cr.valid && close(cr.hit.x, radius * c) && close(cr.hit.y, radius * s));

    sr = sphere(q0, q1, make_float3(-200.f, 50.f, 0.f), make_float3(1.f, 0.f, 0.f));
    check("quarter ZSphere first hits the end-phi wall", sr.valid && close(sr.isect.w, 200.f) && close(sr.hit.x, 0.f) && close(sr.isect.x, -1.f));

    cr = cylinder(q0, q1, make_float3(-200.f, 50.f, 0.f), make_float3(1.f, 0.f, 0.f));
    check("quarter Cylinder first hits the end-phi wall", cr.valid && close(cr.isect.w, 200.f) && close(cr.hit.x, 0.f) && close(cr.isect.x, -1.f));

    sr = sphere(q0, q1, make_float3(-200.f, -50.f, 0.f), make_float3(1.f, 0.f, 0.f));
    cr = cylinder(q0, q1, make_float3(-200.f, -50.f, 0.f), make_float3(1.f, 0.f, 0.f));
    check("quarter ZSphere misses a ray wholly outside the wedge", !sr.valid);
    check("quarter Cylinder misses a ray wholly outside the wedge", !cr.valid);

    check("quarter ZSphere distance is negative inside", distance_leaf_zsphere(make_float3(50.f, 50.f, 0.f), q0, q1) < 0.f);
    check("quarter ZSphere distance is positive outside phi", distance_leaf_zsphere(make_float3(-50.f, 50.f, 0.f), q0, q1) > 0.f);
    check("quarter Cylinder distance is negative inside", distance_leaf_cylinder(make_float3(50.f, 50.f, 0.f), q0, q1) < 0.f);
    check("quarter Cylinder distance is positive outside phi", distance_leaf_cylinder(make_float3(-50.f, 50.f, 0.f), q0, q1) > 0.f);
}

void check_caps_and_wrapped_start()
{
    const float radius = 100.f;
    const float quarter = 0.5f * CUDART_PI_F;
    const quad  q0 = phi_primitive(0.f, quarter, radius);
    const quad  q1 = z_range(-60.f, 60.f);

    Result sr = sphere(q0, q1, make_float3(20.f, 20.f, 200.f), make_float3(0.f, 0.f, -1.f));
    Result cr = cylinder(q0, q1, make_float3(20.f, 20.f, 200.f), make_float3(0.f, 0.f, -1.f));
    check("quarter ZSphere accepts an in-wedge z cap", sr.valid && close(sr.hit.z, 60.f) && close(sr.isect.z, 1.f));
    check("quarter Cylinder accepts an in-wedge z cap", cr.valid && close(cr.hit.z, 60.f) && close(cr.isect.z, 1.f));

    sr = sphere(q0, q1, make_float3(-20.f, 20.f, 200.f), make_float3(0.f, 0.f, -1.f));
    cr = cylinder(q0, q1, make_float3(-20.f, 20.f, 200.f), make_float3(0.f, 0.f, -1.f));
    check("quarter ZSphere rejects an out-of-wedge z cap", !sr.valid);
    check("quarter Cylinder rejects an out-of-wedge z cap", !cr.valid);

    const quad wrapped = phi_primitive(1.75f * CUDART_PI_F, quarter, radius);
    sr = sphere(wrapped, q1, make_float3(200.f, 0.f, 0.f), make_float3(-1.f, 0.f, 0.f));
    cr = cylinder(wrapped, q1, make_float3(200.f, 0.f, 0.f), make_float3(-1.f, 0.f, 0.f));
    check("wrapped-start ZSphere includes phi zero", sr.valid && close(sr.hit.x, radius));
    check("wrapped-start Cylinder includes phi zero", cr.valid && close(cr.hit.x, radius));
}

void check_pacman_wedge()
{
    const float radius = 100.f;
    const float delta = 1.5f * CUDART_PI_F;
    const quad  q0 = phi_primitive(0.f, delta, radius);
    const quad  q1 = z_range(-radius, radius);

    Result sr = sphere(q0, q1, make_float3(200.f, -50.f, 0.f), make_float3(-1.f, 0.f, 0.f));
    Result cr = cylinder(q0, q1, make_float3(200.f, -50.f, 0.f), make_float3(-1.f, 0.f, 0.f));
    check("pacman ZSphere first hits the end-phi wall", sr.valid && close(sr.hit.x, 0.f) && close(sr.isect.x, 1.f));
    check("pacman Cylinder first hits the end-phi wall", cr.valid && close(cr.hit.x, 0.f) && close(cr.isect.x, 1.f));

    check("pacman ZSphere distance is negative in retained region", distance_leaf_zsphere(make_float3(-50.f, -50.f, 0.f), q0, q1) < 0.f);
    check("pacman ZSphere distance is positive in missing quadrant", distance_leaf_zsphere(make_float3(50.f, -50.f, 0.f), q0, q1) > 0.f);
    check("pacman Cylinder distance is negative in retained region", distance_leaf_cylinder(make_float3(-50.f, -50.f, 0.f), q0, q1) < 0.f);
    check("pacman Cylinder distance is positive in missing quadrant", distance_leaf_cylinder(make_float3(50.f, -50.f, 0.f), q0, q1) > 0.f);
}
} // namespace

int main()
{
    check_quarter_wedge();
    check_caps_and_wrapped_start();
    check_pacman_wedge();
    std::printf("// intersect_leaf_phi_wedge_test : %s (%d failure%s)\n", failures == 0 ? "PASS" : "FAIL", failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
