# Replacement F-16 airframe (`f16_rs.obj`)

Optional higher-detail airframe for the player F-16, loaded at start-up by the
`[Models] f16_obj` option of `config.ini` (path relative to the game's working
directory). Flight model, hard points and gear stay original; only the visible
geometry is replaced. An optional `# lift <m>` header line in the OBJ moves the
afterburner child object along with the new model.

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
  decimated to four levels of detail (15000 / 5000 / 1500 / 500 triangles).
* Axes and scale fitted to the original Strike Commander F-16
  (x forward, y up, z lateral); belly closed; vertices split at 45 degree creases.
* Canopy glass (`CanopyForwardOutside`, `CanopyBackOutside`) kept as a separate
  surface (`usemtl glass`), pilot (`pilot_externalview.ac`) and ejection seat
  (`Cockpit/chair/chair.ac`) decimated into fixed-colour groups (`usemtl rgb_RRGGBB`).
* Per-triangle colours are taken from the original game model at load time.
