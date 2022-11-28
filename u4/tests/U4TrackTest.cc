#include "U4Track.h"

int main(int argc, char** argv)
{
    G4Track* track = U4Track::MakePhoton(); 
    spho p0 = {1, 2, 3, {0,0,0,0}} ; 

    STrackInfo<spho>::Set(track, p0 ); 

    const G4Track* ctrack = track ; 
    std::cout << U4Track::Desc<spho>(ctrack) << std::endl ; 


    spho* p2 = STrackInfo<spho>::GetRef(ctrack); 
    assert( p2->isIdentical(p0) ); 
    std::cout << U4Track::Desc<spho>(ctrack) << std::endl ; 


    p2->uc4.w = 'Z' ; 
    std::cout << U4Track::Desc<spho>(ctrack) << std::endl ; 


    return 0 ; 
}
