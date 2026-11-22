# R4nkine dans libRealSpace : r4bridge, r4objviewer et neosc_r4

Ce code, sur la branche `r4nkine` de libRealSpace, branche **R4nkine** sur **libRealSpace** sans rien relire.
- libRealSpace lit les données, comme aujourd'hui :
  - `AssetManager` ouvre les TRE ;
  - `RSPalette` lit la palette ;
  - `RSEntity` lit un objet 3D.
- **r4bridge** convertit un `RSEntity` en modèle R4nkine (`r4::assets::Model`).
- **R4nkine** le charge sur le GPU et le dessine (OpenGL 3.3 core, ou 4.1 sur macOS).

```
TOBIGTRE.TRE ──AssetManager──▶ TreEntry ──RSEntity::InitFromRAM──▶ RSEntity
                                                               │
                                     r4bridge::toModel(entity, palette)
                                                               ▼
                       r4::assets::Model (images, matériaux, meshes, nœuds)
                                                               │
                                          renderer.upload(model)
                                                               ▼
                       r4::scene::Node ──renderer.render(root, camera)──▶ l'écran
```

## Contenu

| Fichier | Rôle |
|---|---|
| `src/r4bridge/R4Bridge.h/.cpp` | L'adaptateur `RSEntity` vers `r4::assets::Model`, et la table des teintes de la palette |
| `src/r4bridge/R4World.h/.cpp` | Le terrain d'une zone (`RSArea`) et ses overlays (les pistes) en modèles R4nkine, et les matrices des objets de la carte (`MapObject`) et de la mission (`MISN_PART`), calculées comme `SCRenderer` |
| `src/r4bridge/CMakeLists.txt` | La bibliothèque statique `r4bridge` |
| `src/executables/r4objviewer/main.cpp` | Le viewer : liste des objets ou des missions, caméra, GUI, post-process |
| `src/executables/r4objviewer/WorldScene.h/.cpp` | Une mission en scène R4nkine : `RSMission`, `RSWorld` et `RSArea` chargés par libRealSpace, le terrain, les objets de la carte et de la mission |
| `src/executables/r4objviewer/TerrainDiag.h/.cpp` | Le diagnostic d'un terrain (`-diag`) : ses types et ses textures en images |
| `src/executables/r4objviewer/CMakeLists.txt` | L'exécutable, lié comme le debugger |
| `src/r4game/R4Screen.h/.cpp` | La fenêtre du jeu sur R4nkine : contexte GL, device, post-process, l'image où le jeu dessine, l'allumage de la TV |
| `src/r4game/R4VGA.h/.cpp` | L'écran 320 x 200 (menus, conversations, films, cockpit 2D) affiché par les overlays de R4nkine, par-dessus ce que le jeu a dessiné |
| `src/r4game/R4Loader.h/.cpp` | L'écran de chargement affiché par un overlay de R4nkine |
| `src/r4game/R4Renderer.h/.cpp` | Le rendu de libRealSpace sur R4nkine, sans aucun appel GL : sa pile de matrices, les modèles et les effets du jeu gardés puis dessinés en une passe, le ciel, la caméra de cible |
| `src/r4game/R4Panel.h/.cpp` | Le panneau de R4nkine sur le jeu (F11) : performance et réglages du rendu |
| `src/r4game/R4ConfigPanel.h/.cpp` | Le panneau de configuration (Ctrl+F11) : les options de `config.ini` et les entrées des actions |
| `src/r4game/R4Timer.h/.cpp` | Le temps du jeu : la durée de chaque image mesurée, à toutes les fréquences |
| `src/r4game/R4Mission.h/.cpp` | La mission sur le graphe de scène : le graphe de la mission (terrain, aires, scènes, acteurs, déclencheurs) et son APP |
| `src/r4game/R4Strike.h/.cpp` | La vue du vol : l'affichage, l'ordre des passes, les touches et les caméras ; elle lance l'APP de la mission et la dessine |
| `src/executables/neosc_r4/main.cpp` | `neosc_r4` : le `main` de neosc avec `R4Screen`, `R4VGA`, `R4Loader`, `R4Renderer`, `R4Strike` et `R4Timer` |
| `cmake/R4nkine.cmake` | Ajoute R4nkine (dossier local ou FetchContent), r4bridge, r4objviewer, r4game et neosc_r4 |

## Construire

Tout est déjà dans la branche `r4nkine` de libRealSpace : `include(cmake/R4nkine.cmake)` dans le `CMakeLists.txt` principal, et `glm`, `stb` et `cgltf` dans `vcpkg.json` (R4nkine n'a besoin que de leurs en-têtes).

1. **Configurer.** Il y a deux façons d'obtenir R4nkine.
   - Depuis un clone local (recommandé pour modifier les deux à la fois) :
     ```
     cmake --preset <ton preset> -DR4NKINE_DIR=/chemin/vers/r4nkine
     ```
   - Sans `R4NKINE_DIR` : CMake le télécharge depuis GitHub (FetchContent), branche `R4NKINE_TAG`. Par défaut c'est `claude/magical-bell-y9meqd`. Pour prendre les nouveaux commits de R4nkine, relancer `cmake .` dans le dossier de build.

   Avec `-DREALSPACE_R4=OFF`, rien de tout cela n'est construit.

2. **Lancer** :
   ```
   r4objviewer -data /chemin/vers/les/TRE [-object F-16DES] [-tre TOBIGTRE.TRE] [-textures]
   r4objviewer -data /chemin/vers/les/TRE -mission MISN-A1 [-focus TOWB24] [-distance 2500] [-graph]
   ```
   - **Les jeux** : sans `-tre`, le viewer ouvre chaque jeu présent **seul**, car les deux ont des fichiers différents sous les mêmes noms (301 objets, la palette, les polices) :
     - `BIGTRE.TRE` pour Strike Commander. Ses textures du terrain (TXMPACK, ACCPACK) sont dans `TEXTURES.TRE`. Sans ce fichier, celles de `TOBIGTRE.TRE` les remplacent : il est ouvert en second et ne fournit que les fichiers absents de BIGTRE. Ses objets et ses missions ne sont pas listés.
     - `TOBIGTRE.TRE` pour Tactical Operations.
     - Sans aucun des deux, `OBJECTS.TRE` et `MISC.TRE`.

     Le menu **Game** en haut à gauche passe d'un jeu à l'autre et refait tout (palette, objets, missions). La fenêtre reste ouverte. `-game sc|to` choisit le jeu au départ ; avec `-mission`, c'est le jeu qui a cette mission qui est ouvert. Sans les textures du terrain, une mission est refusée avec un message au lieu de planter RSArea.
   - En haut à gauche, **Mode** : *Objects* (un objet à la fois) ou *Missions*.
   - Mode **Missions** :
     - la liste ne montre que les missions et les mondes dont le terrain (`<tera>.PAK`) est dans le dossier `-data` ;
     - un monde seul, sans mission, montre son terrain et les objets de la carte ;
     - à droite, la section **World** :
       - couches *Terrain*, *Runways*, *Map Objects*, *Mission Parts* et *Forests* ;
       - **Forest Density** (`-forests 0..3`) : aucune, clairsemée, normale ou dense ;
       - niveau de détail du terrain ;
       - chiffres : triangles, textures, objets, objets absents du TRE, lumière du monde, temps de chargement ;
     - le graphe : sous « parts of the mission », un nœud par zone de la mission (entrer dans une zone déclenche ses scènes), puis les scènes de cette zone ;
     - la section **Scenes** : les parts groupées par la scène qui les fait apparaître ; chaque scène s'affiche ou se cache, et « No scene » (les parts amenées par un programme de la mission) est cachée au départ ;
     - les sections **Parts** et **Map Objects** : un clic met la caméra sur l'objet ;
     - `-focus NOM` fait de même au démarrage, `-at X,Y` met la caméra sur un point de la carte (axes de R4nkine), et `-distance D` règle le recul de la caméra.
   - En haut à droite, **Scene Graph** (ou `-graph`), en haut du panneau de droite : le graphe de la scène affichée (objet ou mission) s'affiche sous ce bouton. On y voit le type de chaque nœud, un voyant pour le cacher ou le montrer, et un clic sur un nœud y amène la caméra.
   - Panneau de gauche en mode Objects, **Objects** : la liste des objets des TRE. Un clic choisit un objet, la molette fait défiler la liste, et il y a des boutons Précédent et Suivant.
   - Panneau de droite :
     - **Render** : fil de fer, textures, facteur de LOD, lumière, ciel, angles de la caméra ;
     - **Post Process** : MSAA, FXAA, scanlines, palette… ;
     - **Performance** : FPS, courbes des temps (image, cull, draw), draw calls, triangles, changements d'état, items et nœuds ;
     - **Object** : sommets, faces, textures, les LOD avec leurs distances (celui affiché est marqué), le temps de chargement et de conversion, les meshes R4nkine ;
     - **Textures** : chaque image de l'objet, avec sa taille et son nombre de frames. `-textures` ouvre cette section au départ.
   - Souris : bouton gauche pour tourner, molette pour le zoom, bouton droit pour déplacer.
   - Clavier : flèches haut et bas pour l'objet précédent ou suivant, Échap pour quitter.
   - Dans **Render** :
     - **Flat Shading** (ou `-flat`) : les objets prennent la normale de leur face, comme `object_gouraud = false` dans le jeu, et le terrain est recalculé par `RSArea` avec `terrain_flat` ;
     - **Gouraud by Vertex** (ou `-gouraud`) : l'éclairage des objets est calculé aux sommets puis interpolé, comme le jeu (le dos d'une face double prend la normale de la face). Sinon, il est calculé à chaque pixel.
     - **Texture Upscale** (`-upscale 0..3`) : les textures du jeu agrandies une seule fois, à leur chargement, en 2xSaI, xBR 2x ou xBR 4x (`r4/assets/upscale.h`). Cela concerne les objets, le terrain et les pistes ; dans l'atlas des pistes, chaque image est agrandie avant d'y être rangée. L'alpha compte comme une couleur, donc les trous (index 255) gardent des bords nets. xBR 4x ajoute environ 1 s au chargement de TURKEY.
     - **Mipmaps** (`-mipmaps`) : avec leurs mipmaps. Le jeu n'en a pas ; avec, les bordures des cellules de l'atlas des pistes passent de 2 à 8 texels. Les textures du terrain n'en ont pas : ce sont des index de palette, filtrés dans le shader après leur ombrage.
     - **Anisotropic Filter** (`-aniso N`, de 2 à 16) : le filtrage anisotrope, quand le GPU le permet.

       Ces trois réglages refont l'objet ou le monde affiché (`r4bridge::textureQuality()`, `finishTexture`). Par défaut, tout est comme le jeu.

     Les deux se règlent au départ d'après le `config.ini` du jeu s'il est là.
   - **Sun** (les deux modes) :
     - **Sun Heading / Sun Elevation** : la direction du soleil (`-sun C E`), qui éclaire les objets ;
     - **World's Light** : revient à la lumière du monde, `RSWorld::light_direction` ;
     - **Lit Terrain** (`-lit`) : le terrain éclairé par le soleil plutôt que par la lumière des données. Chaque triangle prend le ton le plus clair de la rampe de son type de terrain (`RSArea::terrainTone`), assombri le long de cette rampe par l'intensité, comme les objets (PaletteRamp : ambiant + lumière × N·L). Les normales sont calculées à partir du relief, sur toute la carte ;
     - **Relief of the Light** (`-relief 0..4`) : multiplie le relief pour les normales seulement (×1 à ×16, ×4 par défaut). Le relief des cartes est doux, avec un sommet tous les 1 000, alors que la lumière des données l'exagère beaucoup.
     - **Cast Shadows** (`-shadows`) : les ombres portées du soleil. Une *shadow map* (2048², vue orthographique depuis le soleil) couvre la vue jusqu'à **Shadow Distance** (Auto : 3 fois la distance de la caméra), avec un PCF 3×3 et un fondu au bord. Les objets jusqu'à 8 km vers le soleil y projettent leur ombre, par exemple un avion haut au-dessus du sol. Les objets éclairés perdent la lumière directe et gardent l'ambiante. Ce qui n'est pas éclairé, comme le terrain de la lumière des données ou les pistes, est assombri de **Shadow on Unlit**.
   - **Navigation**, dans une mission :
     - les touches à leur place sur le clavier (WASD en QWERTY, ZQSD en AZERTY) et les flèches pour avancer et se décaler, Q/E (A/E en AZERTY) pour tourner, R/F pour se rapprocher ou s'éloigner, Maj pour aller plus vite ;
     - **double clic** sur le terrain : la caméra y va (rayon lancé contre `RSArea::getY`) ;
     - **Follow the Ground** : la cible reste sur le sol ;
     - les boutons **Whole Map**, **Top View** et un par zone de la mission.
   - `-frames N -shot image.bmp` fait une capture après N images, utile pour les tests.
   - `-mission NOM -diag DOSSIER` écrit le diagnostic du terrain dans DOSSIER, qui doit exister. Tout est au niveau LVL0, 360 × 360 sommets, avec le nord en haut :
     - `terrain.ppm` : les couleurs de RSArea ;
     - `types.ppm` : le type de chaque sommet, dans le ton le plus clair de sa rampe ;
     - `textures.ppm` : les triangles texturés, chacun dans la couleur moyenne de son image ;
     - `tex_<id>.ppm` : chaque image utilisée, avec les texels clairs en magenta ;
     - `types.txt` : le nombre de sommets par type ;
     - `textures.txt` : par image, le nombre de triangles, les types sur lesquels elle est posée et ses premières cellules.

     Avec `-frames 1`, le viewer se ferme ensuite.

## neosc_r4 : le jeu sur R4nkine

`neosc_r4` est neosc dont l'affichage passe par R4nkine. Le travail se fait par étapes :

| Étape | Contenu | État |
|---|---|---|
| 1 | `R4Screen`, `R4VGA`, `R4Loader` : la fenêtre, le post-process, l'écran 320 x 200 et le chargement | **fait** |
| 2 | `R4Strike` : le vol sur le graphe de scène (terrain, avions, objets, armes), mis à jour par l'APP | **fait** |
| 3 | Tout le reste par R4nkine : effets, nuages, ciel, caméra de cible, cockpit 3D, visionneuse d'objets | **fait** |
| 4 | Le contexte core (3.3, 4.1 sous macOS) : le jeu ne fait plus aucun appel GL | **fait** |
| 5 | `R4Mission` : le graphe appartient à la mission (aires, scènes, déclencheurs) ; l'APP passe par les callbacks des nœuds | **fait** |
| 6 | La performance : nuages faits une fois par zone, forêts en instancers statiques, `R4Timer` | **fait** |
| 7 | `R4ConfigPanel` (Ctrl+F11) : les options de `config.ini`, les touches, les manettes et les joysticks | **fait** |

**Les points d'accroche dans libRealSpace.** Ces changements de libRealSpace (autrefois le patch `r4-hooks.patch`) ne changent pas le comportement de neosc :
- `RSVGA` : `displayBuffer`, `init`, `activate` et le destructeur sont virtuels ; la création des textures GL est dans `initTextures()`, virtuelle ; les membres privés sont protégés ;
- `Loader` : l'affichage de la surface de chargement est dans `presentSurface()`, virtuelle ; le destructeur est virtuel ;
- `SCStrike` : le dessin d'une vue est dans `renderScene`, `renderWorld` et `renderPlayerPlane`, virtuelles ; `SCStrike::create()` et `setFactory()` choisissent le type des vols (`SCGameFlow::flyMission` et `SCTrainingMenu::OnSearchAndDestroy` passent par `create()`) ; `createMission`, virtuelle, fait la mission du vol ;
- `SCMission` : le destructeur est virtuel ; `SCMissionEvent` a `MissionEventAreaEntered` et `MissionEventAreaLeft` ;
- `RSArea::ground` : une fonction qui donne la hauteur du sol à `getY` ; `R4Mission` y met `r4bridge::groundHeight` ;
- `SCPlane` : `getWheelIndex()`, `ground_clearance` (la hauteur des roues sous l'origine du modèle) et `SetNormalizedStick` ; `SCSimulatedObject::smokeSet()` ;
- `SCRenderer` : `pushMatrix`, `popMatrix`, `translateMatrix`, `rotateMatrix`, `scaleMatrix`, `clearDepth` et `readRenderToTexture`, virtuelles (par défaut les appels GL d'avant) ; `SCVirtualCockpit::render`, `SCObjectViewer::runFrame` et `SCCockpit::RenderMFDSCamera` passent par elles au lieu d'appeler GL ;
- les entrées : les actions d'axes `AXIS_*` et `bindFlightAxes` (`GameEngine`), le GUID des appareils, la zone morte et la courbe de chaque axe (`InputActionSystem`, `EventManager`) ;
- `Config` : l'écriture de `config.ini`.

**L'étape 1.**
- `R4Screen` ouvre la fenêtre et le contexte OpenGL (voir l'étape 4).
- Le post-process de libRealSpace (`fx_cpc_palette`, `fx_scanlines`, `fx_pixel_scale`, `fx_fxaa` de `[video]`, et `msaa` de `[Video]`) passe par celui de R4nkine. Quand un effet est actif, le jeu dessine dans la cible du post-process (`Device::bindForHost`), puis `refresh()` applique les effets dans la fenêtre. La palette CPC compte 27 couleurs (0, 128 ou 255 par canal).
- `R4VGA` reçoit l'image 320 x 200 déjà passée par la palette, les fondus, le contraste et la teinte (`displayBuffer`). Il la montre en 4:3 au centre de la fenêtre avec un overlay, net ou filtré avec `super_eagle_2x`, et les bandes latérales de l'écran large (`widescreen_ambilight`). L'index 255 reste transparent : le cockpit 2D se pose sur la 3D.
- `R4Loader` montre la surface de chargement en 4:3, ses couleurs converties depuis le format de la surface SDL.
- Testé ici sans les données complètes du jeu (SC.DAT, LILTRE.TRE et VOCLIST.TRE manquent) : un programme de test lance les vraies classes. L'allumage de la TV s'affiche, puis l'écran 320 x 200 sur une scène dessinée en pipeline fixe, sa zone transparente laisse voir la scène, et l'état GL du jeu est intact après chaque passage de R4nkine. Avec scanlines, pixels x 2 et MSAA 4x, l'image finale porte les effets.

**L'étape 2, puis l'étape 5 : le vol sur le graphe de scène.**
- `R4Strike` remplace `SCStrike` pour tous les vols : campagne, entraînement, Tactical Operations. `neosc_r4` lui donne la fabrique `SCStrike::create()` (`SCStrike::setFactory`).
- `SCMission` charge et fait tourner la mission. Le graphe appartient à `R4Mission : SCMission`, que fait `R4Strike::createMission`. Il est fait au chargement de la mission et vit autant qu'elle, en axes de libRealSpace (Y en haut) :
  - `terrain` (masque `LAYER_TERRAIN`) : le terrain de `r4bridge::terrainModel` (quadtree, anneaux, atlas) et les pistes de `overlayModel`, faits une fois par zone ;
  - les avions (masque `LAYER_ACTORS`, `LAYER_PLAYER` pour celui du joueur) : dans un seul groupe, car un avion passe d'une aire à l'autre ; chacun a son corps, ses roues, sa postcombustion et ses armes sous les points d'emport ;
  - un nœud par aire de la mission, avec les objets qui y sont ; ceux d'une scène sont sous un `Switch` de cette scène, allumé quand la scène est activée ;
  - `simulated objects` (masque `LAYER_SHOTS`) : un nœud par missile, bombe ou obus avec un modèle ;
  - un déclencheur par aire (masque `LAYER_TRIGGERS`), avec son volume (sphère, tube ou boîte) : il met `MissionEventAreaEntered` et `MissionEventAreaLeft` des avions sur le bus de messages.
- Chaque acteur a un nœud `LOD` à la distance de rendu de `SCStrike` (`RENDER_DISTANCE`).
- Les modèles des `RSEntity` sont faits par `r4bridge::toModel` au chargement de la mission et partagés entre les nœuds.
- **APP** (`R4Mission::app`, lancé par `R4Strike` une fois par image, après le placement de la caméra) : il crée les nœuds de ce qui apparaît et retire ceux de ce qui disparaît. Puis `r4::scene::app` lance le callback de chaque nœud, qui prend l'état de son objet : matrice, roues, postcombustion, emports, modèle, avec les mêmes calculs que `SCPlane::Render` et `SCRenderer::drawModel`. Enfin, les déclencheurs comparent la place des avions à leurs volumes.
- **Le sol** : `RSArea::ground` donne à `getY` la hauteur de `r4bridge::groundHeight`, sur les triangles LVL0 que le renderer dessine. Les avions au sol sont posés sur leurs roues (`ground_clearance`).
- **CULL et DRAW** (`R4Strike::renderWorld`) : le graphe de la mission entre dans la passe de `R4Renderer` (`addScene`), avec le ciel, les effets du jeu et le cockpit 3D. Une seule passe du renderer dessine la vue, avec la caméra du jeu.
- Le vol avec les boîtes de debug (`show_bbox`) reprend le dessin de `SCStrike`, par `R4Renderer`.
- `RenderOptions::clear` à faux : une passe du renderer garde la couleur et la profondeur de la cible (le ciel du jeu).
- Testé ici avec un programme de test sur MISN-1A (avant `R4Mission`, quand `R4Strike` gardait le graphe) : la vue cockpit (cockpit 2D sur la 3D) et la vue extérieure (F-16 du joueur avec ses emports, un autre avion, la piste) donnent la même image avec `SCRenderer` et `SCStrike` qu'avec `R4Strike` et son graphe. Avec scanlines et pixels x 2, la 3D passe par la cible du post-process.

**L'étape 3 : tout le rendu par R4nkine.** `R4Renderer` remplace toutes les méthodes de `SCRenderer` qui dessinent, et n'appelle plus GL.
- **Matrices** : une pile de matrices propre (`pushMatrix`, `translateMatrix`, `rotateMatrix`, `scaleMatrix`, `popMatrix`) à la place de celle de GL. Tous les `drawModel` (position, orientation, ajustement, échelle, `drawModelWithChilds`, `renderObjects`, `renderMissionObjects`) calculent leur matrice comme `SCRenderer` (`glRotatef` en degrés ou `Matrix::rotateM` en radians).
- **Caméra** : `bindCameraProjectionAndViewViewport` et `initRenderCameraView` donnent la vue, la projection (avec le décalage vertical) et le viewport de la passe, depuis la `Camera` du jeu.
- **Passe** : les modèles, les effets, le ciel et les graphes (`addScene`) sont gardés, puis dessinés en un seul `render()` : avant un changement de vue, avant l'écran 320 x 200 (`R4Screen::beginGamePass`) et à la fin de l'image (`refresh`).
- **Effets** : billboards, sprites, particules, traçantes, triangles d'effets (`drawTriangles`, additifs ou alpha), quads texturés (panneaux du cockpit 3D), nuages, lignes, boîtes et points deviennent des triangles en monde, regroupés par état (texture, mélange, test et écriture de profondeur, test alpha, bac). Un maillage par groupe et par passe.
- **Ordre** : le ciel (bac -2, sans profondeur, hors du brouillard) puis les nuages (bac -1) avant tout, comme le dôme et les nuages de `renderWorldSolid` ; les points et les lignes en dernier, sans profondeur, comme le jeu les dessine après les modèles.
- **Nuages** : faits une seule fois par zone dans le graphe (`makeClouds`, à l'appel de `useArea`), un Geode par nuage avec toutes ses bouffées dans un seul maillage, éliminé par ses bornes. Avant, chaque image refaisait sur le CPU et renvoyait au GPU jusqu'à 500 nuages de 7 à 11 bouffées de 432 sommets. Les bouffées sont blanches : leur mélange donne la même couleur dans tous les ordres, donc il n'y a plus de tri par la distance.
- **Textures du jeu** : les `Texture` (billboards, panneaux du cockpit 3D) et les textures de `createTexture` sont envoyées à R4nkine ; `uploadTextureContentToGPU` les met à jour.
- **Caméra de cible** : sa vue a sa propre cible de rendu de 107 x 75. `getRenderToTexture` la relit, et le cockpit la lit par `readRenderToTexture`. Le problème du MSAA de l'étape 1 disparaît.
- **R4nkine** : `Material::depthTest` et `Material::fog` (le ciel), les bacs de fond (`drawOrder` négatif, dessinés avant tout, mélangés ou non).
- Testé ici, avec la même scène par `SCRenderer` (contexte compatibilité) et par `R4Renderer` (contexte core) : terrain, modèles, billboard, sprite, particule, triangles additifs et alpha, traçante, quad texturé, ligne, point, boîtes, nuage, et l'image de la caméra de cible. Les images sont les mêmes. Différence voulue : les sprites et les particules n'écrivent plus la profondeur et gardent le test de profondeur ; dans le jeu, ils le perdaient par un état GL laissé par `drawModel`.

**L'étape 4 : le contexte core.**
- `R4Screen` demande un contexte OpenGL 3.3 core (4.1 core avec forward compatible sous macOS). La clé `gl_compatibility` de `[Video]` demande un contexte compatibilité, seulement pour comparer avec `SCRenderer`.
- Le jeu ne fait plus aucun appel GL en vol, dans les menus et dans la visionneuse d'objets. Seul l'affichage VR de `SCStrike` (`drawTargetSquareOverlay`) appelle encore GL : il sera repris avec OpenXR.
- R4nkine : `createShader` laissait une erreur GL après une interop (programme inconnu restauré). Corrigé.
- Testé ici en contexte core avec la sortie de debug de GL : MISN-1A en vue cockpit, extérieure et cible, 200 images chacune, la visionneuse d'objets, et la scène des effets : aucune erreur GL. macOS n'est pas testé ici.


**Les options du rendu (`config.ini`, section `[R4nkine]`).** `R4Renderer` les lit à son démarrage. Sans elles, l'image reste celle du jeu.

```ini
[R4nkine]
; ombres portées : une carte d'ombre depuis la lumière des objets du jeu
shadows = false
shadow_map_size = 2048
; en mètres : la zone couverte devant la caméra
shadow_distance = 1500
; les arbres projettent aussi leur ombre (coûteux)
tree_shadows = false
; off, noise (bois et clairières sur le vert du terrain) ou textures (les arbres peints dans les textures)
forests = off
; multiplie toutes les densités
forest_density = 1.0
; en km (multipliés par lod_scale) : les arbres détaillés, puis les arbres simples, éclaircis au-delà des arbres détaillés
tree_detail_km = 1.5
tree_km = 6
; les bâtiments sur les textures de villes, villages, fermes et ports
buildings = false
; none, 2xsai, xbr2x ou xbr4x : agrandit les textures des objets et du terrain
texture_upscale = none
texture_mipmaps = false
; filtre anisotrope, de 1 à 16
anisotropy = 1
; le terrain éclairé par le soleil le long de ses rampes, comme les objets, plutôt que par la lumière des données (-lit de r4objviewer)
sun_terrain = false
; multiplie le relief pour les normales seulement, de 1 à 16 (-relief)
terrain_relief = 4
; la part de leur couleur que perd dans l'ombre ce qui n'est pas éclairé : terrain des données, pistes (Shadow on Unlit)
shadow_strength = 0.5
; le soleil, en degrés : cap depuis le nord vers l'est, hauteur (-sun C E) ; sans ces clés, la lumière des objets du jeu
; sun_heading = 120
; sun_elevation = 30
; la lumière des objets (ambiante + directe x N.L) ; sans ces clés, celle du jeu (Ambient, Light)
; light_ambient = 0.375
; light_direct = 0.625
; le brouillard en km depuis l'œil ; sans ces clés, celui du jeu (0,40 et 0,93 x max_view_distance) ; show_fog de [Game] l'active
; fog_start = 64
; fog_end = 149
; Wireframe, Flat Shading
wireframe = false
flat_shading = false
; multiplie les distances des LOD du terrain, des objets et des arbres (LOD Scale, 0,05 à 4)
lod_scale = 1
; distance (par la distance), fine, medium ou coarse (Terrain Detail)
terrain_lod = distance
vsync = true
; le dôme du jeu (ciel et sol) ; sans lui, le fond du monde est sky_color, comme dans r4objviewer, et le brouillard prend cette couleur
sky = true
sky_color = 0.30, 0.42, 0.60

; le post-process de r4objviewer ; chaque clé présente remplace le réglage du jeu (fx_* de [video], msaa de [Video])
; msaa = 1               ; 1, 2, 4 ou 8
; fxaa = false
; pixelate = 1           ; l'image dessinée comme si elle était N fois moins large
; brightness = 0         ; -1 à 1
; contrast = 1
; saturation = 1         ; 0 : gris
; gamma = 1
; tint = 1, 1, 1         ; r, g, b de 0 à 1, multipliés
; fade = 0               ; 0 à 1 vers fade_color
; fade_color = 0, 0, 0
; palette = none         ; none, vga256, ega16, cga4, handheld4, amber8, grey16 ou cpc27 (Reduce to a Palette)
; dither = true          ; Bayer 4 x 4 avec la palette
; scanlines = false
; scanline_strength = 0.35
; scanline_size = 3      ; pixels par ligne
; curvature = 0          ; 0 à 0,3
; aberration = 0         ; pixels, aux bords
; vignette = 0           ; 0 à 1
```

Les réglages de r4objviewer qui ne sont pas ici : **Textures** et **Gouraud by Vertex** sont `show_texture` et `object_gouraud` de `[Game]`, **Fog** est `show_fog` de `[Game]`. Le mode, la caméra, la navigation, les couches (terrain, pistes, objets, parties), Follow the Ground, On Ground et `-nolift` sont propres au viewer : dans le jeu, c'est lui qui place sa caméra et ses objets.

- Les forêts et les bâtiments viennent de `r4bridge` (`forestChunks`, `plantForest`, `treeModel`, `cityModel`), comme dans r4objviewer. Ils sont faits une fois par zone. Il n'y en a pas sur les pistes, sur les objets de la carte, ni sur les objets de la mission (`R4Mission` donne leur place et leur taille). Un objet de la mission sans sommets n'en écarte rien : sa boîte (`RSEntity::CalcBoundingBox`) reste à `FLT_MAX`, et son rayon infini retirait toutes les forêts et tous les bâtiments de la carte.
- Les forêts sont des blocs de 2 km : un LOD par bloc, un instancer par espèce, l'arbre complet jusqu'à `tree_detail_km` et l'arbre simple jusqu'à `tree_km`. Les instancers sont statiques : leurs arbres vont une seule fois sur le GPU, il n'y a aucun travail par arbre à chaque image, et au-delà des arbres détaillés une partie des arbres est retirée.
- Le soleil (`sun_heading`, `sun_elevation`) éclaire les objets et le terrain (`sun_terrain`) et donne la direction des ombres. Sans ces clés, c'est la lumière des objets du jeu (`setObjectLight`), presque verticale : les ombres sont courtes.
- En fil de fer, le dôme du ciel et les nuages sont aussi en lignes : ce sont des maillages, alors que le ciel de r4objviewer est la couleur du fond.
- Les ombres ne sont pas faites dans la vue de la caméra de cible. Sans `tree_shadows`, les forêts (masque `R4Renderer::kForestMask`) sont hors de la passe d'ombre.
- Au chargement d'une zone, la console donne le nombre d'arbres et de bâtiments.
- Mesuré ici sur ARENA (rendu logiciel, donc seulement pour comparer) : chargement de la zone 1,0 s sans option ; `forests=noise` 5,3 s pour 8,7 millions d'arbres et environ 320 Mo de plus ; `forest_density=0.4` 2,8 s pour 3,5 millions ; `forests=textures` 1,8 s pour 3,0 millions ; `buildings=true` 1,1 s pour 6 838 bâtiments.
- Testé ici sans erreur GL, en contexte core : MISN-1A et ARENA avec toutes les options, une vue proche où le F-16 et le char projettent leur ombre sur le terrain, puis le post-process (MSAA 4x, FXAA, palette EGA tramée, scanlines, courbure, vignette, aberration ; luminosité, contraste, saturation, gamma, fondu), le fil de fer, le rendu plat avec une autre lumière, le terrain grossier avec `lod_scale` 0,2 et un brouillard de 0,2 à 1,5 km.
**Le timer (`R4Timer`).** `neosc_r4` remplace `DesktopTimer` par `R4Timer` (`GameTimer::setTimer`, dans le `main`). La physique, l'IA et la mission avancent de `getDeltaTime()` à chaque image. `DesktopTimer` ne donnait pas le temps réel :
- après 90 images, il arrondit la fréquence mesurée à 60 au plus (`snapToStandardFPS`) ;
- il ne suit ensuite la fréquence que si elle reste entre 10 et 200 images par seconde ;
- au-delà de 200 (sans vsync), chaque image compte donc 1/60 s : à 300 images par seconde, le jeu va 5 fois trop vite ;
- il mesure en millisecondes (`SDL_GetTicks64`) : à 3,3 ms par image, chaque mesure vaut 3 ou 4 ms.

`R4Timer` donne la durée mesurée de chaque image avec `SDL_GetPerformanceCounter`, sans lissage. Une image plus longue que 0,1 s (un chargement) compte pour 0,1 s. `shouldSimulate` garde le pas fixe de 1/30 s.

**Le panneau R4nkine (F11).** `R4Panel` (dans `src/r4game`) est le GUI de R4nkine par-dessus l'image du jeu, après le post-process. F11 seul le montre ou le cache (Ctrl+F11 est le panneau de configuration). Il lit la souris et le clavier de SDL à chaque image, et la molette par un `SDL_AddEventWatch` : les événements restent au jeu, donc un clic sur le panneau va aussi au jeu. Tant qu'il est affiché, le curseur est visible et la souris n'est pas relative (le jeu cache le curseur dans sa fenêtre, et ses menus utilisent la souris relative) ; F11 rend ensuite l'état du jeu.
- **Performance** : FPS et temps moyen sur 240 images, courbes de l'image, du cull et du draw, puis, par image et pour toutes les passes du renderer : passes, draw calls, triangles, changements de pipeline et de texture, nœuds visités et éliminés, drawables, instances, items opaques et transparents, objets qui projettent une ombre, temps de l'ombre, du tri et de l'envoi. Le dessin du panneau n'est pas compté.
- **Area** : ce que `useArea` a fait pour la zone (temps, arbres, bâtiments, nuages), et les options de `config.ini` qui ne changent qu'avec une nouvelle zone (forêts, bâtiments, upscale, mipmaps, anisotrope, terrain éclairé, relief, niveau du terrain, lumière).
- **Render** : les réglages que la passe suivante lit, changés en direct : fil de fer, rendu plat, textures, Gouraud, brouillard, nuages, ciel et couleur du fond sans ciel, ombres, ombres des arbres, LOD Scale, distance et force des ombres, soleil et brouillard des réglages, VSync.
- **Post Process** : les contrôles de r4objviewer (`r4::ui::postProcessControls`), en direct.

**Le panneau de configuration (Ctrl+F11).** `R4ConfigPanel` est une fenêtre de R4nkine au centre de l'écran (70 %). Ctrl+F11 ou sa croix la ferme. Tant qu'elle est affichée, le jeu ne reçoit ni touche ni souris : un filtre d'événements de SDL les garde, et remet à zéro l'état du clavier que le jeu lit. Les manettes et les joysticks continuent d'agir : le panneau regarde seulement leurs événements.
- **Window, Game, Video, Render (R4nkine), Sound** : les options de `config.ini`. Chaque option dit quand elle s'applique : tout de suite, à la prochaine mission (*mission*) ou au prochain démarrage (*restart*).
- **Input** : le fichier des touches (`bindings_file` de `[Input]`), puis les actions du vol.
  - **Change** attend une touche (avec Ctrl, Alt ou Maj) ou un bouton de manette ou de joystick. Pour une action d'axe, il attend un axe poussé loin de son repos, dans le sens du nom de l'action : ce sens donne le signe. Échap annule.
  - **Change** remplace les entrées de la même famille (le clavier, ou les manettes et les joysticks), **Add** les garde, **Clear** enlève tout sauf la souris.
  - Sous une action d'axe, chaque axe a sa zone morte (0 à 0,5), sa courbe (exposant de 1 à 3) et **Inverted**.
- **Devices** : les manettes et les joysticks branchés, leur GUID, leurs axes en direct et les boutons enfoncés.
- **Save** écrit `./assets/config.ini` et le fichier des touches.

**Les entrées.** Les axes du vol sont des actions : `AXIS_PITCH`, `AXIS_ROLL`, `AXIS_RUDDER`, `AXIS_THROTTLE`, `AXIS_LOOK_X` et `AXIS_LOOK_Y` (69 à 74 dans `SimActionOfst`).
- Par défaut, quand un axe n'a pas de liaison (`bindFlightAxes`) : le stick gauche pour le roulis et le tangage, le stick droit pour le regard, les gâchettes pour le palonnier. La manette des gaz n'a pas d'axe par défaut.
- Les anciennes actions des sticks (55 à 58) ne servent plus : leurs liaisons sont effacées au chargement.
- Une ligne du fichier des touches : `action,type,deviceId,code,scale,modifiers,guid,deadzone,curve`. Les trois derniers champs sont facultatifs : les anciens fichiers se chargent.
- Avec un GUID (le GUID de SDL, le même d'un lancement et d'un branchement à l'autre), `deviceId` est le rang de l'appareil parmi ceux de ce GUID : 0 pour la première manette d'un modèle, 1 pour la deuxième. Sans GUID, c'est le numéro de la manette ou du joystick.
- Une zone morte de -1 prend celle du système (0,1). Le jeu lit les axes et les boutons directement dans SDL.

## Ce qu'il faut regarder pour apprendre R4nkine

Le viewer suit les étapes d'un programme R4nkine, numérotées dans `main.cpp`.

1. **Le device** : `r4::gpu::createGLDevice(SDL_GL_GetProcAddress)`.
   - C'est la couche la plus basse : buffers, textures, shaders, pipelines.
   - Les fonctions GL sont chargées à l'exécution, donc il n'y a ni GLEW ni GLAD.
2. **Le renderer** : `r4::render::Renderer renderer(*device)`.
   - Il dessine un graphe de scène : cull, tri opaque/transparent, puis draw.
   - `renderer.environment()` règle la lumière et le fond.
   - `renderer.options()` règle les textures, le facteur de LOD et le reste.
3. **Le modèle neutre** : `r4::assets::Model`.
   - Les chargeurs de R4nkine (OBJ, glTF, OpenFlight…) produisent ce modèle, et r4bridge aussi.
   - Il ne contient que des données : `images`, `materials`, `meshes` et `nodes`, avec `nodes[0]` pour la racine.
   - `renderer.upload(model)` en fait un sous-graphe prêt à dessiner.
4. **La caméra** : `r4::render::Orbit` tourne autour d'un point et `orbit.apply(camera, fov, aspect, near, far)` remplit la `Camera`.
5. **La GUI** : `r4::ui::Context`, de l'immédiat comme ImGui mais avec le look Performer.
   - Le code : `beginFrame`, `beginPanel`, les widgets (`button`, `toggle`, `slider`, `section`…), `endPanel`, `endFrame`, puis `render()` après la scène.
   - `r4::ui::postProcessControls()` fournit d'un coup tout le panneau des effets.
6. **Le post-process** : `r4::render::PostProcess`.
   - `post.begin()` donne une cible, le renderer y dessine avec `setTarget`, puis `post.end()` applique les effets : MSAA, FXAA, scanlines, palette…
   - Quand aucun effet n'est actif, `begin()` renvoie une cible nulle et la scène est dessinée directement.

## Comment r4bridge convertit un RSEntity

Le pont suit `SCRenderer::drawModel` passe par passe (couleur, texture, transparence) :

| RealSpace (libRealSpace) | R4nkine |
|---|---|
| Axes : Y en haut (`entity->vertices`) | Z en haut : (x, y, z) devient (x, −z, y), une rotation de 90° |
| Faces dans le sens horaire (`glFrontFace(GL_CW)`) | Ordre inversé (a, c, b) : R4nkine attend le sens anti-horaire |
| Normales `(a − b) × (c − b)`, lissées comme `objectNormals` (un quad compte pour deux triangles) | Même calcul, après le changement d'axes |
| `lods` (LVL0, LVL1…) et `attrs` (T/Q/L) | Un nœud `NodeKind::LOD`, avec un enfant par niveau ; `lodRanges` = [0, dist1, dist2, …, ∞] |
| Passe couleur : triangle `color`, quad `color − 1` | Matériau `Shading::PALETTE_RAMP` ; l'index est dans le rouge de la couleur de sommet |
| Éclairage (ambiant 3/8, lumière 5/8, `shadeAlongRamp`) | Image 256 × 32 (`rampImage`) : ligne k = intensité k/31 |
| Face double : `flags[2] == 1` (triangle), `attrs->props2 == 1` (quad) | `doubleSided`, sinon face arrière éliminée comme dans le jeu |
| Textures (`uvs`, `qmapuvs`, `images`) | Images RGBA à travers la palette, index 255 transparent ; `GL_LINEAR` sans mipmaps et bords bornés (`Image::mipmaps = false`, `clampS/T`), comme `createTextureInGPU` |
| Passe texture : mélange `SRC_ALPHA, ONE_MINUS_SRC_ALPHA` et test alpha `> 0,1` | `AlphaMode::BLEND` avec `alphaTest = 0,1` |
| Propriété 6 ou 9 : la texture est la face (`GL_LESS`, profondeur écrite) | Matériau `rampTextured`, `blendDepthWrite` |
| Autres faces texturées : sur la face colorée (`GL_EQUAL`, profondeur non écrite) | `depthEqual`, `depthWrite = false` : la texture n'apparaît que là où la face colorée est dessinée |
| Textures animées (`nbframes` > 1 : les images les unes sous les autres, `RSImage::GetNextFrame` toutes les 10 images d'affichage) | Matériau `frames` et `framesPerSecond` (6 par seconde, soit 10 images à 60 Hz) : le shader choisit l'image avec `Environment::time`, le filtrage reste dans l'image |
| Passe transparente : propriété 2, couleur `color − 1`, `glBlendFunc(GL_ONE, GL_ONE)` | `AlphaMode::ADDITIVE`, palette sans rampe (couleur × intensité) |

La lumière se règle avec `r4bridge::Lighting` ou avec les curseurs Ambient et Light du viewer.

Pour afficher un objet ailleurs que dans le viewer, par exemple dans un futur `SCRenderer` sur R4nkine :

```cpp
RSEntity *entity = ...;                       // déjà chargé par libRealSpace
auto model = r4bridge::toModel(entity, *palette.GetColorPalette(), palette.ranges);
r4::scene::NodePtr node = renderer.upload(*model);
someGroup->addChild(node);                    // et le renderer s'en occupe
```

## Les missions : un seul changement de repère

Une mission reste entièrement dans le repère de libRealSpace (Y en haut) :
- le terrain : `terrainModel` reprend les sommets de `RSArea` tels quels ;
- les objets : `toModel(..., Axes::REAL_SPACE)` ;
- leurs matrices : `mapObjectMatrix` et `missionPartMatrix` reprennent les `glTranslate`, `glRotate` et `glMultMatrix` de `SCRenderer`.

Une seule matrice, `r4bridge::realSpaceToR4()`, posée à la racine de la scène, passe dans le repère de R4nkine (Z en haut). C'est la frontière unique entre les deux conventions : physique, IA et mission n'ont rien à convertir.

Pour la lumière du monde, `setObjectLight` reçoit (c0, c1, c2) et en fait (c0, c2, −c1) en Y up. Ramenée dans le repère de R4nkine, elle redevient (c0, c1, c2), c'est-à-dire les valeurs du fichier telles quelles.

Le terrain (`WorldScene` et `R4World`) :
- **Un nœud par bloc** : un `Group` « terrain » contient les 324 blocs (« block x,y »). Chacun est un nœud `LOD` sur les trois niveaux de `RSArea` (LVL0 400 sommets, LVL1 100, LVL2 25), avec ses jointures vers ses voisins de droite et du bas, comme `renderBlock`. Le renderer élimine donc les blocs hors du champ et choisit le niveau de chacun selon sa distance : fin jusqu'à 40 000, grossier au-delà de 100 000, réglable avec « LOD Scale ». Le jeu, lui, dessine tous ses blocs au même niveau (`world_detail`). Le menu « Terrain Detail » propose les deux : « By Distance », ou un niveau forcé partout.
- **Les ombres des arbres** : le nœud des forêts a le masque `WorldScene::kForestMask` (le bit 31). Sans **Tree Shadows** (dans la section Sun ; `-treeshadows` au départ), le viewer retire ce bit de `RenderOptions::shadowMask` : la passe d'ombre saute toute la branche des forêts, sans parcourir les arbres. Sur NICARAGA, les objets qui projettent une ombre passent de 304 à 4, et leur temps CPU de 0,26 à 0,05 ms.
- **Les parts amenées par une scène** (placement 1) sont placées comme `SCMission::placeRelativeToPlayer` les place par rapport au joueur, mais par rapport au centre de la zone de la scène, cap au nord : position = centre de la zone + `relative_position`, cap = `relative_azymuth`. Les avions ont donc l'altitude de la zone plus leur décalage, au lieu des 4 000 fixes d'avant ; ils sont ensuite remontés s'ils sont sous le sol.
- **La sélection dans le graphe** montre la sphère de culling du nœud (trois cercles jaunes) et sa boîte englobante (violette, dans l'espace du parent : les boîtes des meshes des Geodes, les sphères des Instancers), tant que la section Scene Graph est ouverte.
- **Les objets posés sur le terrain** : `r4bridge::onGround(area, entity, matrix, flies)` déplace un objet pour que le plus bas de ses sommets, une fois tourné par sa matrice, soit sur le sol à son origine ; un objet qui vole n'est que remonté s'il est dessous. La hauteur du sol vient de `r4bridge::groundHeight`, calculée sur les triangles LVL0 tels que `terrainModel` les dessine. `RSArea::getY` n'est pas utilisé ici : il prend le sommet le plus proche puis interpole dans la cellule qui part de ce sommet, même quand le point est de l'autre côté, et ajoute 5 au bord des blocs ; sur ARENAMIS, il donnait des écarts jusqu'à 86 unités, d'où des chars qui flottaient. Avec le calcul exact, les objets d'ARENAMIS bougent de -3 à +7 unités. Le viewer l'applique aux objets de la carte et aux parts de la mission (toggle **On Ground** du panneau World, actif par défaut ; `-nolift` garde la hauteur des données). Un avion de la mission vole quand il est à plus de 10 au-dessus du sol après `placeParts`. Un objet hors de la carte n'est pas touché. Le sol est mesuré à l'origine de l'objet et sur LVL0 : de loin, sur LVL1 et LVL2, un objet peut paraître un peu au-dessus ou dans le terrain.
- **Un atlas des textures du terrain** : toutes les images posées sur la carte (TXMPACK, ACCPACK ou les textures propres à la carte) sont rangées dans une seule image d'index de palette, en grille de cases égales (la case fait la taille de la plus grande image). Chaque image garde sa taille dans sa case, sans être agrandie. Il n'y a donc qu'un seul matériau texturé pour tout le terrain. Chaque sommet porte le numéro de sa case dans le vert et le bleu de sa couleur, et le log2 de la largeur et de la hauteur de son image dans son alpha. Le shader borne le filtrage bilinéaire à l'image dans sa case, donc rien ne déborde d'une case à l'autre. Le rendu est le même qu'avec une texture par image, mais un bloc ou un groupe du quadtree ne coûte plus que deux draw calls (couleurs et textures) : sur TURKEY, de 66 à 50 à 30 km, de 100 à 49 à 150 km, de 92 à 75 à 400 km, et de 62 changements de texture à 6. Avec `-upscale 3` (xBR 4x), les cases de 128 texels passent à 512 et l'atlas de TURKEY fait 4096², la taille maximale garantie par beaucoup de GPU.
- **Un quadtree sur les blocs** : les blocs sont groupés par 2×2, 4×4… jusqu'à 32×32 (la carte en fait 18×18). Chaque groupe est un nœud `LOD` à deux enfants. De près, il montre ses quatre enfants, que le renderer élimine par leurs bornes. Dès que tous ses blocs sont de toute façon au niveau le plus grossier (la distance de son centre dépasse le début de LVL2 plus l'écart entre ce centre et le centre de son bloc le plus éloigné), il montre un mesh fusionné par matériau des LVL2 de ses blocs, bandes de raccord comprises. L'image ne change pas, mais les draw calls baissent : sur TURKEY, de 126 à 66 à 30 km, de 215 à 100 à 150 km et de 634 à 92 à 400 km. Le centre des LOD des blocs est celui de leurs sommets LVL0. « Terrain Detail » force les groupes à montrer leurs blocs pour Fine et Medium, et leurs meshes fusionnés pour Coarse.
- **Hors de la carte**, le terrain continue jusqu'au bord du monde (deux fois la taille de la carte de chaque côté, comme les jupes de `RSArea::BuildSkirts`). libRealSpace n'a pas de données au-delà de la carte : le bord de la carte est prolongé, ses hauteurs et ses couleurs lissées le long du bord, puis il s'efface sur 150 km vers la hauteur 0 et la couleur moyenne du bord (en `-lit`, vers le type de terrain le plus fréquent du bord, ombré sur sa rampe comme la carte).
  - Une **bande de raccord** de 20 km entoure la carte. Chaque bloc du bord porte la sienne dans chacun de ses niveaux de détail : elle part des sommets du bord de ce niveau (LVL1 et LVL2 s'arrêtent avant LVL0 sur les bords droit et bas) et change de niveau avec le bloc, sans fente. Elle utilise le matériau du terrain, donc aucun draw call en plus.
  - Puis **deux anneaux de tuiles** : le premier jusqu'à 360 km du centre, en cellules de 20 km ; le second jusqu'au bord du monde (720 km), plat, une cellule par tuile. Chaque anneau compte 12 tuiles, chacune un nœud avec ses bornes et un seul mesh : le renderer élimine celles hors du champ, et il y a au plus 24 draw calls pour tout l'extérieur.
  - Le **brouillard**, de la couleur du ciel, commence 120 km et finit 450 km au-delà de la cible (réglables dans le panneau, `-nofog` pour l'enlever). Le plan lointain s'arrête juste après le brouillard, ce qui garde la précision du depth buffer ; sans brouillard, il passe à 1 500 km. « Whole Map » cadre la carte seule.
- **Les couleurs** sont celles des rampes de `RSArea` (`ComputeTriangleColors`). Leurs ombres dépendent de `terrain_shadow_lift` et `terrain_shadow_range` du `config.ini` du jeu : le viewer le lit dans `-data` (`config.ini` ou `assets/config.ini`) ou dans `./assets/config.ini`, et prend les valeurs par défaut sinon.
- **Des jupes** : des murs de 600 descendent le long des bords de chaque bloc, pour qu'un voisin à un autre niveau ne laisse pas de fente. Le jeu n'en a pas besoin.
- **Les couleurs** viennent de `RSArea` et ne sont pas éclairées, comme dans le jeu. Les textures sont une couche en `Blend` par-dessus la couleur, bornée aux bords. Leurs texels sont des index de palette ombrés le long de leur rampe, comme les sommets : l'intensité de chaque sommet est sa lumière (`flag` / 255, moyennée avec `terrain_flat`), ou le soleil avec **Lit Terrain**. Une texture garde donc le ton de son voisinage à l'ombre, sans halo autour des zones texturées. L'index le plus sombre d'une rampe reste lui-même (`RampFirst::ITSELF`), alors que pour les faces des objets il désigne toute la rampe. Le filtrage bilinéaire est fait dans le shader après l'ombrage ; avec `-upscale`, l'image agrandie est ramenée aux index qu'elle utilise. Le relèvement des ombres de `terrain_shadow_lift` ne s'applique qu'aux couleurs des sommets. Elles sont dans **la palette du jeu** (PALETTE.IFF), index 255 transparent. Attention : la palette propre à chaque `RSImage` est celle de `SCRenderer`, qui reste vide tant que `SCRenderer::init` n'a pas été appelé. Le pont ne l'utilise donc pas.
- **Les overlays** (les pistes et leur sol, `objectOverlay`) : comme `renderMapOverlay`.
  - Un mesh par overlay, ses triangles dans l'ordre de `drawList`, dessinés après le terrain (`drawOrder` 1) et sans écrire la profondeur. Chaque triangle recouvre donc les précédents, comme le `GL_LEQUAL` du jeu, sans surélévation.
  - Un triangle à la couleur 255 prend sa couleur de palette ; les autres prennent l'image `GetImageByID(color + 255)`, avec les UV / 128. Leurs pixels transparents prennent la couleur de palette du triangle, comme sous `GL_DECAL`.
  - Les images sont réunies dans un atlas, avec des bords répétés (le `GL_CLAMP_TO_EDGE` du jeu).
- **Les forêts** (`forestChunks`, `plantForest` et `treeModel` dans R4World). RSArea n'a pas d'arbres : le jeu en peint seulement dans quelques textures (217 sur TURKEY). Le pont en génère sur le vert.
  Deux sources, au choix (menu **Forest Source**, `-woods 0|1`) :
  - **Green Terrain + Noise** (`ForestMode::NOISE`, par défaut) : le vert du terrain et un bruit, décrits ci-dessous.
  - **Woods of the Textures** (`ForestMode::TEXTURES`) : un arbre sur les bois peints dans les textures du jeu, comme pour les bâtiments.
    - **Texels d'arbre** : vert foncé, et pas un aplat (leurs voisins varient), car les arbres dessinés sont ombrés alors qu'un aplat de jungle est uni.
    - **Textures retenues** : celles qui ont au moins 8 % de texels d'arbre, soit les bois, les vergers, les bosquets des villages et des parcs, et les buissons des cratères. Les rangs de culture des champs (232, 233) sont écartés.
    - **Densité** : un arbre par 400 de surface, les couronnes se touchent. Les arbres sont posés par les UV de chaque triangle.
    - **Comparaison** :

      | Carte | Noise | Textures |
      |---|---|---|
      | NICARAGA | 11,2 M arbres, 2,4 s | 3,8 M arbres, 0,5 s |
      | TURKEY | 2,1 M arbres, 1,2 s | 70 000 arbres, 0,3 s |
      | MAURITAN | 2,5 M arbres, 1,4 s | 139 000 arbres, 0,4 s |

  Le reste ci-dessous vaut pour les deux sources (modèles, graphe, chargement).
  - **Où** : les triangles de LVL0 de type 3 (la rampe verte de la palette) ou de type 4 (la jungle, rampe verte foncée à partir de 128), sans sommet d'eau. Le type est celui du sommet le plus haut, comme dans `RSArea`. Les cellules de la ville et du port (224, 225, 229, 231) restent nues.
  - **La jungle** (NICARAGA) : des bois sur la plus grande partie, mais moins denses (×0,4), et presque seulement des feuillus, dans la rampe verte foncée de leur sol.
  - **Densité** : un bruit de quelques km fait des bois, jusqu'à 750 arbres/km² en leur cœur, et des arbres isolés entre eux. Sous les textures de bois (216 et 217), la densité est fixe. Il n'y a pas d'arbre près des pistes ni des objets de la carte et de la mission.
  - **Les mêmes arbres à chaque chargement** : le tirage est calé sur la cellule.
  - **Les modèles** : deux essences en low poly, à faces plates, dans les rampes de la palette (PaletteRamp) :
    - un conifère, plus fréquent en altitude ;
    - un feuillu.

    Chacune a une version détaillée jusqu'à 1 500 et une version simple (pyramide, octaèdre) jusqu'à 6 000.
  - **Le graphe** : la carte est découpée en carrés de 5 km, chacun un nœud `LOD` coupé au-delà de 6 000 avec un `Instancer` par essence. Le renderer cule chaque arbre et choisit son niveau ; les arbres projettent leur ombre.
  - **Tout au chargement**, en `Placement` compactes de R4nkine (24 octets par arbre) :
    - TURKEY : environ 2,1 millions d'arbres, 50 Mo, plantés en environ 1 s ;
    - NICARAGA : environ 11 millions d'arbres, 270 Mo, plantés en environ 1,5 s.

    Seuls les arbres visités par le renderer reçoivent une matrice. Pendant le vol, les carrés hors du champ ou au-delà de 6 km sont écartés d'un bloc par leur nœud `LOD`.
- **Les villes, villages, fermes et ports** (`cityModel` dans R4World ; couche **Buildings** du viewer). Les textures du jeu (TXMPACK) sont les mêmes sur toutes les cartes. Sur chaque cellule dont la texture montre des bâtiments, des immeubles sont posés sur ce que l'image montre :
  - **relevés à la main** sur les images (`kBuildings`, 143 bâtiments) :
    - les deux centres-villes (229, 230) ;
    - le port (223 à 228, en morceaux qui ne couvrent chacun que la moitié d'une cellule) ;
    - la ferme (233) ;
    - les villages sur herbe (248, 249) et sur sable (250, 251) ; ces derniers sont sur TURKEY ;
  - **trouvées sur l'image** : les maisons (231), à leurs toits gris clair. Les maisons dessinées seulement en contour restent à plat.

  Le passage d'un texel de l'image au sol se fait par les UV de chaque triangle, ceux que `RSArea` donne : l'image entière sur la cellule, ou une moitié d'image sur un triangle pour les textures 64 × 64 du port.

  **Rendu** :
  - **toit** : l'image elle-même, donc vue de haut la carte reste la texture ;
  - **murs** : des façades faites par le pont (bureaux à fenêtres, maisons, bardage des hangars), sur des étages de 8 ;
  - **éclairage** : par le soleil comme les objets, et ils projettent leur ombre.

  **Aucun bâtiment sur un objet de la carte ou de la mission** : un bâtiment dont l'emprise touche le rayon d'un objet n'est pas posé. Par exemple, les deux silos d'un village de TURKEY laissent la place à l'objet de la carte posé à cet endroit.

  Un nœud par cellule (« city x,y »). NICARAGA en compte environ 2 200, TURKEY 136. `-diag` écrit aussi toutes les textures du jeu (`all_<id>.ppm`), utilisées ou non.
- **Pas encore fait** : le ciel, les nuages et le brouillard.

Le placement des parts reprend `SCMission::loadMission` et `SCMissionScene::spawnCast` :
- **Le joueur** : sa position plus celle de la zone de la mission qu'il nomme.
- **Les autres parts** apparaissent avec la première scène dont le `cast` les contient (le `cast` donne l'index de la part dans la liste) :
  - avec `placement` 0, elles sont placées dans leur zone (position plus `areas[area_id]->position`) ;
  - avec `placement` 1, le jeu les place par rapport au joueur au moment où la scène s'active (`placeRelativeToPlayer`). Le viewer les met au centre de la zone de la scène, à 4000 d'altitude pour un avion (`EntityType::jet`).
- **Ensuite**, une part sous le sol est posée dessus (`RSArea::getY`), et un avion à moins de 10 du sol aussi.
- **Une part placée dans le repère du monde** (`area_id` 255, `placement` 0) reste telle quelle.

## Testé

- Mode Missions : testé avec `TURKEY.PAK` sur les mondes TURKEY et TURKEY22 :
  - 257 762 triangles de terrain, 22 textures et 44 objets de carte ;
  - environ 0,3 s de chargement ;
  - les objets sont posés sur le terrain.

  Aucune mission de TOBIGTRE.TRE n'utilise la Turquie.
- Avec `BIGTRE.TRE` (Strike Commander, textures du terrain prises dans TOBIGTRE.TRE), `MAURITAN.PAK` et `TURKEY.PAK`, on obtient 11 missions et mondes : MISN-1A à 2A, 12A/B, 14A/B, MAURITAN, TURKEY et TURKEY2.
  - MISN-1A : 31 objets de carte et 9 parts.
  - Nouakchott, avec son centre-ville, ses maisons et son port, et 28 fermes autour ; 3 074 bâtiments en tout.
  - 2,5 millions d'arbres, plantés en environ 1,4 s.
  - Les forêts sont en carrés de 2 km groupés par bloc : la culture passe de 23 à 5 ms sur cette carte.
- Avec `SYRIA.PAK`, MISN-A1 (la première mission de Tactical Operations) montre :
  - les deux pistes (236 triangles d'overlay) ;
  - les 41 parts réparties dans 10 scènes, plus 8 sans scène ;
  - la base d'attaque (scène 0) et la base d'entraînement (scène 6), posées au sol à côté de leurs pistes.
- Testé sur Linux (GCC 13) avec une copie de libRealSpace et R4nkine `6af0d4b`. Les données venaient de `TOBIGTRE.TRE`.
- 342 objets listés. Rendu vérifié sur F-16DES, A-10 (verrière), 747, CARRIERW et T-72.
- R4nkine doit être au moins à `6af0d4b` (le mode `AlphaMode::ADDITIVE`, ajouté pour les verrières).
- Les objets sans géométrie (les obus `20MM`…) s'affichent vides, c'est normal.
- T-72 : les bandes derrière les chenilles sont les faces `TRAX` (propriété 9, `flags[1] = 255`), des traces au sol que `SCRenderer` dessine aussi. Si on veut les cacher, c'est probablement `flags[1] == 255` qui les distingue.
