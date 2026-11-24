# Création des activités : `Creatable<Base>`

Ce document décrit la généralisation de la factory de `SCStrike` (`setFactory` / `create()`) à toutes
les activités, pour qu'une activité s'instancie toujours de la même façon, quel que soit son contenu.

## 1. Objectif

- **Une seule façon d'instancier une activité** : `X::create()`, partout.
- **Le choix des types concrets se fait dans le `main.cpp`** de chaque exécutable, à côté des
  `setInstance`, avec `X::registerType<Y>()`.
- **La structure du projet ne change pas** : le code des activités reste dans les `.cpp`. Le seul
  template est un petit en-tête générique.
- **Les bibliothèques ne dépendent d'aucune cible** : `strike_commander` n'inclut ni `r4game`,
  ni `enhanced`, ni le debugger.

## 2. Constat actuel

### 2.1 Classes qui ne servent qu'à instancier un autre type

| Classe | Ce qu'elle redéfinit | Seul changement |
|---|---|---|
| `EnhancedMainMenu` | `OnStartNewGame`, `LoadGame`, `OnTrainingMission` | `new EnhancedGameFlow` / `new EnhancedTrainingMenu` |
| `EnhancedTrainingMenu` | `OnSearchAndDestroy` | `new EnhancedStrike` |
| `EnhancedGameFlow` | `flyMission` | `new EnhancedStrike` |
| `EnhancedPSGameFlow` | `flyMission` | `new EnhancedPSStrike` |
| `DebugTOGameFlow` | `playConv` | appelle seulement `TOGameFlow::playConv` |

`neosc_r4` traite déjà ce besoin pour le vol avec `SCStrike::setFactory`.

### 2.2 Méthodes recopiées pour changer un seul type

- `flyMission` : 6 copies (SC, PS, Enhanced, EnhancedPS, Debug, DebugPS).
  `EnhancedGameFlow::flyMission` et `DebugPSGameFlow::flyMission` ne transmettent pas
  `player_loadout`, contrairement aux autres copies.
- `playConv` : 5 copies (SC, PS, TO, Debug, DebugPS).

## 3. Mécanisme

### 3.1 `Creatable<Base>`

Nouveau fichier `src/engine/Creatable.h` :

```cpp
#pragma once
#include <type_traits>

// Emplacement de création d'un rôle d'activité : create() rend le type enregistré par
// registerType<T>(), ou Base si rien n'a été enregistré.
template <class Base>
class Creatable {
    inline static Base *(*s_factory)() = nullptr;

    template <class T>
    static Base *instantiate() { return new T(); }

public:
    static Base *create() {
        return s_factory != nullptr ? s_factory() : new Base();
    }

    template <class T>
    static void registerType() {
        static_assert(std::is_base_of_v<Base, T>, "T doit dériver de Base");
        s_factory = &instantiate<T>;
    }
};
```

- Une instanciation de `Creatable<Base>` = un emplacement (`s_factory`) propre à `Base`.
- `inline static` (C++17, déjà utilisé dans `SCStrike.h`) garantit un seul emplacement dans tout
  l'exécutable.
- `instantiate<T>` est générée dans l'unité de compilation qui appelle `registerType<T>()`,
  c'est-à-dire le `main.cpp` : seule la cible inclut l'en-tête de `T`.

### 3.2 Déclaration d'un emplacement

Chaque rôle d'activité ajoute une ligne d'héritage :

```cpp
class SCStrike       : public IActivity, public Creatable<SCStrike>       { ... };
class SCGameFlow     : public IActivity, public Creatable<SCGameFlow>     { ... };
class SCMainMenu     : public IActivity, public Creatable<SCMainMenu>     { ... };
class SCTrainingMenu : public IActivity, public Creatable<SCTrainingMenu> { ... };
class SCConvPlayer   : public IActivity, public Creatable<SCConvPlayer>   { ... };
class SCRegister     : public IActivity, public Creatable<SCRegister>     { ... };
```

### 3.3 Emplacement propre à une sous-classe

Quand une sous-classe doit pouvoir être créée en même temps que sa base dans un même exécutable
(voir § 5), elle déclare son propre emplacement et lève l'ambiguïté de nom :

```cpp
class TOGameFlow : public virtual SCGameFlow, public Creatable<TOGameFlow> {
public:
    using Creatable<TOGameFlow>::create;
    using Creatable<TOGameFlow>::registerType;
    ...
};
```

### 3.4 Création

`create()` remplace le `new`. L'appel à `init()` et l'empilement restent inchangés :

```cpp
void SCMainMenu::OnStartNewGame() {
    SCGameFlow *gfl = SCGameFlow::create();
    GameState.Reset();
    gfl->init();
    SCRegister *registerActivity = SCRegister::create();
    registerActivity->init();
    Game->addActivity(gfl);
    Game->addActivity(registerActivity);
}

void SCGameFlow::playConv(uint8_t convId) {
    SCConvPlayer *conv = SCConvPlayer::create();
    conv->init();
    conv->SetID(convId);
    this->pending_activities.push(conv);
}
```

## 4. Règles

1. **Toute activité est créée par `X::create()`**, jamais par `new X()`.
2. **Chaque rôle d'activité possède un emplacement `Creatable<Rôle>`**, y compris ceux qui n'ont
   pas de variante aujourd'hui (`SCRegister`, `SCCredit`, `SCNavMap`…).
3. **Les types concrets sont enregistrés dans le `main.cpp`**, avant la première création.
4. **`registerType` s'appelle sur la classe qui porte l'emplacement**. `PSGameFlow::registerType<X>()`
   écrirait dans l'emplacement de `SCGameFlow` : à éviter.
5. **Les objets qui ne sont pas des activités** (`SCMission`, `SCScene`, `TOScene`, cockpits,
   entités) restent créés par l'activité qui les utilise, via les hooks existants
   (`createMission`, `createCockpit`, `createVirtualCockpit`, `createEntity`, `createScen`…).

## 5. Emplacements

| Emplacement | Créé par | Variantes enregistrées |
|---|---|---|
| `SCStrike` | `SCGameFlow::flyMission`, `SCTrainingMenu::OnSearchAndDestroy` | `R4Strike`, `EnhancedStrike`, `PSStrike`, `EnhancedPSStrike`, `DebugStrike` |
| `SCGameFlow` | `SCMainMenu` | — |
| `SCTrainingMenu` | `SCMainMenu` | — |
| `SCMainMenu` | `main.cpp`, `TOGameSelectMenu` | — |
| `SCConvPlayer` | `SCGameFlow::playConv` | `PSConvPlayer`, `DebugConvPlayer`, `DebugPSConvPlayer` |
| `SCRegister` | `SCMainMenu`, `TOMainMenu` | — |
| `TOMainMenu` | `TOGameSelectMenu` | — |
| `TOGameFlow` | `TOMainMenu` | — |
| `TOConvPlayer` | `TOGameFlow::playConv` | — |
| autres activités (`SCCredit`, `SCFlicPlayer`, `SCAnimationPlayer`, `SCShot`, `MapShot`, `SCNavMap`, `SCObjectViewer`, `SCDogFightMenu`, `TOGameSelectMenu`) | — | — |

TO possède ses propres emplacements parce que `neosc` et `neosc_r4` font cohabiter SC et TO dans le
même exécutable (choix dans `TOGameSelectMenu`) : un emplacement unique par rôle ne permettrait pas
d'y créer à la fois `SCConvPlayer` et `TOConvPlayer`. `TOGameFlow::playConv` appelle donc
`TOConvPlayer::create()`.

PS n'a pas besoin de ses propres emplacements : aucun exécutable ne fait cohabiter PS et SC.
`PSStrike` et `PSConvPlayer` sont enregistrés dans `SCStrike` et `SCConvPlayer`.

## 6. Enregistrements par exécutable

| Exécutable | Enregistrements |
|---|---|
| `neosc` | aucun |
| `neosc_r4` | `SCStrike::registerType<R4Strike>()` (remplace `setFactory`) |
| `neosc_enhanced` | `SCStrike::registerType<EnhancedStrike>()` |
| `neops`, `vrpacific` | `SCStrike::registerType<PSStrike>()`, `SCConvPlayer::registerType<PSConvPlayer>()` |
| `neops_enhanced` | `SCStrike::registerType<EnhancedPSStrike>()`, `SCConvPlayer::registerType<PSConvPlayer>()` |
| `vrstrike`, `neoto` | aucun |
| `debugger` | selon le mode choisi dans `DebugGame`, voir § 7 |

Exemple `neosc_r4/main.cpp` :

```cpp
SCRenderer::setInstance(std::make_unique<R4Renderer>());
SCStrike::registerType<R4Strike>();
RSScreen::setInstance(std::make_unique<R4Screen>());
```

## 7. Debugger

`DebugGame` lance SC, TO ou PS à l'exécution. Chaque méthode de chargement enregistre les types de
son mode avant de créer l'activité :

```cpp
void DebugGame::loadSC() {
    SCStrike::registerType<DebugStrike>();
    SCConvPlayer::registerType<DebugConvPlayer>();
    ...
}

void DebugGame::loadPacificConv() {
    SCConvPlayer::registerType<DebugPSConvPlayer>();
    ...
}
```

`DebugGameFlow::flyMission` et `DebugGameFlow::playConv` deviennent inutiles, ainsi que
`DebugPSGameFlow::playConv` et `DebugTOGameFlow`.

**Exception** : `DebugPacificStrikeMISN` hérite deux fois de `SCStrike` (par `DebugStrike` et par
`PSStrike`, sans héritage virtuel). La conversion `DebugPacificStrikeMISN *` → `SCStrike *` est
ambiguë, il ne peut donc pas être enregistré dans `SCStrike`. `DebugPSGameFlow::flyMission` et
`playEndMission` gardent leur `new DebugPacificStrikeMISN()` tant que cette double base n'est pas
reprise (hors du périmètre de ce document).

## 8. Ce qui disparaît

- `SCStrike::setFactory` et `s_factory` dans `SCStrike` (remplacés par `Creatable<SCStrike>`).
  `SCStrike::create()` garde le même nom : les appelants actuels ne changent pas.
- `EnhancedMainMenu`, `EnhancedTrainingMenu`, `EnhancedGameFlow`, `EnhancedPSGameFlow`,
  `DebugTOGameFlow`.
- `PSGameFlow::flyMission`, `PSGameFlow::playConv`, `DebugGameFlow::flyMission`,
  `DebugGameFlow::playConv`, `DebugPSGameFlow::playConv`.
- L'écart sur `player_loadout` dans Enhanced (§ 2.2), puisque `SCGameFlow::flyMission` est utilisé.

`TOMainMenu` reste : il diffère aussi par `GameState.LoadTO`. Il crée `TOGameFlow::create()`.

## 9. Contraintes

- **Constructeur sans argument** : `create()` appelle `new T()`. Toutes les activités le respectent,
  sauf `EndMissionScene` et `TOKillMarksScene` (paramètres `PakArchive *`). Elles gardent un `new`
  direct, ou reçoivent leurs données par une méthode après la création.
- **Un type par emplacement et par processus** : l'enregistrement est global. Quand deux variantes
  d'un même rôle doivent cohabiter, il faut un emplacement distinct (cas de TO, § 5) ou un
  réenregistrement au changement de mode (cas du debugger, § 7).
- **Ordre d'initialisation** : enregistrer avant la première création, comme les `setInstance`.
- **`create()` n'appelle pas `init()`** : la séquence actuelle `create` → `init` → `addActivity`
  est conservée.
- **`create()` rend un pointeur sur le rôle** (`SCStrike *`) : un appelant qui a besoin d'une API
  propre à la variante fait un `dynamic_cast`, comme `R4Strike::mission()` aujourd'hui.

## 10. Plan de migration

Chaque étape doit compiler et garder tous les exécutables fonctionnels.

1. Ajouter `src/engine/Creatable.h`.
2. `SCStrike` : remplacer `s_factory` / `setFactory` / `create()` par `Creatable<SCStrike>`.
   `neosc_r4` passe à `registerType<R4Strike>()`.
3. Emplacements SC : `SCGameFlow`, `SCMainMenu`, `SCTrainingMenu`, `SCConvPlayer`, `SCRegister`.
   Remplacer les `new` correspondants par `create()`.
4. Enhanced : enregistrements dans `neosc_enhanced` et `neops_enhanced`, suppression des classes
   listées au § 8. `neosc_enhanced/main.cpp` crée `SCMainMenu::create()`.
5. PS : enregistrements dans `neops`, `vrpacific`, `neops_enhanced`, suppression de
   `PSGameFlow::flyMission` et `PSGameFlow::playConv`.
6. TO : emplacements `TOMainMenu`, `TOGameFlow`, `TOConvPlayer`.
7. Debugger : enregistrements par mode dans `DebugGame`, suppression des surcharges du § 8.
8. Activités restantes : emplacement et `create()` pour `SCCredit`, `SCFlicPlayer`,
   `SCAnimationPlayer`, `SCShot`, `MapShot`, `SCNavMap`, `SCObjectViewer`, `SCDogFightMenu`,
   `TOGameSelectMenu` et les activités Debug.
9. Facultatif : passer les constructeurs des activités en `protected` pour interdire le `new`
   direct (impose de rendre `Creatable<Base>` ami de chaque variante).

## 11. Validation

- Build de tous les exécutables après chaque étape.
- Vérification par exécutable : menu principal → nouvelle partie → conversation → mission → retour ;
  entraînement ; chargement de partie ; sélection SC/TO pour `neosc` et `neosc_r4`.
- Vérifier que le type obtenu est bien la variante attendue (R4Strike, EnhancedStrike, PSStrike…).
- Vérifier que le loadout choisi par le joueur est transmis au vol dans Enhanced (§ 2.2).
