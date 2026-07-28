#include <cassert>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

#include <plog/Appenders/ConsoleAppender.h>
#include <plog/Formatters/TxtFormatter.h>
#include <plog/Init.h>
#include <plog/Log.h>

using plog::error;
using plog::fatal;
using plog::info;

#include "G4LogicalVolume.hh"
#include "G4PhysicalConstants.hh"
#include "G4SystemOfUnits.hh"
#include "G4Tubs.hh"
#include "G4VPhysicalVolume.hh"

#include "config_path.h"

#include "U4GDML.h"
#include "U4Solid.h"
#include "U4Volume.h"

#include "s_csg.h"

namespace
{
inline constexpr char testGeomFile[] = SIMPHONY_TEST_GEOM_DIR "/tube_phicut_quarter_shell.gdml";

bool close(double actual, double expected, double tolerance = 1.e-9)
{
    return std::fabs(actual - expected) <= tolerance * (1. + std::fabs(expected));
}
} // namespace

int main()
{
    static plog::ConsoleAppender<plog::TxtFormatter> consoleAppender;
    plog::init(plog::info, &consoleAppender);

    const G4VPhysicalVolume* world = U4GDML::Read(testGeomFile);
    if (world == nullptr)
    {
        std::cerr << "failed to load " << testGeomFile << std::endl;
        return EXIT_FAILURE;
    }

    const G4VPhysicalVolume* tubePV = U4Volume::FindPV(world, "QuarterTube_pv");
    const G4LogicalVolume*   tubeLV = tubePV ? tubePV->GetLogicalVolume() : nullptr;
    const G4Tubs*            tube = tubeLV ? dynamic_cast<const G4Tubs*>(tubeLV->GetSolid()) : nullptr;
    if (tube == nullptr)
    {
        std::cerr << "failed to find partial-phi G4Tubs QuarterTube_pv" << std::endl;
        return EXIT_FAILURE;
    }

    const double startPhi = tube->GetStartPhiAngle() / CLHEP::radian;
    const double deltaPhi = tube->GetDeltaPhiAngle() / CLHEP::radian;
    if (!close(startPhi, 0.) || !close(deltaPhi, 0.5 * CLHEP::pi))
    {
        std::cerr << "GDML tube did not preserve its quarter-phi interval" << std::endl;
        return EXIT_FAILURE;
    }

    s_csg* csg = new s_csg;
    assert(csg);

    const int lvid = 0;
    sn*       root = U4Solid::Convert(tube, lvid, 0, 1);
    if (root == nullptr || root->typecode != CSG_INTERSECTION)
    {
        std::cerr << "partial annular G4Tubs did not convert to a canonical intersection" << std::endl;
        return EXIT_FAILURE;
    }

    std::vector<const sn*> primitives;
    root->collect_prim(primitives);
    int  matching = 0;
    bool outer = false;
    bool inner = false;
    int  complemented = 0;

    for (const sn* primitive : primitives)
    {
        if (primitive->typecode != CSG_CYLINDER)
            continue;

        const double* param = primitive->getParam();
        const double* aabb = primitive->getAABB();
        if (param == nullptr || aabb == nullptr)
            continue;

        bool phi = close(param[0], startPhi) && close(param[1], deltaPhi);
        bool bounds = close(aabb[0], -param[3]) && close(aabb[1], -param[3]) && close(aabb[3], param[3]) && close(aabb[4], param[3]);
        if (phi && bounds)
        {
            matching++;
            outer = outer || close(param[3], 100.);
            inner = inner || close(param[3], 50.);
            complemented += primitive->complement == 1 ? 1 : 0;
        }
    }

    delete root;

    if (matching != 2 || !outer || !inner || complemented != 1)
    {
        std::cerr << "converted tube did not retain phi metadata, bounds, and annular complement" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "partial-phi G4Tubs converted to two phi-aware Cylinder leaves" << std::endl;
    return EXIT_SUCCESS;
}
