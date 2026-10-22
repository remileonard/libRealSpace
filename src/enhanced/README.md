# StrikeCommanderEnhanced

Optional extensions of Strike Commander that live outside the original classes. Everything here derives from the
original code (`EnhancedEntity : RSEntity`, later `EnhancedStrike : SCStrike`, ...) and is only active in the
`neosc_enhanced` executable. The original `neosc` is not affected.

## Replacement models

For an aircraft loaded from `F-16DES.IFF`, `EnhancedEntity` looks for `assets/models/F-16DES.obj`. If the file
exists it replaces the airframe geometry (flight model, hard points and child objects stay from the IFF). No
configuration entry is needed; any model can be replaced by dropping a file with the base name of the original.

OBJ conventions: groups `lod0`..`lodN`, game axes (x forward, y up, z lateral); `usemtl glass` marks the canopy,
`usemtl rgb_RRGGBB` a fixed palette colour.

## Creation chain

There is no factory. `main.cpp` creates `new EnhancedMainMenu()`; each class creates the next one with a visible `new`
(`EnhancedGameFlow::flyMission` -> `new EnhancedStrike()`, `EnhancedStrike::createMission` -> `new EnhancedMission()`,
`EnhancedMission::createEntity` -> `new EnhancedEntity()`).

## Effects (`EnhancedStrike`)

`SCStrike` only provides an empty virtual hook, `afterPlaneRendered(SCPlane *, Vector3D position, int thrust)`, called after each aircraft has been
drawn. `EnhancedStrike` overrides it and adds, per aircraft:

- `ExhaustEffect`: thrust dependent exhaust (hot gas with Mach diamonds, afterburner plume),
- `WingVortexEffect`: condensation trails behind the wing tips while pulling G,
- `DamageSmokeEffect`: smoke and, when badly hit, fire and embers. The damage level is computed from the same
  `system_health` values the damage display uses.

The renderer stays pure: it only offers `SCRenderer::createTexture()` and `SCRenderer::drawTriangles()` (world-space
triangles, texture, alpha or additive blending). All effect geometry and the procedural textures are built here.
Plain fixed-function OpenGL, nothing else is required.

## Mouse orbit for the external cameras (`EnhancedStrike`)

`SCStrike` provides two empty hooks, `afterInput()` (after the keyboard/mouse processing, before the simulation) and
`afterCameraPlaced()` (after the camera of the current view has been placed). `EnhancedStrike` uses them for the chase (F2),
target (F7) and rotating cameras of the camera director and for the missile camera (F8):

- the mouse position orbits the camera around the plane (screen edge = 180 degrees), no button needed,
- the mouse wheel zooms, the middle button resets the zoom,
- while orbiting, the mouse does not fly the plane.

`[Video] camera_mouse_invert_y = 0` makes mouse-down raise the camera instead of lowering it.

## Debug keys

With the environment variable `NEOSC_DEBUG_KEYS` set, F12 steps the player's aircraft through damage stages (repaired, light damage with smoke, heavy damage with fire). The aircraft is never destroyed, so the effects can be looked at without being shot at.
