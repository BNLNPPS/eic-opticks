#include <cassert>
#include <cstring>

#include "OpticksGenstep.h"

void test_retired_codes()
{
    for (unsigned type = 1; type <= 5; ++type)
    {
        assert(std::strcmp(OpticksGenstep_::Name(type), OpticksGenstep_::INVALID_) == 0);
        assert(!OpticksGenstep_::IsValid(type));
        assert(!OpticksGenstep_::IsCerenkov(type));
        assert(!OpticksGenstep_::IsScintillation(type));
        assert(!OpticksGenstep_::IsExpected(type));
        assert(OpticksGenstep_::GenstepToPhotonFlag(type) == NAN_ABORT);
    }
}

void test_current_optical_codes()
{
    static_assert(OpticksGenstep_TORCH == 6, "genstep protocol values must remain stable");
    static_assert(OpticksGenstep_CERENKOV == 15, "genstep protocol values must remain stable");
    static_assert(OpticksGenstep_SCINTILLATION == 16, "genstep protocol values must remain stable");

    assert(OpticksGenstep_::Type("CERENKOV") == OpticksGenstep_CERENKOV);
    assert(OpticksGenstep_::Type("SCINTILLATION") == OpticksGenstep_SCINTILLATION);
    assert(OpticksGenstep_::IsCerenkov(OpticksGenstep_CERENKOV));
    assert(OpticksGenstep_::IsCerenkov(OpticksGenstep_G4Cerenkov_modified));
    assert(OpticksGenstep_::IsScintillation(OpticksGenstep_SCINTILLATION));
    assert(OpticksGenstep_::GenstepToPhotonFlag(OpticksGenstep_CERENKOV) == CERENKOV);
    assert(OpticksGenstep_::GenstepToPhotonFlag(OpticksGenstep_SCINTILLATION) == SCINTILLATION);
}

int main()
{
    test_retired_codes();
    test_current_optical_codes();
    return 0;
}
