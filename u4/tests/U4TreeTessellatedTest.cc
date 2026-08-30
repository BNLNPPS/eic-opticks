/**
 * Verifies U4Tree handling of repeated, nested tessellated solids.
 *
 * The test builds a Boolean solid containing a tessellated tetrahedron at
 * runtime and places its logical volume twice in a simple world. It checks
 * that U4Tree selects the enclosing Boolean solid for triangulation, keeps
 * both placements out of analytic repeat factors, and records both as global
 * triangulated nodes. This is a CPU-side geometry-conversion test; it does not
 * run GPU intersection.
 */

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <vector>

#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4TessellatedSolid.hh"
#include "G4TriangularFacet.hh"
#include "G4UnionSolid.hh"

#include "OPTICKS_LOG.hh"
#include "U4Material.hh"
#include "U4Tree.h"

#include "NPFold.h"
#include "stree.h"

namespace
{
bool addFacet(G4TessellatedSolid& solid, const G4ThreeVector& a, const G4ThreeVector& b, const G4ThreeVector& c)
{
    return solid.AddFacet(new G4TriangularFacet(a, b, c, ABSOLUTE));
}

G4TessellatedSolid* makeTetrahedron()
{
    G4ThreeVector p0(0., 0., 0.);
    G4ThreeVector p1(4., 0., 0.);
    G4ThreeVector p2(0., 4., 0.);
    G4ThreeVector p3(0., 0., 4.);

    G4TessellatedSolid* solid = new G4TessellatedSolid("NestedTetrahedron");
    bool facetsAdded =
        addFacet(*solid, p0, p2, p1) &&
        addFacet(*solid, p0, p1, p3) &&
        addFacet(*solid, p0, p3, p2) &&
        addFacet(*solid, p1, p2, p3);

    if (!facetsAdded) return nullptr;
    solid->SetSolidClosed(true);
    return solid;
}

int findLvid(const stree& st, const char* name)
{
    std::vector<std::string>::const_iterator it = std::find(st.soname.begin(), st.soname.end(), name);
    return it == st.soname.end() ? -1 : int(std::distance(st.soname.begin(), it));
}
} // namespace

int main(int argc, char** argv)
{
    OPTICKS_LOG(argc, argv);

    G4TessellatedSolid* tessellated = makeTetrahedron();
    if (tessellated == nullptr)
    {
        std::cerr << "failed to construct tessellated test geometry" << std::endl;
        return EXIT_FAILURE;
    }

    G4Material* material = U4Material::Get(U4Material::VACUUM);
    if (material == nullptr)
    {
        std::cerr << "failed to construct vacuum material" << std::endl;
        return EXIT_FAILURE;
    }

    G4Box* component = new G4Box("BooleanBox", 3., 3., 3.);
    G4UnionSolid* nested = new G4UnionSolid("NestedTessellatedUnion", component, tessellated);
    G4LogicalVolume* nestedLV = new G4LogicalVolume(nested, material, "NestedTessellatedLV");

    G4Box* worldSolid = new G4Box("WorldBox", 100., 100., 100.);
    G4LogicalVolume* worldLV = new G4LogicalVolume(worldSolid, material, "WorldLV");

    new G4PVPlacement(nullptr, G4ThreeVector(-20., 0., 0.), nestedLV, "NestedPV0", worldLV, false, 0);
    new G4PVPlacement(nullptr, G4ThreeVector( 20., 0., 0.), nestedLV, "NestedPV1", worldLV, false, 1);
    G4VPhysicalVolume* world = new G4PVPlacement(
        nullptr, G4ThreeVector(), worldLV, "WorldPV", nullptr, false, 0);

    stree st;
    st.FREQ_CUT = 2;
    U4Tree* tree = U4Tree::Create(&st, world);
    if (tree == nullptr)
    {
        std::cerr << "U4Tree creation failed" << std::endl;
        return EXIT_FAILURE;
    }

    int lvid = findLvid(st, "NestedTessellatedUnion");
    std::vector<int> nodes;
    if (lvid > -1) st.find_lvid_nodes(nodes, lvid, 'N');

    bool nodesAreGlobal = nodes.size() == 2;
    for (unsigned i = 0; i < nodes.size(); ++i)
        nodesAreGlobal = nodesAreGlobal && st.nds[nodes[i]].repeat_index == 0;

    int triangulatedPlacements = 0;
    for (unsigned i = 0; i < st.tri.size(); ++i)
        if (st.tri[i].lvid == lvid) triangulatedPlacements += 1;

    const NPFold* mesh = lvid > -1 ? st.mesh->get_subfold(st.soname[lvid].c_str()) : nullptr;
    bool passed =
        lvid > -1 &&
        st.is_force_triangulate(lvid) &&
        st.get_num_factor() == 0 &&
        nodesAreGlobal &&
        triangulatedPlacements == 2 &&
        mesh != nullptr &&
        mesh->get_meta<int>("lvid", -1) == lvid;

    if (!passed)
    {
        std::cerr
            << "nested tessellated solid was not retained as global triangulated geometry"
            << " lvid " << lvid
            << " factors " << st.get_num_factor()
            << " nodes " << nodes.size()
            << " triangulated placements " << triangulatedPlacements
            << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "repeated nested tessellated solid remained global and triangulated" << std::endl;
    return EXIT_SUCCESS;
}
