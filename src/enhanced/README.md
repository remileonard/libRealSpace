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

## 3D cockpit as forward view (`EnhancedStrike`)

With `[Video] cockpit = 3d` in `config.ini` the forward view and the left/right/rear looks (F1, F3, F4, F5) use the 3D virtual
cockpit with the head fixed in that direction instead of the 2D 320x200 cockpit; the mouse keeps flying the aircraft. The
explicit cockpit key still gives the free mouse look. Default is `2d`. `[Video] cockpit_fov` (default 45, the original value) sets
the vertical field of view of the 3D cockpit views; 55-65 shows more of the lower panel, especially on wide windows. Implemented in `EnhancedStrike::afterInput()`, the core
is not changed.

## Cockpit glass (`EnhancedStrike`)

`SCStrike` offers a second empty hook, `afterCockpitSceneRendered()`, called in every cockpit view (2D and 3D cockpit) after the 3D scene and before the 2D cockpit overlay.
`CockpitGlassEffect` draws a thin overlay directly in front of the camera: a faint reflection with a streak that follows the
sun, fine dust and scratches that light up when looking towards the sun, a glare veil and halo, and impact cracks that appear
step by step with the damage level of the player aircraft.

## HD cockpit textures (`EnhancedCockpit`)

`EnhancedStrike::createCockpit()` returns an `EnhancedCockpit`. With `[Video] hd_textures = 1` its `init()` replaces cockpit
images by PNG files from `./assets/hdtex/`. The file name is the first 8 characters of the image name plus its original size,
for example `FI6PIT1_70x121.png`. The replacement may have any larger size (RGBA, mipmapped) and is uploaded to the existing
GPU texture, the renderer is not touched. Without a matching file the original image is used. The console prints
`[enhanced] HD textures: N replaced`. Example files for the side consoles are in `resources/hdtex` (see `CREDITS.md`),
`stb_image.h` (public domain / MIT) is in `third_party`.

Not verified: images that the game rewrites later (marked dirty) would show the original again; none were observed.

## Water splash (`WaterSplashEffect`)
With `[Video] water_splash = 1` (default 0) shots and bombs that hit open sea no longer show the land explosion sprite. Each
frame `EnhancedStrike::afterCameraPlaced()` looks for new entries in `mission->explosions`. If all four vertices of the terrain
cell below are sea (`type == 0`), the explosion is marked finished and a splash is started: a ring wave on the water, soft spray
puffs and droplets that rise, fall back and fade out (about 1.3 s). Hits at the coast keep the normal explosion. Drawing happens
in `afterPlaneRendered()` once per frame. There is no splash sound, the original impact sound is unchanged.

## End-mission question (`EndMissionPrompt`, `GameUI`)
ESC in a mission ends it at once, which is easy to hit by mistake instead of F2. `neosc_enhanced` always asks: `EnhancedStrike::confirmEndMission()` pauses the simulation and shows a panel with the
two buttons "END MISSION" and "STAY IN COCKPIT". Mouse (own cursor, hover, click) and keys work: Enter ends the mission, ESC or N
goes back. If the game menu assets cannot be loaded the question is skipped and ESC behaves as before.

`GameUI` is a small immediate-mode UI in the look of the main menu: a panel of any size cut from the silver board of
`MAINMENU.PAK` (corners and edges from its clean margins), bevelled buttons with free text in `OPTFONT.SHP`, labels with whole-number
scaling and a cursor. It draws into a 320x200 palette picture (main menu palette), uploads it as one texture and draws it over the
finished frame. `NEOSC_UI_SHEET=NAME` shows all images of `DATA\FONTS\NAME.SHP` in the dialog (debug aid to look at the game's own UI art).

## Options screen (ALT+O, `OptionsPanel`)
ALT+O pauses the mission and opens a panel with the switches of the enhanced module: "Enhanced Water" (`water_splash`), "Enhanced Cockpit"
(`cockpit = 3d`, with the cockpit glass) "Enhanced Outside Camera" (`camera_mouse_orbit`, mouse orbit of the external cameras) and "Enhanced F-16 Model" (`enhanced_f16_model`: the OBJ airframe from `assets/models` and the thrust dependent exhaust flame, off = original model and flame; `EnhancedEntity` keeps both geometries and swaps them at run time). A click
flips the switch at once and writes it to `./assets/config.ini` (a copy `config.ini.bak` is made on the first save). ESC or CLOSE leaves.
