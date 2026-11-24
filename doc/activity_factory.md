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

**Sous-classe qui est un rôle distinct.** Une variante (`TOGameFlow`, `PSConvPlayer`…) remplace sa
base et n'a pas d'emplacement. Une sous-classe qui coexiste avec sa base dans un même jeu, en
revanche, est un rôle à part entière et déclare son propre emplacement. C'est le cas de
`EndMissionScene` (dérive de `SCShot`) et de `TOKillMarksScene` (dérive de `EndMissionScene`) :
dans une partie TO, `SCShot`, `EndMissionScene` et `TOKillMarksScene` sont créés tous les trois.

`create` et `registerType` sont alors hérités de deux `Creatable` différents : un `using` lève
l'ambiguïté et désigne l'emplacement de la classe.

```cpp
class EndMissionScene : public SCShot, public Creatable<EndMissionScene> {
public:
    using Creatable<EndMissionScene>::create;
    using Creatable<EndMissionScene>::registerType;
    ...
};

class TOKillMarksScene : public EndMissionScene, public Creatable<TOKillMarksScene> {
public:
    using Creatable<TOKillMarksScene>::create;
    using Creatable<TOKillMarksScene>::registerType;
    ...
};
```

### 3.3 Changement de mode à l'exécution

Un emplacement contient un seul type pour tout le processus. Quand un exécutable choisit son jeu
à l'exécution (`TOGameSelectMenu` dans `neosc` et `neosc_r4`, `DebugGame` dans le debugger), le point
de choix **réenregistre** les emplacements qui dépendent du mode avant de créer la première activité.

Les emplacements se répartissent en deux catégories :

- **emplacements de la cible** : fixés une fois dans le `main.cpp`, jamais modifiés par le choix
  du jeu. Exemple : `SCStrike` dans `neosc_r4` (`R4Strike` sert à SC et à TO) ;
- **emplacements du mode** : enregistrés au point de choix, **tous à chaque fois**, y compris quand
  le mode utilise le type de base. Le résultat ne dépend donc pas du mode lancé précédemment.

```cpp
void TOGameSelectMenu::onStrikeCommander() {
    SCMainMenu::registerType<SCMainMenu>();
    SCGameFlow::registerType<SCGameFlow>();
    SCConvPlayer::registerType<SCConvPlayer>();
    ...
    SCMainMenu *main = SCMainMenu::create();
    ...
}

void TOGameSelectMenu::onTacticalOperations() {
    SCMainMenu::registerType<TOMainMenu>();
    SCGameFlow::registerType<TOGameFlow>();
    SCConvPlayer::registerType<TOConvPlayer>();
    ...
    SCMainMenu *main = SCMainMenu::create();
    ...
}
```

`registerType<SCConvPlayer>()` est valide : `std::is_base_of_v<SCConvPlayer, SCConvPlayer>` est vrai.
Une variante ne déclare pas d'emplacement : il n'y a qu'un emplacement par rôle (§ 3.2).

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

Une activité qui a besoin de données externes les reçoit par un setter, **après `create()` et
avant `init()`**. Le constructeur reste sans argument et `init()` garde sa signature virtuelle :

```cpp
TOKillMarksScene *end = TOKillMarksScene::create();
end->setArchives(&this->optShps2, &this->optPals2);   // utilisées par init()
end->init();
this->pending_activities.push(end);
```

Un setter est préféré à une surcharge `init(PakArchive *, PakArchive *)`, qui masquerait le `init()`
virtuel d'`IActivity`.

## 4. Règles

1. **Toute activité est créée par `X::create()`**, jamais par `new X()`.
2. **Chaque rôle d'activité possède un emplacement `Creatable<Rôle>`**, y compris ceux qui n'ont
   pas de variante aujourd'hui (`SCRegister`, `SCCredit`, `SCNavMap`…).
3. **Les types concrets sont enregistrés dans le `main.cpp`**, avant la première création, ou au
   point de choix du mode pour les exécutables qui choisissent leur jeu à l'exécution (§ 3.3).
4. **`registerType` s'appelle sur la classe qui porte l'emplacement**. `PSGameFlow::registerType<X>()`
   écrirait dans l'emplacement de `SCGameFlow` : à éviter.
5. **Les objets qui ne sont pas des activités** (`SCMission`, `SCScene`, `TOScene`, cockpits,
   entités) restent créés par l'activité qui les utilise, via les hooks existants
   (`createMission`, `createCockpit`, `createVirtualCockpit`, `createEntity`, `createScen`…).
6. **Un point de choix enregistre tous les emplacements du mode**, jamais seulement ceux qui changent,
   et ne touche pas aux emplacements de la cible (§ 3.3).
7. **Toute activité a un constructeur sans argument**, sans exception. Les données externes sont
   transmises par un setter entre `create()` et `init()` (§ 3.4).

## 5. Emplacements

| Emplacement | Créé par | Variantes enregistrées |
|---|---|---|
| `SCStrike` | `SCGameFlow::flyMission`, `SCTrainingMenu::OnSearchAndDestroy` | `R4Strike`, `EnhancedStrike`, `PSStrike`, `EnhancedPSStrike`, `DebugStrike` |
| `SCGameFlow` | `SCMainMenu` | `TOGameFlow` |
| `SCTrainingMenu` | `SCMainMenu` | — |
| `SCMainMenu` | `main.cpp`, `TOGameSelectMenu` | `TOMainMenu` |
| `SCConvPlayer` | `SCGameFlow::playConv` | `PSConvPlayer`, `TOConvPlayer`, `DebugConvPlayer`, `DebugPSConvPlayer` |
| `SCRegister` | `SCMainMenu` | — |
| `EndMissionScene` | `SCGameFlow` (`SHOW_KILL_MARKS`) | — |
| `TOKillMarksScene` | `TOGameFlow` (`SHOW_KILL_MARKS_ALT`) | — |
| autres activités (`SCCredit`, `SCFlicPlayer`, `SCAnimationPlayer`, `SCShot`, `MapShot`, `SCNavMap`, `SCObjectViewer`, `SCDogFightMenu`, `TOGameSelectMenu`) | — | — |

SC, TO et PS partagent les mêmes emplacements. Quand un exécutable fait cohabiter plusieurs jeux,
le point de choix réenregistre les emplacements du mode (§ 3.3).

## 6. Enregistrements par exécutable

| Exécutable | Enregistrements |
|---|---|
| `neosc` | aucun dans le `main` ; emplacements du mode dans `TOGameSelectMenu` (§ 3.3) |
| `neosc_r4` | `SCStrike::registerType<R4Strike>()` (remplace `setFactory`) ; emplacements du mode dans `TOGameSelectMenu` |
| `neosc_enhanced` | `SCStrike::registerType<EnhancedStrike>()` |
| `neops`, `vrpacific` | `SCStrike::registerType<PSStrike>()`, `SCConvPlayer::registerType<PSConvPlayer>()` |
| `neops_enhanced` | `SCStrike::registerType<EnhancedPSStrike>()`, `SCConvPlayer::registerType<PSConvPlayer>()` |
| `vrstrike` | aucun |
| `debugger` | selon le mode choisi dans `DebugGame`, voir § 7 |

Exemple `neosc_r4/main.cpp` :

```cpp
SCRenderer::setInstance(std::make_unique<R4Renderer>());
SCStrike::registerType<R4Strike>();
RSScreen::setInstance(std::make_unique<R4Screen>());
```

## 7. Debugger

`DebugGame` lance SC, TO ou PS à l'exécution : c'est un point de choix au sens du § 3.3. Dans le
debugger, le vol dépend aussi du mode, donc `SCStrike` est un emplacement du mode. Chaque méthode de
chargement enregistre l'ensemble de ses emplacements avant de créer l'activité :

```cpp
void DebugGame::loadSC() {
    SCStrike::registerType<DebugStrike>();
    SCConvPlayer::registerType<DebugConvPlayer>();
    ...
}

void DebugGame::loadTO() {
    SCStrike::registerType<DebugStrike>();
    SCConvPlayer::registerType<TOConvPlayer>();
    ...
}

void DebugGame::loadPacificConv() {
    SCStrike::registerType<DebugPacificStrikeMISN>();
    SCConvPlayer::registerType<DebugPSConvPlayer>();
    ...
}

void DebugGame::loadPacificMISN() {
    SCStrike::registerType<DebugPacificStrikeMISN>();
    ...
    SCStrike *main = SCStrike::create();
    main->init();
    main->setMission("PRLH-M1.IFF");
    this->addActivity(main);
}
```

`DebugGameFlow::flyMission` et `DebugGameFlow::playConv` deviennent inutiles, ainsi que
`DebugPSGameFlow::flyMission`, `DebugPSGameFlow::playConv` et `DebugTOGameFlow`.
`DebugPSGameFlow::playEndMission` crée son vol par `SCStrike::create()`.

Les variantes héritant virtuellement de `SCConvPlayer` (`DebugConvPlayer`, `DebugPSConvPlayer`,
`PSConvPlayer`) s'enregistrent normalement : la conversion vers une base virtuelle unique n'est pas
ambiguë.

**Héritage virtuel de `SCStrike`.** `DebugPacificStrikeMISN` dérive de `DebugStrike` et de
`PSStrike`. Aujourd'hui, ces deux classes héritent de `SCStrike` sans héritage virtuel : l'objet
contient deux sous-objets `SCStrike`, la conversion vers `SCStrike *` est ambiguë et
`DebugPacificStrikeMISN::setMission` recopie l'état de la partie `PSStrike` vers la partie
`DebugStrike`. `DebugStrike` et `PSStrike` héritent donc virtuellement de `SCStrike`, comme les
variantes de `SCConvPlayer` :

```cpp
class DebugStrike : public virtual SCStrike { ... };
class PSStrike    : public virtual SCStrike { ... };
class DebugPacificStrikeMISN : public DebugStrike, public PSStrike { ... };
```

- Un seul sous-objet `SCStrike` : `DebugPacificStrikeMISN` s'enregistre normalement, sans exception.
- Les recopies d'état dans `setMission`, les préfixes `DebugStrike::` / `PSStrike::` sur les membres
  de `SCStrike` et les `static_cast<DebugStrike *>` du debugger disparaissent.
- Une méthode virtuelle redéfinie à la fois dans `DebugStrike` et dans `PSStrike` doit être
  redéfinie dans `DebugPacificStrikeMISN` (déjà le cas pour `init`, `runFrame`, `renderMenu`,
  `renderUI`, `firePrimary`, `selectNextWeapon`). Celles redéfinies dans `PSStrike` seulement
  (`createMission`, `createCockpit`, `createVirtualCockpit`, `start`) viennent de `PSStrike`.
- `R4Strike` et `EnhancedStrike` gardent un héritage simple, inchangé. `EnhancedPSStrike` hérite
  de la base virtuelle par `PSStrike` ; `EnhancedOrbit` accède aux membres par `this->`, sans
  changement.

## 8. Ce qui disparaît

- `SCStrike::setFactory` et `s_factory` dans `SCStrike` (remplacés par `Creatable<SCStrike>`).
  `SCStrike::create()` garde le même nom : les appelants actuels ne changent pas.
- `EnhancedMainMenu`, `EnhancedTrainingMenu`, `EnhancedGameFlow`, `EnhancedPSGameFlow`,
  `DebugTOGameFlow`.
- `PSGameFlow::flyMission`, `PSGameFlow::playConv`, `TOGameFlow::playConv`,
  `DebugGameFlow::flyMission`, `DebugGameFlow::playConv`, `DebugPSGameFlow::playConv`.
- `DebugPSGameFlow::flyMission`, les recopies d'état de `DebugPacificStrikeMISN::setMission` et
  les `static_cast<DebugStrike *>` de `DebugPSGameFlow` et `DebugGame` (§ 7).
- `TOMainMenu::OnStartNewGame` : identique à `SCMainMenu::OnStartNewGame` une fois que
  `SCGameFlow::create()` rend `TOGameFlow`.
- L'écart sur `player_loadout` dans Enhanced (§ 2.2), puisque `SCGameFlow::flyMission` est utilisé.

`TOMainMenu` reste, pour `LoadGame` qui appelle `GameState.LoadTO`. Il crée `SCGameFlow::create()`.
`TOGameFlow` et `TOConvPlayer` restent pour leurs comportements propres, sans emplacement dédié.

## 9. Contraintes

- **Constructeur sans argument** : `create()` appelle `new T()`. Deux activités sont à adapter :
  - `EndMissionScene(PakArchive *, PakArchive *)` n'utilise pas ses paramètres (son `init()` lit
    `optShps` / `optPals`, chargés par `SCShot::init`) : ils sont supprimés ;
  - `TOKillMarksScene(PakArchive *, PakArchive *)` utilise `optShps2` / `optPals2` dans `init()` :
    le constructeur devient sans argument et les archives passent par `setArchives()`, appelé
    avant `init()` (§ 3.4).
- **Un type par emplacement et par processus** : l'enregistrement est global. Quand plusieurs jeux
  cohabitent dans un exécutable, le point de choix réenregistre les emplacements du mode
  (`TOGameSelectMenu`, `DebugGame`, § 3.3 et § 7).
- **Variante de cible sur un emplacement du mode** : `TOGameSelectMenu` enregistre les types de base
  ou TO. Si une cible avait besoin de sa propre variante d'un emplacement du mode (un `SCConvPlayer`
  propre à R4, par exemple), il faudrait transmettre cette variante au menu de sélection. Ce n'est
  pas le cas aujourd'hui.
- **Ordre d'initialisation** : enregistrer avant la première création, comme les `setInstance`.
- **`create()` n'appelle pas `init()`** : la séquence actuelle `create` → `init` → `addActivity`
  est conservée.
- **`create()` rend un pointeur sur le rôle** (`SCStrike *`) : un appelant qui a besoin d'une API
  propre à la variante fait un `dynamic_cast`, comme `R4Strike::mission()` aujourd'hui. Avec la base
  virtuelle (§ 7), un `static_cast` de `SCStrike *` vers `PSStrike *` ou `DebugStrike *` est
  interdit ; il n'en existe aucun aujourd'hui.
- **Méthodes appelées sur le rôle** : l'appelant ne connaît que `SCStrike *`, donc une méthode
  spécialisée par une variante doit être virtuelle dans le rôle. `SCStrike::setMission(char const *)`
  ne l'est pas : `PSStrike::setMission` le masque (musique, joueur au sol, première arme) et
  `DebugStrike::setMission(std::string)` a une autre signature. `setMission(char const *)` devient
  virtuel dans `SCStrike`, `PSStrike` et `DebugStrike` le redéfinissent avec cette signature.
  Sinon, `SCGameFlow::flyMission` appellerait `SCStrike::setMission` pour un `PSStrike`. Pour
  `DebugStrike`, la redéfinition ne change rien dans `flyMission` : son nettoyage ne s'exécute qu'au
  rechargement (`current_mission != nullptr`), jamais sur un vol qui vient d'être créé.

**Hors périmètre : fuite de `simulated_objects`.** Les objets éjectés (`SCStrike.cpp`, ~1578 et
~1592) sont ajoutés à `simulated_objects` mais ne sont libérés ni par `SCStrike::~SCStrike()`, ni par
`SCStrike::setMission` au rechargement ; seul `DebugStrike::setMission` les libère. La fuite existe
dans toutes les variantes. Correction indépendante de ce document : libérer `simulated_objects` dans
le destructeur et au rechargement dans `SCStrike::setMission`, puis retirer cette libération de
`DebugStrike::setMission`.

## 10. Plan de migration

Chaque étape doit compiler et garder tous les exécutables fonctionnels.

1. Ajouter `src/engine/Creatable.h`.
2. `SCStrike` : remplacer `s_factory` / `setFactory` / `create()` par `Creatable<SCStrike>`.
   `neosc_r4` passe à `registerType<R4Strike>()`. `setMission(char const *)` devient virtuel,
   redéfini dans `PSStrike` et `DebugStrike` (§ 9).
3. Emplacements SC : `SCGameFlow`, `SCMainMenu`, `SCTrainingMenu`, `SCConvPlayer`, `SCRegister`.
   Remplacer les `new` correspondants par `create()`.
4. Enhanced : enregistrements dans `neosc_enhanced` et `neops_enhanced`, suppression des classes
   listées au § 8. `neosc_enhanced/main.cpp` crée `SCMainMenu::create()`.
5. PS : enregistrements dans `neops`, `vrpacific`, `neops_enhanced`, suppression de
   `PSGameFlow::flyMission` et `PSGameFlow::playConv`.
6. TO : enregistrements des emplacements du mode dans `TOGameSelectMenu` (§ 3.3), création par
   `SCMainMenu::create()` dans les deux branches, suppression de `TOGameFlow::playConv` et de
   `TOMainMenu::OnStartNewGame`.
7. Debugger : héritage virtuel de `SCStrike` dans `DebugStrike` et `PSStrike`, simplification de
   `DebugPacificStrikeMISN` ; enregistrements par mode dans `DebugGame`, suppression des surcharges
   du § 8.
8. `EndMissionScene` et `TOKillMarksScene` : constructeurs sans argument, `setArchives()` pour
   `TOKillMarksScene`, emplacements propres avec `using` (§ 3.2), création par `create()` dans
   `SCGameFlow` et `TOGameFlow`.
9. Activités restantes : emplacement et `create()` pour `SCCredit`, `SCFlicPlayer`,
   `SCAnimationPlayer`, `SCShot`, `MapShot`, `SCNavMap`, `SCObjectViewer`, `SCDogFightMenu`,
   `TOGameSelectMenu` et les activités Debug.
10. Facultatif : passer les constructeurs des activités en `protected` pour interdire le `new`
    direct (impose de rendre `Creatable<Base>` ami de chaque variante).

## 11. Validation

- Build de tous les exécutables après chaque étape.
- Vérification par exécutable : menu principal → nouvelle partie → conversation → mission → retour ;
  entraînement ; chargement de partie ; sélection SC/TO pour `neosc` et `neosc_r4`.
- Vérifier que le type obtenu est bien la variante attendue (R4Strike, EnhancedStrike, PSStrike…).
- `neosc` / `neosc_r4` : conversation TO après le choix TO, conversation SC après le choix SC ;
  si le retour au menu de sélection est possible, enchaîner SC → TO → SC dans la même session.
  Dans `neosc_r4`, le vol reste un `R4Strike` dans les deux jeux.
- Debugger : enchaîner plusieurs modes dans la même session et vérifier les types obtenus.
- Vérifier que le loadout choisi par le joueur est transmis au vol dans Enhanced (§ 2.2).
- Debugger, mode Pacific : vol `DebugPacificStrikeMISN` depuis la conversation et depuis
  `loadPacificMISN` ; tir, changement d'arme, rechargement de mission dans le menu de debug.
