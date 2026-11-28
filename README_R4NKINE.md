# R4nkine in libRealSpace: r4bridge, r4objviewer and neosc_r4

This code, on the `r4nkine` branch of libRealSpace, connects **R4nkine** to **libRealSpace**. It reads nothing again.
- libRealSpace reads the data, as before:
  - `AssetManager` opens the TRE files;
  - `RSPalette` reads the palette;
  - `RSEntity` reads a 3D object.
- **r4bridge** converts an `RSEntity` into an R4nkine model (`r4::assets::Model`).
- **R4nkine** loads it on the GPU and draws it (OpenGL 3.3 core, or 4.1 on macOS).

```
TOBIGTRE.TRE ──AssetManager──▶ TreEntry ──RSEntity::InitFromRAM──▶ RSEntity
                                                               │
                                     r4bridge::toModel(entity, palette)
                                                               ▼
                       r4::assets::Model (images, materials, meshes, nodes)
                                                               │
                                          renderer.upload(model)
                                                               ▼
                       r4::scene::Node ──renderer.render(root, camera)──▶ the screen
```

## Contents

| File | Function |
|---|---|
| `src/r4bridge/R4Bridge.h/.cpp` | The adapter from `RSEntity` to `r4::assets::Model`, and the table of the shades of the palette |
| `src/r4bridge/R4World.h/.cpp` | The terrain of an area (`RSArea`) and its overlays (the runways) as R4nkine models, and the matrices of the map objects (`MapObject`) and of the mission parts (`MISN_PART`), calculated as `SCRenderer` does |
| `src/r4bridge/CMakeLists.txt` | The static library `r4bridge` |
| `src/executables/r4objviewer/main.cpp` | The viewer: list of the objects or of the missions, camera, GUI, post process |
| `src/executables/r4objviewer/WorldScene.h/.cpp` | A mission as an R4nkine scene: `RSMission`, `RSWorld` and `RSArea` loaded by libRealSpace, the terrain, the map objects and the mission parts |
| `src/executables/r4objviewer/TerrainDiag.h/.cpp` | The diagnostic of a terrain (`-diag`): its types and its textures as images |
| `src/executables/r4objviewer/CMakeLists.txt` | The executable, linked as the debugger |
| `src/r4game/R4Screen.h/.cpp` | The window of the game on R4nkine: GL context, device, post process, the image where the game draws, the TV start effect |
| `src/r4game/R4VGA.h/.cpp` | The 320 x 200 screen (menus, conversations, films, 2D cockpit) shown by the overlays of R4nkine, over what the game drew |
| `src/r4game/R4Loader.h/.cpp` | The loading screen shown by an overlay of R4nkine |
| `src/r4game/R4Renderer.h/.cpp` | The renderer of libRealSpace on R4nkine, with no GL call: its matrix stack, the models and the effects of the game kept and then drawn in one pass, the sky, the target camera |
| `src/r4game/R4Panel.h/.cpp` | The R4nkine panel over the game (F11): performance and render settings |
| `src/r4game/R4ConfigPanel.h/.cpp` | The configuration panel (Ctrl+F11): the options of `config.ini` and the inputs of the actions |
| `src/r4game/R4Timer.h/.cpp` | The time of the game: the measured duration of each frame, at all frame rates |
| `src/r4game/R4Mission.h/.cpp` | The mission on the scene graph: the graph of the mission (terrain, areas, scenes, actors, triggers) and its APP |
| `src/r4game/R4Strike.h/.cpp` | The view of the flight: the display, the order of the passes, the keys and the cameras; it starts the APP of the mission and draws it |
| `src/r4bridge/R4SoundFx.h/.cpp` | The sound effects in PCM: `renderSoundFx` renders an XMIDI effect off line with a `MusicDevice` (OPL or munt), and finds a loop without end. `decodeVoc` decodes a VOC |
| `src/r4game/R4Mixer.h/.cpp` | `R4Mixer`, a `RSMixer` on `r4nkine_audio` (OpenAL Soft). The music is a stream. The effects of the game are sources in space. The VOC of the channels play at the listener |
| `src/executables/r4soundplayer/` | `r4soundplayer`: a player of the sounds of the game, as `neosc_r4` plays them (refer to "Audio") |
| `src/executables/neosc_r4/main.cpp` | `neosc_r4`: the `main` of neosc with `R4Screen`, `R4VGA`, `R4Loader`, `R4Renderer`, `R4Strike` and `R4Timer` |
| `cmake/R4nkine.cmake` | Adds R4nkine (local folder or FetchContent), r4bridge, r4objviewer, r4game and neosc_r4 |

## Build

All is already on the `r4nkine` branch of libRealSpace: `include(cmake/R4nkine.cmake)` in the main `CMakeLists.txt`, and `glm`, `stb` and `cgltf` in `vcpkg.json` (R4nkine needs only their headers).

1. **Configure.** There are two ways to get R4nkine.
   - From a local clone (recommended to change the two at the same time):
     ```
     cmake --preset <your preset> -DR4NKINE_DIR=/path/to/r4nkine
     ```
   - Without `R4NKINE_DIR`: CMake downloads it from GitHub (FetchContent), branch `R4NKINE_TAG`. The default is `main`. To get the new commits of R4nkine, run `cmake .` again in the build folder.

   With `-DREALSPACE_R4=OFF`, none of this is built. Without `r4nkine_audio` in R4nkine (the `audio` feature of vcpkg, OpenAL Soft), `R4Mixer` is not built and `neosc_r4` keeps `RSMixer` and SDL_mixer.

2. **Run**:
   ```
   r4objviewer -data /path/to/the/TRE [-object F-16DES] [-tre TOBIGTRE.TRE] [-textures]
   r4objviewer -data /path/to/the/TRE -mission MISN-A1 [-focus TOWB24] [-distance 2500] [-graph]
   ```
   - **The games**: without `-tre`, the viewer opens each game that is there **alone**, because the two games have different files with the same names (301 objects, the palette, the fonts):
     - `BIGTRE.TRE` for Strike Commander. Its terrain textures (TXMPACK, ACCPACK) are in `TEXTURES.TRE`. Without this file, those of `TOBIGTRE.TRE` replace them: it is opened second and gives only the files that are not in BIGTRE. Its objects and its missions are not listed.
     - `TOBIGTRE.TRE` for Tactical Operations.
     - Without the two, `OBJECTS.TRE` and `MISC.TRE`.

     The **Game** menu at the top left goes from a game to the other and makes all again (palette, objects, missions). The window stays open. `-game sc|to` selects the game at the start; with `-mission`, the game that has this mission is opened. Without the terrain textures, the viewer refuses a mission with a message. Thus RSArea does not crash.
   - At the top left, **Mode**: *Objects* (one object at a time) or *Missions*.
   - **Missions** mode:
     - the list shows only the missions and the worlds whose terrain (`<tera>.PAK`) is in the `-data` folder;
     - a world alone, without a mission, shows its terrain and the map objects;
     - on the right, the **World** section:
       - the layers *Terrain*, *Runways*, *Map Objects*, *Mission Parts* and *Forests*;
       - **Forest Density** (`-forests 0..3`): none, sparse, normal or dense;
       - the level of detail of the terrain;
       - numbers: triangles, textures, objects, objects that are not in the TRE, light of the world, load time;
     - the graph: below "parts of the mission", a node for each area of the mission (to go into an area starts its scenes), then the scenes of this area;
     - the **Scenes** section: the parts grouped by the scene that makes them appear; each scene can be shown or hidden, and "No scene" (the parts that a program of the mission adds) is hidden at the start;
     - the **Parts** and **Map Objects** sections: a click puts the camera on the object;
     - `-focus NAME` does the same at the start, `-at X,Y` puts the camera on a point of the map (axes of R4nkine), and `-distance D` sets the distance of the camera.
   - At the top right, **Scene Graph** (or `-graph`), at the top of the right panel: the graph of the scene shown (object or mission) is below this button. It shows the type of each node and a light to hide or show it. A click on a node moves the camera to it.
   - Left panel in Objects mode, **Objects**: the list of the objects of the TRE files. A click selects an object, the wheel scrolls the list, and there are Previous and Next buttons.
   - Right panel:
     - **Render**: wireframe, textures, LOD factor, light, sky, angles of the camera;
     - **Post Process**: MSAA, FXAA, scanlines, palette...;
     - **Performance**: FPS, curves of the times (frame, cull, draw), draw calls, triangles, state changes, items and nodes;
     - **Object**: vertices, faces, textures, the LODs with their distances (the one shown is marked), the load and conversion time, the R4nkine meshes;
     - **Textures**: each image of the object, with its size and its number of frames. `-textures` opens this section at the start.
   - Mouse: left button to turn, wheel to zoom, right button to move.
   - Keyboard: up and down arrows for the previous or the next object, Esc to quit.
   - In **Render**:
     - **Flat Shading** (or `-flat`): the objects use the normal of their face, as `object_gouraud = false` in the game, and `RSArea` calculates the terrain again with `terrain_flat`;
     - **Gouraud by Vertex** (or `-gouraud`): the lighting of the objects is calculated at the vertices and then interpolated, as in the game (the back of a double face uses the normal of the face). Else, it is calculated at each pixel.
     - **Texture Upscale** (`-upscale 0..3`): the textures of the game are made larger one time, when they load, with 2xSaI, xBR 2x or xBR 4x (`r4/assets/upscale.h`). This applies to the objects, the terrain and the runways; in the atlas of the runways, each image is made larger before it goes into the atlas. The alpha counts as a color, thus the holes (index 255) keep sharp edges. xBR 4x adds approximately 1 s to the load of TURKEY.
     - **Mipmaps** (`-mipmaps`): with their mipmaps. The game has none; with them, the borders of the cells of the runway atlas go from 2 to 8 texels. The terrain textures have none: they are palette indices, filtered in the shader after their shading.
     - **Anisotropic Filter** (`-aniso N`, from 2 to 16): the anisotropic filter, when the GPU has it.

       These three settings make the object or the world shown again (`r4bridge::textureQuality()`, `finishTexture`). By default, all is as in the game.

     The two are set at the start from the `config.ini` of the game, if it is there.
   - **Sun** (the two modes):
     - **Sun Heading / Sun Elevation**: the direction of the sun (`-sun H E`), which lights the objects;
     - **World's Light**: goes back to the light of the world, `RSWorld::light_direction`;
     - **Lit Terrain** (`-lit`): the sun lights the terrain, not the light of the data. Each triangle takes the lightest shade of the ramp of its terrain type (`RSArea::terrainTone`), made darker along this ramp by the intensity, as the objects (PaletteRamp: ambient + light × N·L). The normals are calculated from the relief, on all the map;
     - **Relief of the Light** (`-relief 0..4`): multiplies the relief for the normals only (×1 to ×16, ×4 by default). The relief of the maps is soft, with a vertex each 1,000, but the light of the data makes it much stronger.
     - **Cast Shadows** (`-shadows`): the shadows of the sun. A *shadow map* (2048², orthographic view from the sun) covers the view up to **Shadow Distance** (Auto: 3 times the distance of the camera), with a 3×3 PCF and a fade at the edge. The objects up to 8 km toward the sun cast their shadow into it, for example a plane high above the ground. The lit objects lose the direct light and keep the ambient light. What is not lit, as the terrain with the light of the data or the runways, is made darker by **Shadow on Unlit**.
   - **Navigation**, in a mission:
     - the keys at their place on the keyboard (WASD on QWERTY, ZQSD on AZERTY) and the arrows to go forward and to the side, Q/E (A/E on AZERTY) to turn, R/F to go nearer or farther, Shift to go faster;
     - **double click** on the terrain: the camera goes there (a ray against `RSArea::getY`);
     - **Follow the Ground**: the target stays on the ground;
     - the **Whole Map** and **Top View** buttons, and one button for each area of the mission.
   - `-frames N -shot image.bmp` makes a capture after N frames, for the tests.
   - `-mission NAME -diag FOLDER` writes the diagnostic of the terrain into FOLDER, which must exist. All is at the LVL0 level, 360 × 360 vertices, with north at the top:
     - `terrain.ppm`: the colors of RSArea;
     - `types.ppm`: the type of each vertex, in the lightest shade of its ramp;
     - `textures.ppm`: the textured triangles, each in the average color of its image;
     - `tex_<id>.ppm`: each image used, with the light texels in magenta;
     - `types.txt`: the number of vertices of each type;
     - `textures.txt`: for each image, the number of triangles, the types where it is and its first cells.

     With `-frames 1`, the viewer then closes.

## neosc_r4: the game on R4nkine

`neosc_r4` is neosc with its display through R4nkine. The work is done in steps:

| Step | Content | State |
|---|---|---|
| 1 | `R4Screen`, `R4VGA`, `R4Loader`: the window, the post process, the 320 x 200 screen and the loading screen | **done** |
| 2 | `R4Strike`: the flight on the scene graph (terrain, planes, objects, weapons), updated by the APP | **done** |
| 3 | All the rest by R4nkine: effects, clouds, sky, target camera, 3D cockpit, object viewer | **done** |
| 4 | The core context (3.3, 4.1 on macOS): the game makes no GL call | **done** |
| 5 | `R4Mission`: the graph belongs to the mission (areas, scenes, triggers); the APP goes through the callbacks of the nodes | **done** |
| 6 | The performance: clouds made one time for each area, forests in static instancers, `R4Timer` | **done** |
| 7 | `R4ConfigPanel` (Ctrl+F11): the options of `config.ini`, the keys, the controllers and the joysticks | **done** |

**The hooks in libRealSpace.** These changes of libRealSpace (before, the patch `r4-hooks.patch`) do not change the behavior of neosc:
- `RSVGA`: `displayBuffer`, `init`, `activate` and the destructor are virtual; the creation of the GL textures is in `initTextures()`, virtual; the private members are protected;
- `Loader`: the display of the loading surface is in `presentSurface()`, virtual; the destructor is virtual;
- `SCStrike`: the draw of a view is in `renderScene`, `renderWorld` and `renderPlayerPlane`, virtual; `SCStrike::create()` and `setFactory()` select the type of the flights (`SCGameFlow::flyMission` and `SCTrainingMenu::OnSearchAndDestroy` use `create()`); `createMission`, virtual, makes the mission of the flight;
- `SCMission`: the destructor is virtual; `SCMissionEvent` has `MissionEventAreaEntered` and `MissionEventAreaLeft`;
- `RSArea::ground`: a function that gives the height of the ground to `getY`; `R4Mission` sets it to `r4bridge::groundHeight`;
- `SCPlane`: `getWheelIndex()`, `ground_clearance` (the height of the wheels below the origin of the model) and `SetNormalizedStick`; `SCSimulatedObject::smokeSet()`;
- `SCRenderer`: `pushMatrix`, `popMatrix`, `translateMatrix`, `rotateMatrix`, `scaleMatrix`, `clearDepth` and `readRenderToTexture`, virtual (by default, the GL calls of before); `SCVirtualCockpit::render`, `SCObjectViewer::runFrame` and `SCCockpit::RenderMFDSCamera` use them and do not call GL;
- the inputs: the axis actions `AXIS_*` and `bindFlightAxes` (`GameEngine`), the GUID of the devices, the deadzone and the curve of each axis (`InputActionSystem`, `EventManager`);
- `Config`: the write of `config.ini`.

**Step 1.**
- `R4Screen` opens the window and the OpenGL context (refer to step 4).
- The post process of libRealSpace (`fx_cpc_palette`, `fx_scanlines`, `fx_pixel_scale`, `fx_fxaa` of `[video]`, and `msaa` of `[Video]`) goes through the post process of R4nkine. When an effect is on, the game draws into the target of the post process (`Device::bindForHost`), then `refresh()` applies the effects into the window. The CPC palette has 27 colors (0, 128 or 255 for each channel).
- `R4VGA` receives the 320 x 200 image after the palette, the fades, the contrast and the tint (`displayBuffer`). It shows it in 4:3 at the center of the window with an overlay, sharp or filtered with `super_eagle_2x`, and the side bands of the wide screen (`widescreen_ambilight`). Index 255 stays transparent: the 2D cockpit is over the 3D.
- `R4Loader` shows the loading surface in 4:3, with its colors converted from the format of the SDL surface.
- Tested here without the full data of the game (SC.DAT, LILTRE.TRE and VOCLIST.TRE are missing): a test program runs the real classes. The TV start effect is shown, then the 320 x 200 screen over a scene drawn with the fixed pipeline. Its transparent part shows the scene, and the GL state of the game is the same after each pass of R4nkine. With scanlines, pixels x 2 and MSAA 4x, the final image has the effects.

**Step 2, then step 5: the flight on the scene graph.**
- `R4Strike` replaces `SCStrike` for all the flights: campaign, training, Tactical Operations. `neosc_r4` gives it to the factory `SCStrike::create()` (`SCStrike::setFactory`).
- `SCMission` loads and runs the mission. The graph belongs to `R4Mission : SCMission`, which `R4Strike::createMission` makes. It is made when the mission loads and lives as long as the mission, in the axes of libRealSpace (Y up):
  - `terrain` (mask `LAYER_TERRAIN`): the terrain of `r4bridge::terrainModel` (quadtree, rings, atlas) and the runways of `overlayModel`, made one time for each area;
  - the planes (mask `LAYER_ACTORS`, `LAYER_PLAYER` for the plane of the player): in one group, because a plane goes from an area to another; each has its body, its wheels, its afterburner and its weapons below the hardpoints;
  - a node for each area of the mission, with the objects that are in it; the objects of a scene are below a `Switch` of this scene, on when the scene is activated;
  - `simulated objects` (mask `LAYER_SHOTS`): a node for each missile, bomb or shell that has a model;
  - a trigger for each area (mask `LAYER_TRIGGERS`), with its volume (sphere, tube or box): it puts `MissionEventAreaEntered` and `MissionEventAreaLeft` of the planes on the message bus.
- Each actor has a `LOD` node at the render distance of `SCStrike` (`RENDER_DISTANCE`).
- `r4bridge::toModel` makes the models of the `RSEntity` objects when the mission loads. The nodes share them.
- **APP** (`R4Mission::app`, started by `R4Strike` one time in each frame, after the camera is set): it makes the nodes of what appears and removes the nodes of what is gone. Then `r4::scene::app` runs the callback of each node, which takes the state of its object: matrix, wheels, afterburner, stores, model, with the same calculations as `SCPlane::Render` and `SCRenderer::drawModel`. Then the triggers compare the positions of the planes with their volumes.
- **The ground**: `RSArea::ground` gives to `getY` the height of `r4bridge::groundHeight`, on the LVL0 triangles that the renderer draws. The planes on the ground stand on their wheels (`ground_clearance`).
- **CULL and DRAW** (`R4Strike::renderWorld`): the graph of the mission goes into the pass of `R4Renderer` (`addScene`), with the sky, the effects of the game and the 3D cockpit. One pass of the renderer draws the view, with the camera of the game.
- The flight with the debug boxes (`show_bbox`) uses the draw of `SCStrike`, through `R4Renderer`.
- `RenderOptions::clear` is false: a pass of the renderer keeps the color and the depth of the target (the sky of the game).
- Tested here with a test program on MISN-1A (before `R4Mission`, when `R4Strike` kept the graph): the cockpit view (2D cockpit over the 3D) and the outside view (F-16 of the player with its stores, another plane, the runway) give the same image with `SCRenderer` and `SCStrike` as with `R4Strike` and its graph. With scanlines and pixels x 2, the 3D goes through the target of the post process.

**Step 3: all the rendering by R4nkine.** `R4Renderer` replaces all the methods of `SCRenderer` that draw. It does not call GL.
- **Matrices**: its own matrix stack (`pushMatrix`, `translateMatrix`, `rotateMatrix`, `scaleMatrix`, `popMatrix`) in place of the GL stack. All the `drawModel` methods (position, orientation, adjustment, scale, `drawModelWithChilds`, `renderObjects`, `renderMissionObjects`) calculate their matrix as `SCRenderer` does (`glRotatef` in degrees or `Matrix::rotateM` in radians).
- **Camera**: `bindCameraProjectionAndViewViewport` and `initRenderCameraView` give the view, the projection (with the vertical offset) and the viewport of the pass, from the `Camera` of the game.
- **Pass**: the models, the effects, the sky and the graphs (`addScene`) are kept, then drawn in one `render()`: before a change of view, before the 320 x 200 screen (`R4Screen::beginGamePass`) and at the end of the frame (`refresh`).
- **Effects**: billboards, sprites, particles, tracers, effect triangles (`drawTriangles`, additive or alpha), textured quads (panels of the 3D cockpit), clouds, lines, boxes and points become triangles in the world, grouped by state (texture, blend, depth test and depth write, alpha test, bin). One mesh for each group and each pass.
- **Order**: the sky (bin -2, no depth, no fog) then the clouds (bin -1) before all, as the dome and the clouds of `renderWorldSolid`; the points and the lines last, with no depth, as the game draws them after the models.
- **Clouds**: made one time for each area in the graph (`makeClouds`, when `useArea` is called), a Geode for each cloud with all its puffs in one mesh, culled by its bounds. Before, each frame made again on the CPU, and sent again to the GPU, up to 500 clouds of 7 to 11 puffs of 432 vertices. The puffs are white: their blend gives the same color in all orders, thus there is no sort by distance.
- **Textures of the game**: the `Texture` objects (billboards, panels of the 3D cockpit) and the textures of `createTexture` go to R4nkine; `uploadTextureContentToGPU` updates them.
- **Target camera**: its view has its own render target of 107 x 75. `getRenderToTexture` reads it back, and the cockpit reads it through `readRenderToTexture`. The MSAA problem of step 1 is gone.
- **R4nkine**: `Material::depthTest` and `Material::fog` (the sky), the background bins (negative `drawOrder`, drawn before all, blended or not).
- Tested here, with the same scene through `SCRenderer` (compatibility context) and through `R4Renderer` (core context): terrain, models, billboard, sprite, particle, additive and alpha triangles, tracer, textured quad, line, point, boxes, cloud, and the image of the target camera. The images are the same. One difference is intentional: the sprites and the particles do not write the depth and keep the depth test; in the game, they lost it because of a GL state that `drawModel` left.

**Step 4: the core context.**
- `R4Screen` requests an OpenGL 3.3 core context (4.1 core with forward compatibility on macOS). The `gl_compatibility` key of `[Video]` requests a compatibility context, only to compare with `SCRenderer`.
- The game makes no GL call in flight, in the menus and in the object viewer. Only the VR display of `SCStrike` (`drawTargetSquareOverlay`) still calls GL: it will be done again with OpenXR.
- R4nkine: `createShader` left a GL error after an interop (an unknown program was restored). Fixed.
- Tested here in a core context with the GL debug output: MISN-1A in the cockpit, outside and target views, 200 frames each, the object viewer, and the scene of the effects: no GL error. macOS is not tested here.


**The render options (`config.ini`, section `[R4nkine]`).** `R4Renderer` reads them when it starts. Without them, the image stays the image of the game.

```ini
[R4nkine]
; cast shadows: a shadow map from the light of the objects of the game
shadows = false
shadow_map_size = 2048
; in meters: the zone covered in front of the camera
shadow_distance = 1500
; the trees also cast their shadow (costly)
tree_shadows = false
; off, noise (woods and clearings on the green of the terrain) or textures (the trees painted in the textures)
forests = off
; multiplies all the densities
forest_density = 1.0
; in km (multiplied by lod_scale): the detailed trees, then the simple trees, thinned beyond the detailed trees
tree_detail_km = 1.5
tree_km = 6
; the buildings on the textures of cities, villages, farms and harbors
buildings = false
; none, 2xsai, xbr2x or xbr4x: makes the textures of the objects and of the terrain larger
texture_upscale = none
texture_mipmaps = false
; anisotropic filter, from 1 to 16
anisotropy = 1
; the sun lights the terrain along its ramps, as the objects, not the light of the data (-lit of r4objviewer)
sun_terrain = false
; multiplies the relief for the normals only, from 1 to 16 (-relief)
terrain_relief = 4
; the part of its color that what is not lit loses in the shadow: terrain of the data, runways (Shadow on Unlit)
shadow_strength = 0.5
; the sun, in degrees: heading from north to east, elevation (-sun H E); without these keys, the light of the objects of the game
; sun_heading = 120
; sun_elevation = 30
; the light of the objects (ambient + direct x N.L); without these keys, the light of the game (Ambient, Light)
; light_ambient = 0.375
; light_direct = 0.625
; the fog in km from the eye; without these keys, the fog of the game (0.40 and 0.93 x max_view_distance); show_fog of [Game] turns it on
; fog_start = 64
; fog_end = 149
; Wireframe, Flat Shading
wireframe = false
flat_shading = false
; multiplies the LOD distances of the terrain, of the objects and of the trees (LOD Scale, 0.05 to 4)
lod_scale = 1
; distance (by the distance), fine, medium or coarse (Terrain Detail)
terrain_lod = distance
vsync = true
; the dome of the game (sky and ground); without it, the background of the world is sky_color, as in r4objviewer, and the fog takes this color
sky = true
sky_color = 0.30, 0.42, 0.60

; the post process of r4objviewer; each key that is there replaces the setting of the game (fx_* of [video], msaa of [Video])
; msaa = 1               ; 1, 2, 4 or 8
; fxaa = false
; pixelate = 1           ; the image drawn as if it were N times less wide
; brightness = 0         ; -1 to 1
; contrast = 1
; saturation = 1         ; 0: grey
; gamma = 1
; tint = 1, 1, 1         ; r, g, b from 0 to 1, multiplied
; fade = 0               ; 0 to 1 toward fade_color
; fade_color = 0, 0, 0
; palette = none         ; none, vga256, ega16, cga4, handheld4, amber8, grey16 or cpc27 (Reduce to a Palette)
; dither = true          ; Bayer 4 x 4 with the palette
; scanlines = false
; scanline_strength = 0.35
; scanline_size = 3      ; pixels for each line
; curvature = 0          ; 0 to 0.3
; aberration = 0         ; pixels, at the edges
; vignette = 0           ; 0 to 1
```

The settings of r4objviewer that are not here: **Textures** and **Gouraud by Vertex** are `show_texture` and `object_gouraud` of `[Game]`, **Fog** is `show_fog` of `[Game]`. The mode, the camera, the navigation, the layers (terrain, runways, objects, parts), Follow the Ground, On Ground and `-nolift` are only for the viewer: in the game, the game places its camera and its objects.

- The forests and the buildings come from `r4bridge` (`forestChunks`, `plantForest`, `treeModel`, `cityModel`), as in r4objviewer. They are made one time for each area. There are none on the runways, on the map objects and on the mission objects (`R4Mission` gives their positions and their sizes). A mission object with no vertices keeps nothing away: its box (`RSEntity::CalcBoundingBox`) stays at `FLT_MAX`, and its infinite radius removed all the forests and all the buildings of the map.
- The forests are in blocks of 2 km: a LOD for each block, an instancer for each species, the full tree up to `tree_detail_km` and the simple tree up to `tree_km`. The instancers are static: their trees go one time to the GPU, there is no work for each tree in a frame, and beyond the detailed trees a part of the trees is removed.
- The sun (`sun_heading`, `sun_elevation`) lights the objects and the terrain (`sun_terrain`) and gives the direction of the shadows. Without these keys, it is the light of the objects of the game (`setObjectLight`), almost vertical: the shadows are short.
- In wireframe, the sky dome and the clouds are also lines: they are meshes, but the sky of r4objviewer is the background color.
- The shadows are not made in the view of the target camera. Without `tree_shadows`, the forests (mask `R4Renderer::kForestMask`) are not in the shadow pass.
- When an area loads, the console gives the number of trees and of buildings.
- Measured here on ARENA (software rendering, thus only to compare): area load 1.0 s with no option; `forests=noise` 5.3 s for 8.7 million trees and approximately 320 MB more; `forest_density=0.4` 2.8 s for 3.5 million; `forests=textures` 1.8 s for 3.0 million; `buildings=true` 1.1 s for 6,838 buildings.
- Tested here with no GL error, in a core context: MISN-1A and ARENA with all the options, a near view where the F-16 and the tank cast their shadow on the terrain, then the post process (MSAA 4x, FXAA, dithered EGA palette, scanlines, curvature, vignette, aberration; brightness, contrast, saturation, gamma, fade), the wireframe, the flat shading with another light, the coarse terrain with `lod_scale` 0.2 and a fog from 0.2 to 1.5 km.

**The timer (`R4Timer`).** `neosc_r4` replaces `DesktopTimer` with `R4Timer` (`GameTimer::setTimer`, in the `main`). The physics, the AI and the mission advance by `getDeltaTime()` at each frame. `DesktopTimer` did not give the real time:
- after 90 frames, it rounds the measured rate to 60 at most (`snapToStandardFPS`);
- then it follows the rate only if it stays between 10 and 200 frames per second;
- above 200 (without vsync), each frame thus counts 1/60 s: at 300 frames per second, the game goes 5 times too fast;
- it measures in milliseconds (`SDL_GetTicks64`): at 3.3 ms for each frame, each measure is 3 or 4 ms.

`R4Timer` gives the measured duration of each frame with `SDL_GetPerformanceCounter`, with no smoothing. A frame longer than 0.1 s (a load) counts as 0.1 s. `shouldSimulate` keeps the fixed step of 1/30 s.

**The R4nkine panel (F11).** `R4Panel` (in `src/r4game`) is the GUI of R4nkine over the image of the game, after the post process. F11 alone shows or hides it (Ctrl+F11 is the configuration panel). It reads the mouse and the keyboard of SDL at each frame, and the wheel through an `SDL_AddEventWatch`: the events stay with the game, thus a click on the panel also goes to the game. While it is shown, the cursor is visible and the mouse is not relative (the game hides the cursor in its window, and its menus use the relative mouse); F11 then gives back the state of the game.
- **Performance**: FPS and average time on 240 frames, curves of the frame, of the cull and of the draw, then, for each frame and for all the passes of the renderer: passes, draw calls, triangles, pipeline and texture changes, nodes visited and culled, drawables, instances, opaque and transparent items, objects that cast a shadow, times of the shadow, of the sort and of the submit. The draw of the panel is not counted.
- **Area**: what `useArea` did for the area (time, trees, buildings, clouds), and the options of `config.ini` that change only with a new area (forests, buildings, upscale, mipmaps, anisotropic, lit terrain, relief, terrain level, light).
- **Render**: the settings that the next pass reads, changed live: wireframe, flat shading, textures, Gouraud, fog, clouds, sky and background color without sky, shadows, tree shadows, LOD Scale, distance and strength of the shadows, sun and fog of the settings, VSync.
- **Post Process**: the controls of r4objviewer (`r4::ui::postProcessControls`), live.

**The configuration panel (Ctrl+F11).** `R4ConfigPanel` is an R4nkine window at the center of the screen (70 %). Ctrl+F11 or its cross closes it. While it is shown, the game gets no key and no mouse: an SDL event filter keeps them, and resets the keyboard state that the game reads. The controllers and the joysticks still act: the panel only looks at their events.
- **Window, Game, Video, Render (R4nkine), Sound**: the options of `config.ini`. Each option tells when it applies: now, at the next mission (*mission*) or at the next start (*restart*).
- **Input**: the keys file (`bindings_file` of `[Input]`), then the actions of the flight.
  - **Change** waits for a key (with Ctrl, Alt or Shift) or a button of a controller or of a joystick. For an axis action, it waits for an axis pushed far from its rest, in the direction of the name of the action: this direction gives the sign. Esc cancels.
  - **Change** replaces the inputs of the same family (the keyboard, or the controllers and the joysticks), **Add** keeps them, **Clear** removes all but the mouse.
  - Below an axis action, each axis has its deadzone (0 to 0.5), its curve (exponent from 1 to 3) and **Inverted**.
- **Devices**: the controllers and the joysticks that are plugged in, their GUID, their axes live and the buttons pressed.
- **Save** writes `./assets/config.ini` and the keys file.

**The inputs.** The axes of the flight are actions: `AXIS_PITCH`, `AXIS_ROLL`, `AXIS_RUDDER`, `AXIS_THROTTLE`, `AXIS_LOOK_X` and `AXIS_LOOK_Y` (69 to 74 in `SimActionOfst`).
- By default, when an axis has no binding (`bindFlightAxes`): the left stick for the roll and the pitch, the right stick for the look, the triggers for the rudder. The throttle has no default axis.
- The old actions of the sticks (55 to 58) are not used: their bindings are cleared when the file loads.
- A line of the keys file: `action,type,deviceId,code,scale,modifiers,guid,deadzone,curve`. The last three fields are optional: the old files load.
- With a GUID (the SDL GUID, the same from a start and a plug to the next), `deviceId` is the rank of the device among the devices of this GUID: 0 for the first controller of a model, 1 for the second. Without a GUID, it is the number of the controller or of the joystick.
- A deadzone of -1 uses the deadzone of the system (0.1). The game reads the axes and the buttons directly from SDL.

## Audio

`neosc_r4` plays the sound through `R4Mixer`, on `r4nkine_audio`. The design is in `docs/AUDIO.md` of R4nkine. The changes in libRealSpace for the audio: `AILXmidiDriver` (the loops without end), `RSMixer` (`setInstance`, `initEngine`, `playEffectAt`, `playOrUpdateEffectAt`), `SCMission`, `SCPlane`, `SCMissionActors`, `Mt32Synth` (`readRoms`: the ROMs read once), the Sound group of `R4ConfigPanel`.

### Player

`r4soundplayer [-data folder] [-hrtf]` shows the sounds of the game, with R4Mixer:

- The tab XMIDI: the effects of `SOUNDFX`, rendered into PCM with the synthesizer of `[Sound] midi_music`. The list gives the length and the loops.
- The tab VOC: the digitized effects of `DSOUNDFX.PAK`, with the names of `SoundEffectIds`.
- The tab Music: the tracks of a bank, played as in the game.
- For a sound: the curve of its level, Play, Stop, Loop, Gain, and Export WAV (the introduction and two passes of the loop).
- In space: a direction and a distance from the listener, or "Pass in front": the source goes from left to right at a speed, for the Doppler effect. Use headphones, and `-hrtf` to hear above and below.
- The volumes of the music and of the effects. The synthesizer of the effects (OPL or MT-32) and "Render the effects again".

The data: `./SC.DAT` and `./assets`, as `neosc_r4`. Else `-data`, a folder with `BIGTRE.TRE` and `LILTRE.TRE`.

### Settings (`[Sound]`)

| Key | Default | Function |
| --- | --- | --- |
| `max_voices` | 64 | The sources heard at one time. The farther sources are virtual. |
| `hrtf` | false | The HRTF of OpenAL Soft, for headphones. |
| `speed_of_sound` | 1125 | In units of the world a second (1125: feet). |
| `doppler` | 1 | The Doppler effect; 0 stops it. |
| `music_volume` | 100 | The volume of the music, 0 to 100. |
| `sfx_volume` | 100 | The volume of all the other sounds (effects, engine, radio, voices), 0 to 100. |
| `sfx_synth` | auto | The synthesizer that renders the XMIDI effects: `auto` (the one of the music), `opl` or `mt32`. |

`sfx_synth` selects the synthesizer of the XMIDI effects. Each effect is rendered on a new synthesizer:

- `opl`: `SOUNDFX.ADL` with the timbres of `STRIKE.AD`, on an OPL.
- `mt32`: `SOUNDFX.ROL` with the timbres of `STRIKE.MT`, on munt (MT-32 emulated). It needs the ROMs in `mt32_rom_dir`; they are read once, and each effect starts a new munt from them in memory. Without them, the effects are on the OPL.
- `auto`: the synthesizer of the music (`midi_music`). For 0 (AdLib) and 1 (General MIDI): the OPL. For 2 (MT-32 on a MIDI port, which cannot be recorded) and 3: munt, else the OPL.

When the files of the music are not those of the synthesizer of the effects, R4Mixer loads the effects of the other synthesizer.

The volumes are the volume groups of `r4nkine_audio`: the music stream is in the group `GROUP_MUSIC`, all the other sources in `GROUP_EFFECTS`. The game's own volumes (`setVolume`, `engine_volume`) stay, and are multiplied by them.

### Tests done

- The build of all the targets of libRealSpace at `3a11f13`, on Linux.
- The effects of `SOUNDFX.ADL` on the OPL (`LILTRE.TRE`): 14 effects, rendered in 0.5 to 0.9 s. 4 loop: `GUN_FIRE` (introduction 1 s, loop 4.42 s) and the effects 6, 7 and 13. They hold a note without end: their loop is found in the wave. At the join of each loop, the step between two samples is as small as the median step of the sound. The 10 other effects end (0.33 to 4.08 s).
- The TVFX voices keep a noise of 1 to 6 units after the end of an effect. Below 16 units, `renderSoundFx` takes it as silence.
- `decodeVoc`: the 31 VOC of `DSOUNDFX.PAK` (entry 22 is empty), and the 39 VOC of the cutscenes, which have the length that SDL_mixer gives.
- `renderSoundFx` on the 29 sequences of `AMUSIC.PAK`: 5 have a loop without end, and the loop is found.
- `R4Mixer` and `r4soundplayer` on the null device of OpenAL: the lists, Play, the loops, the pass in front, Export WAV, the volumes, the render of the effects again.

### Not tested

- The sound on a sound card: the tests used the null device of OpenAL. The WAV files were not heard.
- The MT-32 render (munt): there were no ROMs. Tested: a folder without valid ROMs; at the start the effects go on the OPL, and the player refuses MT-32 and keeps the effects of the OPL.
- `neosc_r4` in a mission.

## What to look at to learn R4nkine

The viewer follows the steps of an R4nkine program, numbered in `main.cpp`.

1. **The device**: `r4::gpu::createGLDevice(SDL_GL_GetProcAddress)`.
   - It is the lowest layer: buffers, textures, shaders, pipelines.
   - The GL functions are loaded at run time, thus there is no GLEW and no GLAD.
2. **The renderer**: `r4::render::Renderer renderer(*device)`.
   - It draws a scene graph: cull, opaque/transparent sort, then draw.
   - `renderer.environment()` sets the light and the background.
   - `renderer.options()` sets the textures, the LOD factor and the rest.
3. **The neutral model**: `r4::assets::Model`.
   - The loaders of R4nkine (OBJ, glTF, OpenFlight...) make this model, and r4bridge also.
   - It contains only data: `images`, `materials`, `meshes` and `nodes`, with `nodes[0]` for the root.
   - `renderer.upload(model)` makes a subgraph ready to draw from it.
4. **The camera**: `r4::render::Orbit` turns around a point and `orbit.apply(camera, fov, aspect, near, far)` fills the `Camera`.
5. **The GUI**: `r4::ui::Context`, immediate mode as ImGui but with the Performer look.
   - The code: `beginFrame`, `beginPanel`, the widgets (`button`, `toggle`, `slider`, `section`...), `endPanel`, `endFrame`, then `render()` after the scene.
   - `r4::ui::postProcessControls()` gives all the panel of the effects at one time.
6. **The post process**: `r4::render::PostProcess`.
   - `post.begin()` gives a target, the renderer draws into it with `setTarget`, then `post.end()` applies the effects: MSAA, FXAA, scanlines, palette...
   - When no effect is on, `begin()` gives a null target and the scene is drawn directly.

## How r4bridge converts an RSEntity

The bridge follows `SCRenderer::drawModel` pass by pass (color, texture, transparency):

| RealSpace (libRealSpace) | R4nkine |
|---|---|
| Axes: Y up (`entity->vertices`) | Z up: (x, y, z) becomes (x, −z, y), a rotation of 90° |
| Faces clockwise (`glFrontFace(GL_CW)`) | Order reversed (a, c, b): R4nkine expects counterclockwise |
| Normals `(a − b) × (c − b)`, smoothed as `objectNormals` (a quad counts as two triangles) | Same calculation, after the change of axes |
| `lods` (LVL0, LVL1...) and `attrs` (T/Q/L) | A `NodeKind::LOD` node, with a child for each level; `lodRanges` = [0, dist1, dist2, ..., ∞] |
| Color pass: triangle `color`, quad `color − 1` | Material `Shading::PALETTE_RAMP`; the index is in the red of the vertex color |
| Lighting (ambient 3/8, light 5/8, `shadeAlongRamp`) | Image 256 × 32 (`rampImage`): line k = intensity k/31 |
| Double face: `flags[2] == 1` (triangle), `attrs->props2 == 1` (quad) | `doubleSided`, else back face culled as in the game |
| Textures (`uvs`, `qmapuvs`, `images`) | RGBA images through the palette, index 255 transparent; `GL_LINEAR` without mipmaps and clamped edges (`Image::mipmaps = false`, `clampS/T`), as `createTextureInGPU` |
| Texture pass: blend `SRC_ALPHA, ONE_MINUS_SRC_ALPHA` and alpha test `> 0.1` | `AlphaMode::BLEND` with `alphaTest = 0.1` |
| Property 6 or 9: the texture is the face (`GL_LESS`, depth written) | Material `rampTextured`, `blendDepthWrite` |
| Other textured faces: on the colored face (`GL_EQUAL`, depth not written) | `depthEqual`, `depthWrite = false`: the texture appears only where the colored face is drawn |
| Animated textures (`nbframes` > 1: the images one below the other, `RSImage::GetNextFrame` each 10 display frames) | Material `frames` and `framesPerSecond` (6 per second, thus 10 frames at 60 Hz): the shader selects the image with `Environment::time`, the filter stays in the image |
| Transparent pass: property 2, color `color − 1`, `glBlendFunc(GL_ONE, GL_ONE)` | `AlphaMode::ADDITIVE`, palette without ramp (color × intensity) |

Set the light with `r4bridge::Lighting` or with the Ambient and Light sliders of the viewer.

To show an object in another program than the viewer, for example in a future `SCRenderer` on R4nkine:

```cpp
RSEntity *entity = ...;                       // already loaded by libRealSpace
auto model = r4bridge::toModel(entity, *palette.GetColorPalette(), palette.ranges);
r4::scene::NodePtr node = renderer.upload(*model);
someGroup->addChild(node);                    // and the renderer does the rest
```

## The missions: one change of axes only

A mission stays fully in the axes of libRealSpace (Y up):
- the terrain: `terrainModel` uses the vertices of `RSArea` as they are;
- the objects: `toModel(..., Axes::REAL_SPACE)`;
- their matrices: `mapObjectMatrix` and `missionPartMatrix` use the `glTranslate`, `glRotate` and `glMultMatrix` of `SCRenderer`.

One matrix, `r4bridge::realSpaceToR4()`, at the root of the scene, goes into the axes of R4nkine (Z up). It is the only border between the two conventions: physics, AI and mission have nothing to convert.

For the light of the world, `setObjectLight` receives (c0, c1, c2) and makes (c0, c2, −c1) in Y up. Back in the axes of R4nkine, it becomes (c0, c1, c2) again, that is, the values of the file as they are.

The terrain (`WorldScene` and `R4World`):
- **A node for each block**: a `Group` "terrain" contains the 324 blocks ("block x,y"). Each is a `LOD` node on the three levels of `RSArea` (LVL0 400 vertices, LVL1 100, LVL2 25), with its seams to its right and bottom neighbors, as `renderBlock`. Thus the renderer culls the blocks out of the view and selects the level of each by its distance: fine up to 40,000, coarse beyond 100,000, set with "LOD Scale". The game draws all its blocks at the same level (`world_detail`). The "Terrain Detail" menu has the two: "By Distance", or one level for all.
- **The shadows of the trees**: the node of the forests has the mask `WorldScene::kForestMask` (bit 31). Without **Tree Shadows** (in the Sun section; `-treeshadows` at the start), the viewer removes this bit from `RenderOptions::shadowMask`: the shadow pass skips all the branch of the forests and does not go through the trees. On NICARAGA, the objects that cast a shadow go from 304 to 4, and their CPU time from 0.26 to 0.05 ms.
- **The parts that a scene adds** (placement 1) are placed as `SCMission::placeRelativeToPlayer` places them relative to the player, but relative to the center of the area of the scene, heading north: position = center of the area + `relative_position`, heading = `relative_azymuth`. Thus the planes have the altitude of the area plus their offset, not the fixed 4,000 of before; then they are raised if they are below the ground.
- **The selection in the graph** shows the culling sphere of the node (three yellow circles) and its bounding box (purple, in the space of the parent: the boxes of the meshes of the Geodes, the spheres of the Instancers), while the Scene Graph section is open.
- **The objects put on the terrain**: `r4bridge::onGround(area, entity, matrix, flies)` moves an object so that its lowest vertex, turned by its matrix, is on the ground at its origin; an object that flies is only raised if it is below. The height of the ground comes from `r4bridge::groundHeight`, calculated on the LVL0 triangles as `terrainModel` draws them. `RSArea::getY` is not used here: it takes the nearest vertex, then interpolates in the cell that starts at this vertex, also when the point is on the other side, and adds 5 at the edge of the blocks; on ARENAMIS, its errors were up to 86 units, thus tanks floated. With the exact calculation, the objects of ARENAMIS move from -3 to +7 units. The viewer applies it to the map objects and to the parts of the mission (toggle **On Ground** of the World panel, on by default; `-nolift` keeps the height of the data). A plane of the mission flies when it is more than 10 above the ground after `placeParts`. An object out of the map does not move. The ground is measured at the origin of the object and on LVL0: from far, on LVL1 and LVL2, an object can look a little above the terrain or in it.
- **An atlas of the terrain textures**: all the images on the map (TXMPACK, ACCPACK or the textures of the map) are in one image of palette indices, in a grid of equal cells (the cell has the size of the largest image). Each image keeps its size in its cell, and is not made larger. Thus there is only one textured material for all the terrain. Each vertex has the number of its cell in the green and the blue of its color, and the log2 of the width and of the height of its image in its alpha. The shader keeps the bilinear filter in the image of the cell, thus nothing goes from a cell into another. The result is the same as with one texture for each image, but a block or a group of the quadtree costs only two draw calls (colors and textures): on TURKEY, from 66 to 50 at 30 km, from 100 to 49 at 150 km, from 92 to 75 at 400 km, and from 62 texture changes to 6. With `-upscale 3` (xBR 4x), the cells of 128 texels become 512 and the atlas of TURKEY is 4096², the maximum size that many GPUs guarantee.
- **A quadtree on the blocks**: the blocks are grouped by 2×2, 4×4... up to 32×32 (the map has 18×18). Each group is a `LOD` node with two children. Near, it shows its four children, which the renderer culls by their bounds. When all its blocks are at the coarsest level in all cases (the distance of its center is more than the start of LVL2 plus the distance between this center and the center of its farthest block), it shows one merged mesh for each material of the LVL2 of its blocks, seam bands included. The image does not change, but there are fewer draw calls: on TURKEY, from 126 to 66 at 30 km, from 215 to 100 at 150 km and from 634 to 92 at 400 km. The center of the LODs of the blocks is the center of their LVL0 vertices. "Terrain Detail" makes the groups show their blocks for Fine and Medium, and their merged meshes for Coarse.
- **Out of the map**, the terrain continues to the edge of the world (two times the size of the map on each side, as the skirts of `RSArea::BuildSkirts`). libRealSpace has no data beyond the map: the edge of the map is extended, its heights and its colors smoothed along the edge, then it fades on 150 km to the height 0 and to the average color of the edge (with `-lit`, to the most frequent terrain type of the edge, shaded on its ramp as the map).
  - A **seam band** of 20 km is around the map. Each block of the edge has its band in each of its levels of detail: the band starts at the edge vertices of this level (LVL1 and LVL2 stop before LVL0 on the right and bottom edges) and changes level with the block, with no gap. It uses the material of the terrain, thus there is no more draw call.
  - Then **two rings of tiles**: the first up to 360 km from the center, in cells of 20 km; the second up to the edge of the world (720 km), flat, one cell for each tile. Each ring has 12 tiles, each a node with its bounds and one mesh: the renderer culls the tiles out of the view, and there are at most 24 draw calls for all the outside.
  - The **fog**, in the color of the sky, starts 120 km and ends 450 km beyond the target (set in the panel, `-nofog` to remove it). The far plane stops just after the fog, which keeps the precision of the depth buffer; without fog, it goes to 1,500 km. "Whole Map" frames the map only.
- **The colors** are those of the ramps of `RSArea` (`ComputeTriangleColors`). Their shades depend on `terrain_shadow_lift` and `terrain_shadow_range` of the `config.ini` of the game: the viewer reads it in `-data` (`config.ini` or `assets/config.ini`) or in `./assets/config.ini`, else it uses the default values.
- **Skirts**: walls of 600 go down along the edges of each block, so that a neighbor at another level leaves no gap. The game does not need them.
- **The colors** come from `RSArea` and are not lit, as in the game. The textures are a `Blend` layer over the color, clamped at the edges. Their texels are palette indices shaded along their ramp, as the vertices: the intensity of each vertex is its light (`flag` / 255, averaged with `terrain_flat`), or the sun with **Lit Terrain**. Thus a texture keeps the shade of what is near it in the shadow, with no halo around the textured zones. The darkest index of a ramp stays itself (`RampFirst::ITSELF`), but for the faces of the objects it is all the ramp. The bilinear filter is done in the shader after the shading; with `-upscale`, the larger image goes back to the indices that it uses. The lift of the shadows of `terrain_shadow_lift` applies only to the colors of the vertices. They are in **the palette of the game** (PALETTE.IFF), index 255 transparent. Caution: the palette of each `RSImage` is the palette of `SCRenderer`, which stays empty until `SCRenderer::init` is called. Thus the bridge does not use it.
- **The overlays** (the runways and their ground, `objectOverlay`): as `renderMapOverlay`.
  - One mesh for each overlay, its triangles in the order of `drawList`, drawn after the terrain (`drawOrder` 1) and without depth write. Thus each triangle covers the triangles before it, as the `GL_LEQUAL` of the game, with no offset.
  - A triangle with the color 255 takes its palette color; the others take the image `GetImageByID(color + 255)`, with the UVs / 128. Their transparent pixels take the palette color of the triangle, as with `GL_DECAL`.
  - The images are in an atlas, with repeated edges (the `GL_CLAMP_TO_EDGE` of the game).
- **The forests** (`forestChunks`, `plantForest` and `treeModel` in R4World). RSArea has no trees: the game paints them only in some textures (217 on TURKEY). The bridge makes trees on the green.
  Two sources, to select (menu **Forest Source**, `-woods 0|1`):
  - **Green Terrain + Noise** (`ForestMode::NOISE`, by default): the green of the terrain and a noise, described below.
  - **Woods of the Textures** (`ForestMode::TEXTURES`): a tree on the woods painted in the textures of the game, as for the buildings.
    - **Tree texels**: dark green, and not a flat area (their neighbors change), because the drawn trees are shaded, but a flat area of jungle has one color.
    - **Textures kept**: those with at least 8 % of tree texels, that is, the woods, the orchards, the groves of the villages and of the parks, and the bushes of the craters. The rows of the fields (232, 233) are not kept.
    - **Density**: one tree for each 400 of surface, the crowns touch. The trees are placed through the UVs of each triangle.
    - **Comparison**:

      | Map | Noise | Textures |
      |---|---|---|
      | NICARAGA | 11.2 M trees, 2.4 s | 3.8 M trees, 0.5 s |
      | TURKEY | 2.1 M trees, 1.2 s | 70,000 trees, 0.3 s |
      | MAURITAN | 2.5 M trees, 1.4 s | 139,000 trees, 0.4 s |

  The rest below applies to the two sources (models, graph, load).
  - **Where**: the LVL0 triangles of type 3 (the green ramp of the palette) or of type 4 (the jungle, dark green ramp from 128), with no water vertex. The type is the type of the highest vertex, as in `RSArea`. The cells of the city and of the harbor (224, 225, 229, 231) stay empty.
  - **The jungle** (NICARAGA): woods on the largest part, but less dense (×0.4), and almost only broadleaf trees, in the dark green ramp of their ground.
  - **Density**: a noise of some km makes the woods, up to 750 trees/km² at their center, and single trees between them. Below the wood textures (216 and 217), the density is fixed. There is no tree near the runways and near the map and mission objects.
  - **The same trees at each load**: the random values come from the cell.
  - **The models**: two low poly species, with flat faces, in the ramps of the palette (PaletteRamp):
    - a conifer, more frequent at altitude;
    - a broadleaf tree.

    Each has a detailed version up to 1,500 and a simple version (pyramid, octahedron) up to 6,000.
  - **The graph**: the map is cut into squares of 5 km, each a `LOD` node cut beyond 6,000 with an `Instancer` for each species. The renderer culls each tree and selects its level; the trees cast their shadow.
  - **All at the load**, in compact R4nkine `Placement` values (24 bytes for each tree):
    - TURKEY: approximately 2.1 million trees, 50 MB, planted in approximately 1 s;
    - NICARAGA: approximately 11 million trees, 270 MB, planted in approximately 1.5 s.

    Only the trees that the renderer visits get a matrix. During the flight, their `LOD` node removes in one block the squares out of the view or beyond 6 km.
- **The cities, villages, farms and harbors** (`cityModel` in R4World; **Buildings** layer of the viewer). The textures of the game (TXMPACK) are the same on all the maps. On each cell whose texture shows buildings, buildings are put on what the image shows:
  - **found by hand** on the images (`kBuildings`, 143 buildings):
    - the two city centers (229, 230);
    - the harbor (223 to 228, in parts that each cover only half of a cell);
    - the farm (233);
    - the villages on grass (248, 249) and on sand (250, 251); the last are on TURKEY;
  - **found in the image**: the houses (231), by their light grey roofs. The houses drawn only as an outline stay flat.

  A texel of the image goes to the ground through the UVs of each triangle, those that `RSArea` gives: the full image on the cell, or half of an image on a triangle for the 64 × 64 textures of the harbor.

  **Rendering**:
  - **roof**: the image itself, thus from above the map stays the texture;
  - **walls**: facades made by the bridge (offices with windows, houses, cladding of the hangars), on floors of 8;
  - **lighting**: by the sun as the objects, and they cast their shadow.

  **No building on a map object or on a mission object**: a building whose footprint touches the radius of an object is not put. For example, the two silos of a village of TURKEY leave the place to the map object at this position.

  One node for each cell ("city x,y"). NICARAGA has approximately 2,200, TURKEY 136. `-diag` also writes all the textures of the game (`all_<id>.ppm`), used or not.
- **Not done yet**: the sky, the clouds and the fog.

The placement of the parts follows `SCMission::loadMission` and `SCMissionScene::spawnCast`:
- **The player**: its position plus the position of the area of the mission that it names.
- **The other parts** appear with the first scene whose `cast` contains them (the `cast` gives the index of the part in the list):
  - with `placement` 0, they are placed in their area (position plus `areas[area_id]->position`);
  - with `placement` 1, the game places them relative to the player when the scene is activated (`placeRelativeToPlayer`). The viewer puts them at the center of the area of the scene, at an altitude of 4000 for a plane (`EntityType::jet`).
- **Then**, a part below the ground is put on it (`RSArea::getY`), and a plane less than 10 above the ground also.
- **A part placed in the axes of the world** (`area_id` 255, `placement` 0) stays as it is.

## Tested

- Missions mode: tested with `TURKEY.PAK` on the worlds TURKEY and TURKEY22:
  - 257,762 terrain triangles, 22 textures and 44 map objects;
  - approximately 0.3 s of load;
  - the objects are on the terrain.

  No mission of TOBIGTRE.TRE uses Turkey.
- With `BIGTRE.TRE` (Strike Commander, terrain textures from TOBIGTRE.TRE), `MAURITAN.PAK` and `TURKEY.PAK`, there are 11 missions and worlds: MISN-1A to 2A, 12A/B, 14A/B, MAURITAN, TURKEY and TURKEY2.
  - MISN-1A: 31 map objects and 9 parts.
  - Nouakchott, with its city center, its houses and its harbor, and 28 farms around it; 3,074 buildings in all.
  - 2.5 million trees, planted in approximately 1.4 s.
  - The forests are in squares of 2 km grouped by block: the cull goes from 23 to 5 ms on this map.
- With `SYRIA.PAK`, MISN-A1 (the first mission of Tactical Operations) shows:
  - the two runways (236 overlay triangles);
  - the 41 parts in 10 scenes, plus 8 with no scene;
  - the attack base (scene 0) and the training base (scene 6), on the ground near their runways.
- Tested on Linux (GCC 13) with a copy of libRealSpace and R4nkine `6af0d4b`. The data came from `TOBIGTRE.TRE`.
- 342 objects listed. Rendering checked on F-16DES, A-10 (canopy), 747, CARRIERW and T-72.
- R4nkine must be at least at `6af0d4b` (the `AlphaMode::ADDITIVE` mode, added for the canopies).
- The objects with no geometry (the `20MM` shells...) are empty, which is normal.
- T-72: the bands behind the tracks are the `TRAX` faces (property 9, `flags[1] = 255`), marks on the ground that `SCRenderer` also draws. To hide them, `flags[1] == 255` is probably what identifies them.
