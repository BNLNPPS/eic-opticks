#include <cstdio>
#include <cstdlib>

#include "scerenkov.h"
#include "scuda.h"
#include "smath.h"
#include "srec.h"
#include "srngcpu.h"
using RNG = srngcpu;

#include "stexture.h"

#include "OpticksPhoton.h"
#include "qsim.h"

namespace
{
void require(bool condition, const char* expression, int line)
{
    if (condition)
        return;
    std::fprintf(stderr, "%s:%d requirement failed: %s\n", __FILE__, line, expression);
    std::exit(EXIT_FAILURE);
}

#define REQUIRE(expression) require((expression), #expression, __LINE__)

constexpr unsigned BOUNDARY_INDEX = 2u;
constexpr unsigned OUTER_MATERIAL_LINE =
    BOUNDARY_INDEX * _BOUNDARY_NUM_MATSUR + OMAT;
constexpr unsigned INNER_MATERIAL_LINE =
    BOUNDARY_INDEX * _BOUNDARY_NUM_MATSUR + IMAT;

void initialize(sctx& ctx, quad2& prd)
{
    prd.zero();
    prd.q0.f.z = 1.f;
    prd.q1.u.w = BOUNDARY_INDEX;

    ctx = {};
    ctx.prd = &prd;
    ctx.pidx = 0u;
    ctx.p.zero();
    ctx.p.mom = make_float3(0.f, 0.f, -1.f);
    ctx.p.pol = make_float3(1.f, 0.f, 0.f);
    ctx.p.wavelength = 420.f;

    ctx.s.material1 = make_float4(1.f, 0.f, 0.f, 0.f);
    ctx.s.material2 = make_float4(1.f, 0.f, 0.f, 0.f);
    ctx.s.m1group2 = make_float4(100.f, 200.f, 0.f, 0.f);
    ctx.s.index = make_uint4(10u, 20u, 0u, 0u);
}

void test_transmission_updates_carried_material(const qsim& sim, RNG& rng)
{
    sctx  ctx = {};
    quad2 prd = {};
    initialize(ctx, prd);
    ctx.current_matline = 99u;

    unsigned         flag = 0u;
    const FlowAction action = sim.propagate_at_boundary(flag, rng, ctx, 1.f);

    REQUIRE(action == FlowAction::Continue);
    REQUIRE(flag == BOUNDARY_TRANSMIT);
    REQUIRE(ctx.current_matline == INNER_MATERIAL_LINE);
    REQUIRE(ctx.current_material_index == 20u);
    REQUIRE(ctx.current_group_velocity == 200.f);
}

void test_reflection_preserves_carried_material(const qsim& sim, RNG& rng)
{
    sctx  ctx = {};
    quad2 prd = {};
    initialize(ctx, prd);
    ctx.current_matline = 99u;

    unsigned         flag = 0u;
    const FlowAction action = sim.propagate_at_boundary(flag, rng, ctx, 0.f);

    REQUIRE(action == FlowAction::Continue);
    REQUIRE(flag == BOUNDARY_REFLECT);
    REQUIRE(ctx.current_matline == 99u);
    REQUIRE(ctx.current_material_index == 10u);
    REQUIRE(ctx.current_group_velocity == 100.f);
}

void test_outward_transmission_selects_outer_material(const qsim& sim, RNG& rng)
{
    sctx  ctx = {};
    quad2 prd = {};
    initialize(ctx, prd);
    ctx.p.mom = make_float3(0.f, 0.f, 1.f);

    unsigned         flag = 0u;
    const FlowAction action = sim.propagate_at_boundary(flag, rng, ctx, 1.f);

    REQUIRE(action == FlowAction::Continue);
    REQUIRE(flag == BOUNDARY_TRANSMIT);
    REQUIRE(ctx.current_matline == OUTER_MATERIAL_LINE);
}
} // namespace

int main()
{
    qbase base = {};
    base.pidx = ~0ull;

    qsim sim;
    sim.base = &base;

    RNG rng;
    rng.set_fake(0.5);

    test_transmission_updates_carried_material(sim, rng);
    test_reflection_preserves_carried_material(sim, rng);
    test_outward_transmission_selects_outer_material(sim, rng);

    std::puts("QSim_MaterialCarryTest: PASS");
    return 0;
}
