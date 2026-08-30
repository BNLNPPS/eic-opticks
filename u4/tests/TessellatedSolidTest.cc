/**
 * Verifies host-side conversion of a `G4TessellatedSolid` by `U4Solid`.
 *
 * This test constructs a closed, offset tetrahedron at runtime. It checks the
 * entity-type and tag dispatch, then verifies that conversion produces a box
 * placeholder with the expected dimensions, local bounds, and translation.
 *
 * This test does not exercise triangulated GPU intersection.
 */

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

#include "G4TessellatedSolid.hh"
#include "G4TriangularFacet.hh"

#include "U4Solid.h"

#include "s_csg.h"

namespace
{
bool close(double actual, double expected, double tolerance = 1.e-9)
{
    return std::fabs(actual - expected) <= tolerance * (1. + std::fabs(expected));
}

bool addFacet(G4TessellatedSolid& solid, const G4ThreeVector& a, const G4ThreeVector& b, const G4ThreeVector& c)
{
    return solid.AddFacet(new G4TriangularFacet(a, b, c, ABSOLUTE));
}
} // namespace

int main()
{
    const G4ThreeVector p0(10., 20., 30.);
    const G4ThreeVector p1(14., 20., 30.);
    const G4ThreeVector p2(10., 26., 30.);
    const G4ThreeVector p3(10., 20., 38.);

    G4TessellatedSolid solid("OffsetTetrahedron");

    bool facetsAdded =
        addFacet(solid, p0, p2, p1) &&
        addFacet(solid, p0, p1, p3) &&
        addFacet(solid, p0, p3, p2) &&
        addFacet(solid, p1, p2, p3);
    if (!facetsAdded)
    {
        std::cerr << "failed to construct tessellated test geometry" << std::endl;
        return EXIT_FAILURE;
    }
    solid.SetSolidClosed(true);

    if (U4Solid::Type(solid.GetEntityType().c_str()) != _G4TessellatedSolid ||
        std::string(U4Solid::Tag(_G4TessellatedSolid)) != "Tes")
    {
        std::cerr << "G4TessellatedSolid type or tag dispatch failed" << std::endl;
        return EXIT_FAILURE;
    }

    s_csg csg;
    sn*   root = U4Solid::Convert(&solid, 0, 0, 0);
    if (root == nullptr || root->typecode != CSG_BOX3)
    {
        std::cerr << "G4TessellatedSolid did not convert to a box placeholder" << std::endl;
        return EXIT_FAILURE;
    }

    const double*        param = root->getParam();
    const double*        aabb = root->getAABB();
    glm::tmat4x4<double> transform(1.);
    glm::tmat4x4<double> inverse(1.);
    root->getNodeTransformProduct(transform, inverse, false, nullptr, nullptr);

    bool dimensions =
        param != nullptr && close(param[0], 4.) && close(param[1], 6.) && close(param[2], 8.);
    bool localBounds =
        aabb != nullptr &&
        close(aabb[0], -2.) && close(aabb[1], -3.) && close(aabb[2], -4.) &&
        close(aabb[3], 2.) && close(aabb[4], 3.) && close(aabb[5], 4.);
    bool translation =
        close(transform[3][0], 12.) && close(transform[3][1], 23.) && close(transform[3][2], 34.);

    delete root;

    if (!dimensions || !localBounds || !translation)
    {
        std::cerr << "box placeholder does not preserve the tessellated-solid extent" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "G4TessellatedSolid converted to the expected translated box placeholder" << std::endl;
    return EXIT_SUCCESS;
}
