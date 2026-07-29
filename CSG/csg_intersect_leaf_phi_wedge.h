#pragma once

/**
 * @file csg_intersect_leaf_phi_wedge.h
 *
 * Shared angular clipping helpers for centred CSG leaf primitives.
 *
 * Partial-phi spheres and cylinders store their angular interval directly in the
 * leaf parameter quad:
 *
 *     q0.f.x = startPhi
 *     q0.f.y = deltaPhi
 *
 * Angles are in radians and describe a counter-clockwise sweep about the positive
 * z axis, starting at `startPhi`. The interval includes both radial boundary
 * half-planes and the z axis. A wedge is active only for
 * `0 < deltaPhi < 2*pi`; values outside that range preserve the legacy unclipped
 * primitive.
 *
 * These `LEAF_FUNC` helpers are shared by host and device intersection and
 * signed-distance implementations. They provide angular classification for
 * curved surfaces and caps, a signed field for combining the angular constraint
 * with the leaf distance, and intersections with the two radial walls. Wall
 * intersections are clipped only to the outward radial half-plane here; callers
 * must additionally apply the primitive's radial, axial, and `t_min` constraints.
 */

/**
 * Reports whether an angular interval clips a full primitive.
 *
 * @param deltaPhi Counter-clockwise angular sweep in radians.
 * @return `true` only for a positive sweep strictly smaller than a full circle.
 *         Non-positive and at-least-full-circle sweeps are treated as unclipped.
 */
LEAF_FUNC
bool csg_has_phi_wedge(const float deltaPhi)
{
    return deltaPhi > 0.f && deltaPhi < 2.f * CUDART_PI_F;
}

/**
 * Computes the counter-clockwise azimuthal displacement from `startPhi`.
 *
 * The result is normalized to the half-open interval `[0, 2*pi)`, which also
 * makes intervals whose end angle crosses phi zero straightforward to test.
 * Callers that need special handling at the z axis should do so before invoking
 * this helper because azimuth is undefined there.
 *
 * @param x Point x coordinate relative to the primitive centre.
 * @param y Point y coordinate relative to the primitive centre.
 * @param startPhi Angular origin of the wedge in radians.
 * @return Normalized counter-clockwise displacement from `startPhi`, in radians.
 */
LEAF_FUNC
float csg_phi_delta(const float x, const float y, const float startPhi)
{
    const float twoPi = 2.f * CUDART_PI_F;
    float       dphi = atan2f(y, x) - startPhi;
    while (dphi < 0.f) dphi += twoPi;
    while (dphi >= twoPi) dphi -= twoPi;
    return dphi;
}

/**
 * Tests whether an xy point lies within an inclusive phi interval.
 *
 * Inactive wedges accept every point. Points sufficiently close to the z axis
 * are also accepted because they belong to both radial boundary half-planes.
 * A small angular tolerance includes points lying numerically on the end wall.
 *
 * @param x Point x coordinate relative to the primitive centre.
 * @param y Point y coordinate relative to the primitive centre.
 * @param startPhi Start angle of the counter-clockwise interval, in radians.
 * @param deltaPhi Angular sweep of the interval, in radians.
 * @return `true` when the point is inside or on the wedge, or when no wedge is
 *         active.
 */
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

/**
 * Evaluates a signed field for the angular wedge constraint.
 *
 * Negative values classify points inside the wedge, positive values classify
 * points outside, and zero identifies either radial wall. For sweeps no larger
 * than pi the wedge is the intersection of the two inward half-spaces, so their
 * signed plane fields are combined with `max`. For larger "pacman" sweeps the
 * wedge is their union and `min` is used instead.
 *
 * This helper assumes an active wedge (`0 < deltaPhi < 2*pi`). It is intended as
 * a classification-compatible field to combine with another leaf distance, not
 * as a general Euclidean distance to every feature of the finite primitive.
 *
 * @param x Point x coordinate relative to the primitive centre.
 * @param y Point y coordinate relative to the primitive centre.
 * @param startPhi Start angle of the counter-clockwise interval, in radians.
 * @param deltaPhi Angular sweep of the interval, in radians.
 * @return Signed angular-wedge field: negative inside, positive outside.
 */
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

    return deltaPhi <= CUDART_PI_F ? fmaxf(sdStart, sdEnd) : fminf(sdStart, sdEnd);
}

/**
 * Intersects a ray with one radial boundary half-plane of a phi wedge.
 *
 * The boundary is the half of the vertical plane extending from the z axis in
 * the direction `boundaryPhi`. Intersections on the opposite radial half-plane
 * are rejected. On success, `(nx, ny)` is the outward unit normal for either the
 * start or end wall of a counter-clockwise wedge.
 *
 * This helper does not reject intersections behind the ray origin and does not
 * clip against a primitive radius or z extent. The caller must validate `t`
 * against `t_min` and its own finite surface bounds. Output values are meaningful
 * only when the function returns `true`.
 *
 * @param[out] t Ray parameter at the wall intersection.
 * @param[out] nx Outward wall-normal x component.
 * @param[out] ny Outward wall-normal y component.
 * @param boundaryPhi Azimuth of the radial boundary half-plane, in radians.
 * @param startWall `true` for the interval's start wall, `false` for its end wall.
 * @param ox Ray-origin x coordinate relative to the primitive centre.
 * @param oy Ray-origin y coordinate relative to the primitive centre.
 * @param vx Ray-direction x component.
 * @param vy Ray-direction y component.
 * @return `true` when the ray is not parallel to the boundary plane and its
 *         intersection lies on the outward radial half-plane.
 */
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
