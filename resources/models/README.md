# Replacement F-16 airframe (`F-16DES.obj`)

Higher-detail airframe for the F-16 (`F-16DES.IFF`). It is picked up automatically by the
`neosc_enhanced` executable (`EnhancedEntity`): the file has the base name of the original
entity and lives in `assets/models/` (copied there from `resources/models/` like the other
resources). No configuration entry is needed; the original `neosc` ignores it. Flight model,
hard points and gear stay original; only the visible geometry is replaced. An optional
`# lift <m>` header line in the OBJ moves the afterburner child object along with the new model.

## Source and licence

This model is a **modified version** of the 3D model of the
FlightGear *General Dynamics F-16 Fighting Falcon*
(<https://github.com/NikolaiVChr/f16>, `Models/f16.ac` and `Models/nozzle.ac`),
Copyright (C) 2023 Erik Hofman and contributors (see `authors.txt` in the
upstream repository).

It is distributed under the **GNU General Public License, version 2 or (at
your option) any later version**. The full text of GPL-2.0 is in `GPL-2.0.txt`
next to this file. The rest of this repository is not affected by this licence
(the model is a separate data file).

## Modifications

* Selected airframe parts only (fuselage, intake, wings, flaps, tail, canopy
  frame, nozzle); landing gear, doors, air brakes, ventral fins, stores, pylons,
  lights, logos, engine fans and flames were left out.
* Merged into one closed mesh (voxel remeshing at 2.5 cm, light smoothing),
  decimated to four levels of detail (about 16000 / 8600 / 1800 / 600 triangles).
* Axes and scale fitted to the original Strike Commander F-16
  (x forward, y up, z lateral); belly closed; vertices split at 45 degree creases.
* Canopy glass (`CanopyForwardOutside`, `CanopyBackOutside`) kept as a separate
  surface (`usemtl glass`), pilot (`pilot_externalview.ac`) and ejection seat
  (`Cockpit/chair/chair.ac`) decimated into fixed-colour groups (`usemtl rgb_RRGGBB`).
* Control surfaces: the rudder (down to the fuselage), the flaperons and a thin
  hinge line are cut into separate, differently coloured faces (static, not animated).
* Cockpit opening: the dark cockpit tub is cut along a smooth outline.
* Livery: desert camouflage (sand, brown, khaki, grey-green on top and sides, grey
  underside), written as fixed `usemtl rgb_RRGGBB` groups; colours are snapped to the
  game palette at load time. The radome has no colour tag on purpose: untagged
  faces take their colour from the original model and are used by the loader to detect
  the triangle winding of the file.
