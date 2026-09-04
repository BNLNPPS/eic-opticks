/**
 * @file TessellatedSolidTest.cc
 * @brief Verifies host-side tessellated-solid conversion and detection.
 *
 * The test constructs a closed, offset tetrahedron at runtime. It verifies
 * entity-type and tag dispatch, then checks that conversion produces a box
 * placeholder with the expected dimensions, local bounds, and translation.
 *
 * The test places the tetrahedron with a non-identity rotation and verifies
 * that displaced-solid and multi-union transforms preserve it. Recursive
 * detection is checked when the tetrahedron is used directly or inside
 * Boolean, displaced, and multi-union solids, while confirming that
 * an ordinary box is not detected as tessellated.
 *
 * The test does not exercise triangulated GPU intersection.
 */

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

#include "G4Box.hh"
#include "G4DisplacedSolid.hh"
#include "G4MultiUnion.hh"
#include "G4RotationMatrix.hh"
#include "G4TessellatedSolid.hh"
#include "G4Transform3D.hh"
#include "G4TriangularFacet.hh"
#include "G4UnionSolid.hh"

#include "U4Solid.h"

#include "s_csg.h"

namespace
{
bool close(double actual, double expected, double tolerance = 1.e-9)
{
    return std::fabs(actual - expected) <= tolerance * (1. + std::fabs(expected));
}

bool close(const glm::tmat4x4<double>& actual, const glm::tmat4x4<double>& expected)
{
    for (int column = 0; column < 4; ++column)
    {
        for (int row = 0; row < 4; ++row)
        {
            if (!close(actual[column][row], expected[column][row]))
                return false;
        }
    }
    return true;
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

    G4Box            box("Box", 1., 1., 1.);
    G4UnionSolid     boolean("BooleanWithTessellated", &box, &solid);
    G4DisplacedSolid displaced("DisplacedTessellated", &solid, G4Transform3D());
    G4MultiUnion     multiUnion("MultiUnionWithTessellated");
    multiUnion.AddNode(box, G4Transform3D());
    multiUnion.AddNode(solid, G4Transform3D());

    bool containment =
        U4Solid::ContainsTessellated(&solid) &&
        U4Solid::ContainsTessellated(&boolean) &&
        U4Solid::ContainsTessellated(&displaced) &&
        U4Solid::ContainsTessellated(&multiUnion) &&
        !U4Solid::ContainsTessellated(&box);

    if (!containment)
    {
        std::cerr << "recursive tessellated-solid detection failed" << std::endl;
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

    G4RotationMatrix rotation;
    rotation.rotateZ(0.5 * std::acos(-1.));
    G4Transform3D placement(rotation, G4ThreeVector(100., 200., 300.));

    G4DisplacedSolid rotatedDisplaced("RotatedDisplacedTessellated", &solid, placement);
    G4MultiUnion     rotatedMultiUnion("RotatedMultiUnionTessellated");
    rotatedMultiUnion.AddNode(solid, placement);

    sn* displacedRoot = U4Solid::Convert(&rotatedDisplaced, 1, 0, 0);
    sn* multiUnionRoot = U4Solid::Convert(&rotatedMultiUnion, 2, 0, 0);

    if (displacedRoot == nullptr || multiUnionRoot == nullptr || multiUnionRoot->num_child() != 1)
    {
        std::cerr << "failed to convert rotated tessellated test geometry" << std::endl;
        delete displacedRoot;
        delete multiUnionRoot;
        return EXIT_FAILURE;
    }

    glm::tmat4x4<double> displacedPlacement(1.);
    glm::tmat4x4<double> multiUnionPlacement(1.);
    U4Transform::GetDispTransform(displacedPlacement, &rotatedDisplaced);
    U4Transform::GetMultiUnionItemTransform(multiUnionPlacement, &rotatedMultiUnion, 0);

    glm::tmat4x4<double> displacedTransform(1.);
    glm::tmat4x4<double> displacedInverse(1.);
    displacedRoot->getNodeTransformProduct(
        displacedTransform, displacedInverse, false, nullptr, nullptr);

    glm::tmat4x4<double> multiUnionTransform(1.);
    glm::tmat4x4<double> multiUnionInverse(1.);
    multiUnionRoot->get_child(0)->getNodeTransformProduct(
        multiUnionTransform, multiUnionInverse, false, nullptr, nullptr);

    glm::tmat4x4<double> expectedDisplaced = displacedPlacement * transform;
    glm::tmat4x4<double> expectedMultiUnion = multiUnionPlacement * transform;
    bool                 displacedPlacementPreserved =
        close(displacedTransform, expectedDisplaced) &&
        close(displacedInverse, glm::inverse(expectedDisplaced));
    bool multiUnionPlacementPreserved =
        close(multiUnionTransform, expectedMultiUnion) &&
        close(multiUnionInverse, glm::inverse(expectedMultiUnion));

    delete displacedRoot;
    delete multiUnionRoot;

    if (!displacedPlacementPreserved || !multiUnionPlacementPreserved)
    {
        std::cerr << "rotated enclosing placement was composed in the wrong order" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "G4TessellatedSolid conversion preserved extent and rotated placements" << std::endl;
    return EXIT_SUCCESS;
}
