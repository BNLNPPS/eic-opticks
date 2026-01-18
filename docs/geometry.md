# Geometry

## Loading Geometry into EIC-Opticks

EIC-Opticks can import geometries in GDML format automatically. There are about
10 primitives supported now (e.g., G4Box). G4Trd and G4Trap are not yet supported.

The `src/simg4oxmt` executable takes GDML files through arguments:

```shell
src/simg4oxmt -g mygdml.gdml
```

## GDML Requirements

The GDML must define all optical properties of surfaces and materials including:

* Efficiency (used by EIC-Opticks to specify detection efficiency and assign sensitive surfaces)
* Refractive index
* Group velocity
* Reflectivity

## Optical Surface Models in Geant4

In Geant4, optical surface properties such as **finish**, **model**, and **type**
are defined using enums in the `G4OpticalSurface` and `G4SurfaceProperty` header files:

* [G4OpticalSurface.hh](https://github.com/Geant4/geant4/blob/geant4-11.3-release/source/materials/include/G4OpticalSurface.hh#L52-L113)
* [G4SurfaceProperty.hh](https://github.com/Geant4/geant4/blob/geant4-11.3-release/source/materials/include/G4SurfaceProperty.hh#L58-L68)

These enums allow users to configure how optical photons interact with surfaces,
controlling behaviors like reflection, transmission, and absorption based on
physical models and surface qualities.

The string values corresponding to these enums (e.g., `"ground"`, `"glisur"`,
`"dielectric_dielectric"`) can also be used directly in **GDML** files when
defining `<opticalsurface>` elements.

### Example GDML

```xml
<gdml>
  ...
  <solids>
    <opticalsurface finish="ground" model="glisur" name="medium_surface" type="dielectric_dielectric" value="1">
      <property name="EFFICIENCY" ref="EFFICIENCY_DEF"/>
      <property name="REFLECTIVITY" ref="REFLECTIVITY_DEF"/>
    </opticalsurface>
  </solids>
  ...
</gdml>
```

For a physics-motivated explanation of how Geant4 handles optical photon boundary
interactions, refer to the [Geant4 Application Developer Guide - Boundary Process](https://geant4-userdoc.web.cern.ch/UsersGuides/ForApplicationDeveloper/html/TrackingAndPhysics/physicsProcess.html#boundary-process).
