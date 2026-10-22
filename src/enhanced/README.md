# StrikeCommanderEnhanced

Optional extensions of Strike Commander that live outside the original classes. Everything here derives from the
original code (`EnhancedMainMenu : SCMainMenu`, `EnhancedGameFlow`, `EnhancedStrike`, `EnhancedMission`, `EnhancedEntity`, ...) and is only active in the
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
