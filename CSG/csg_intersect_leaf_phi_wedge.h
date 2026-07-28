#pragma once

/**
csg_intersect_leaf_phi_wedge
--------------------------------

Shared helpers for phi intervals baked into centred leaf primitives::

    q0.f.x = startPhi
    q0.f.y = deltaPhi

Angles are in radians. A zero delta preserves the legacy full primitive.
**/

LEAF_FUNC
bool csg_has_phi_wedge(const float deltaPhi)
{
    return deltaPhi > 0.f && deltaPhi < 2.f * CUDART_PI_F;
}

LEAF_FUNC
float csg_phi_delta(const float x, const float y, const float startPhi)
{
    const float twoPi = 2.f * CUDART_PI_F;
    float       dphi = atan2f(y, x) - startPhi;
    while (dphi < 0.f) dphi += twoPi;
    while (dphi >= twoPi) dphi -= twoPi;
    return dphi;
}

LEAF_FUNC
bool csg_in_phi_wedge(const float x, const float y, const float startPhi, const float deltaPhi)
{
    if (!csg_has_phi_wedge(deltaPhi))
        return true;

    const float radial2 = x * x + y * y;
    if (radial2 < 1.e-12f)
        return true;

    return csg_phi_delta(x, y, startPhi) <= deltaPhi + 1.e-6f;
}

LEAF_FUNC
float csg_distance_phi_wedge(const float x, const float y, const float startPhi, const float deltaPhi)
{
    const float endPhi = startPhi + deltaPhi;
    const float sinStart = sinf(startPhi);
    const float cosStart = cosf(startPhi);
    const float sinEnd = sinf(endPhi);
    const float cosEnd = cosf(endPhi);

    const float sdStart = x * sinStart - y * cosStart;
    const float sdEnd = -x * sinEnd + y * cosEnd;

    return deltaPhi <= CUDART_PI_F
               ? fmaxf(sdStart, sdEnd)
               : fminf(sdStart, sdEnd);
}

LEAF_FUNC
bool csg_intersect_phi_wall(float& t, float& nx, float& ny, const float boundaryPhi, const bool startWall, const float ox, const float oy, const float vx, const float vy)
{
    const float sinPhi = sinf(boundaryPhi);
    const float cosPhi = cosf(boundaryPhi);
    const float denom = vx * sinPhi - vy * cosPhi;
    if (fabsf(denom) <= 1.e-12f)
        return false;

    t = -(ox * sinPhi - oy * cosPhi) / denom;
    const float x = ox + t * vx;
    const float y = oy + t * vy;
    if (x * cosPhi + y * sinPhi < -1.e-6f)
        return false;

    nx = startWall ? sinPhi : -sinPhi;
    ny = startWall ? -cosPhi : cosPhi;
    return true;
}
