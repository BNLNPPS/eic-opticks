#pragma once
#define PNUMQUAD 4  // quads per photon  

struct Photon
{
   float3 position ;  
   float  time ;  

   float3 direction ;
   float  wavelength ; 

   float3 polarization ;
   float  weight ; 

   quad flags ; 

};

__device__ void psave( Photon& p, optix::buffer<float4>& pbuffer, unsigned int photon_offset )
{
    pbuffer[photon_offset+0] = make_float4( p.position.x,    p.position.y,    p.position.z,     p.time ); 
    pbuffer[photon_offset+1] = make_float4( p.direction.x,   p.direction.y,   p.direction.z,    p.wavelength );
    pbuffer[photon_offset+2] = make_float4( p.polarization.x,p.polarization.y,p.polarization.z, p.weight );
    pbuffer[photon_offset+3] = make_float4( p.flags.f.x,     p.flags.f.y,     p.flags.f.z,      p.flags.f.w); 
}





