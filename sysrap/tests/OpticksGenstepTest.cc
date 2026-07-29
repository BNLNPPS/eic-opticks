#include <cstdio>

#include "OpticksGenstep.h"

static int fail = 0;

static void check(const char* label, bool expected, bool actual)
{
    const bool pass = expected == actual;
    std::printf("  %-44s %s\n", label, pass ? "PASS" : "*** FAIL ***");
    if (!pass)
        fail++;
}

int main()
{
    check("Cerenkov carries a material line", true, OpticksGenstep_UsesMaterialLine(OpticksGenstep_CERENKOV));
    check("scintillation carries a material line", true, OpticksGenstep_UsesMaterialLine(OpticksGenstep_SCINTILLATION));
    check("modified G4 Cerenkov carries a material line", true, OpticksGenstep_UsesMaterialLine(OpticksGenstep_G4Cerenkov_modified));
    check("TORCH does not carry a material line", false, OpticksGenstep_UsesMaterialLine(OpticksGenstep_TORCH));
    check("FRAME does not carry a material line", false, OpticksGenstep_UsesMaterialLine(OpticksGenstep_FRAME));
    check("input photons do not carry a material line", false, OpticksGenstep_UsesMaterialLine(OpticksGenstep_INPUT_PHOTON));
    check("carrier gensteps do not carry a material line", false, OpticksGenstep_UsesMaterialLine(OpticksGenstep_CARRIER));

    std::printf("OpticksGenstepTest: %s (%d failure%s)\n",
                fail == 0 ? "PASS" : "FAIL", fail, fail == 1 ? "" : "s");
    return fail == 0 ? 0 : 1;
}
