# Activités typées par exécutable : `Activity<T>`

Ce document décrit comment remplacer les sous-classes de « câblage » et la factory de `SCStrike`
par une déclaration de types explicite, propre à chaque exécutable.

## 1. Objectif

Chaque exécutable déclare, à un seul endroit, les types concrets de ses activités (menus, gameflow,
vol, conversations…). Toutes les activités reçoivent ces types par un paramètre template commun.

- **Une seule façon de faire** : toute activité hérite d'`Activity<T>`, sans exception.
- **Contexte explicite** : en lisant le `main.cpp`, on sait quels types utilise l'exécutable.
- **Pas d'état global de fabrication** : les types sont résolus à la compilation, sans factory
  ni pointeur de fonction statique.
- **Ouvert aux évolutions** : une activité qui ne crée rien aujourd'hui a déjà accès à `Types`.

## 2. Constat actuel

Le code varie selon deux axes : le **jeu** (SC, PS, TO) et la **couche** (base, Enhanced, Debug, R4).
L'héritage simple combine mal ces deux axes.

### 2.1 Classes qui ne servent qu'à instancier un autre type

| Classe | Ce qu'elle redéfinit | Seul changement |
|---|---|---|
| `EnhancedMainMenu` | `OnStartNewGame`, `LoadGame`, `OnTrainingMission` | `new EnhancedGameFlow` / `new EnhancedTrainingMenu` |
| `EnhancedTrainingMenu` | `OnSearchAndDestroy` | `new EnhancedStrike` |
| `EnhancedGameFlow` | `flyMission` | `new EnhancedStrike` |
| `EnhancedPSGameFlow` | `flyMission` | `new EnhancedPSStrike` |
| `TOMainMenu` | `OnStartNewGame`, `LoadGame` | `new TOGameFlow` (et `LoadTO`) |
| `DebugTOGameFlow` | `playConv` | appelle seulement `TOGameFlow::playConv` |

Pour `neosc_r4`, la même contrainte est traitée par `SCStrike::setFactory` / `SCStrike::create()`.

### 2.2 Méthodes recopiées pour changer un seul type

- `flyMission` : 6 copies (SC, PS, Enhanced, EnhancedPS, Debug, DebugPS).
  `EnhancedGameFlow::flyMission` et `DebugPSGameFlow::flyMission` ne transmettent pas
  `player_loadout`, contrairement aux autres copies.
- `playConv` : 5 copies (SC, PS, TO, Debug, DebugPS).
- `renderConvPlayer` : 2 copies (Debug, DebugPS), différence : type de conv et nombre (256 / 313).

### 2.3 Héritage multiple

- `DebugPSGameFlow : DebugGameFlow, PSGameFlow` et `DebugPSConvPlayer : DebugConvPlayer, PSConvPlayer`
  reposent sur l'héritage virtuel (`public virtual SCGameFlow`, `public virtual SCConvPlayer`).
- `DebugPacificStrikeMISN : DebugStrike, PSStrike` hérite deux fois de `SCStrike` (non virtuel).
  `setMission` recopie sept membres d'un sous-objet vers l'autre, plusieurs méthodes sont relayées
  explicitement (`init`, `runFrame`, `firePrimary`…) et le destructeur remet à `nullptr` les
  pointeurs partagés pour éviter une double libération.

## 3. Règles

1. **Toute activité hérite d'`Activity<T>`**, y compris celles qui ne créent rien
   (`SCRegister`, `SCCredit`, `SCConvPlayer`, `SCStrike`, `DebugEmptyActivity`…).
2. **`IActivity` reste non template.** Le `GameEngine` continue de manipuler des `IActivity *`.
3. **Personne n'hérite directement d'`IActivity`** ; c'est imposé à la compilation (§ 4.1).
4. **Chaque exécutable déclare ses traits en entier**, sans paramètre ni héritage entre traits.
   Quelques lignes dupliquées sont acceptées : tout est lisible d'un coup d'œil.
5. **Les objets qui ne sont pas des activités** (`SCMission`, `SCScene`, `TOScene`, cockpits, entités)
   restent créés par l'activité qui les utilise, via les hooks existants (`createMission`,
   `createCockpit`, `createVirtualCockpit`, `createEntity`, `createScen`…). Ils ne passent pas par `T`.
6. **Les couches transverses** (Enhanced, Debug) s'écrivent comme des mixins template sur leur base,
   sur le modèle d'`EnhancedOrbit<Base>`.

## 4. Mécanisme

### 4.1 `IActivity` verrouillée

`src/engine/IActivity.h` : le constructeur devient privé, seul `Activity<T>` peut l'appeler.

```cpp
template <class> class Activity;

class IActivity {
    template <class> friend class Activity;
    IActivity();
public:
    virtual ~IActivity();
    // reste de l'interface inchangé
};
```

### 4.2 `Activity<T>`

Nouveau fichier `src/engine/Activity.h` :

```cpp
#pragma once
#include "IActivity.h"
#include "GameEngine.h"

template <class T>
class Activity : public IActivity {
public:
    using Types = T;

protected:
    Activity() = default;

    // Création + init + empilement : remplace la séquence répétée dans les menus et gameflows.
    template <class A, class... Args>
    A *push(Args &&...args) {
        A *activity = new A(std::forward<Args>(args)...);
        activity->init();
        GameEngine::instance().addActivity(activity);
        return activity;
    }
};
```

### 4.3 Classes de jeu

Chaque activité devient un template sur `T` :

```cpp
template <class T> class SCMainMenu     : public Activity<T> { ... };
template <class T> class SCGameFlow     : public Activity<T> { ... };
template <class T> class SCTrainingMenu : public Activity<T> { ... };
template <class T> class SCStrike       : public Activity<T> { ... };
template <class T> class SCConvPlayer   : public Activity<T> { ... };
template <class T> class SCRegister     : public Activity<T> { ... };

template <class T> class PSGameFlow   : public SCGameFlow<T>   { ... };
template <class T> class PSStrike     : public SCStrike<T>     { ... };
template <class T> class PSConvPlayer : public SCConvPlayer<T> { ... };
template <class T> class TOGameFlow   : public SCGameFlow<T>   { ... };
template <class T> class TOMainMenu   : public SCMainMenu<T>   { ... };
template <class T> class TOConvPlayer : public SCConvPlayer<T> { ... };
template <class T> class R4Strike     : public SCStrike<T>     { ... };
```

L'héritage virtuel `public virtual SCGameFlow` / `SCConvPlayer` n'est plus nécessaire.

Les méthodes de création n'existent plus qu'une fois :

```cpp
template <class T>
void SCMainMenu<T>::OnStartNewGame() {
    GameState.Reset();
    this->template push<typename T::GameFlow>();
    this->template push<typename T::Register>();
}

template <class T>
void SCGameFlow<T>::flyMission(uint8_t) {
    auto *fly = new typename T::Strike();
    fly->init();
    fly->player_loadout = this->player_loadout_pending;
    this->player_loadout_pending = false;
    fly->setMission(this->missionToFly);
    this->missionToFly = nullptr;
    this->fly_mission.push(fly);
    this->next_miss = this->GameState.mission_id;
}

template <class T>
void SCGameFlow<T>::playConv(uint8_t convId) {
    auto *conv = new typename T::Conv();
    conv->init();
    conv->SetID(convId);
    this->pending_activities.push(conv);
}
```

`fly_mission` et `pending_activities` restent des conteneurs de pointeurs de base
(`std::queue<IActivity *>` ou équivalent).

### 4.4 Sélection du jeu à l'exécution

`sc` et `neosc_r4` choisissent SC ou TO au lancement. `TOGameSelectMenu` reçoit les deux jeux de traits :

```cpp
template <class SC, class TO>
class TOGameSelectMenu : public Activity<SC> { ... };

template <class SC, class TO>
void TOGameSelectMenu<SC, TO>::onChoice(bool to) {
    if (to) {
        this->template push<typename TO::MainMenu>();
    } else {
        this->template push<typename SC::MainMenu>();
    }
}
```

## 5. Traits par exécutable

Les traits sont déclarés dans le `main.cpp` (ou un en-tête propre à l'exécutable), à côté des
`setInstance`. Membres attendus :

| Membre | Rôle |
|---|---|
| `Strike` | vol |
| `Conv` | conversations |
| `GameFlow` | déroulement de campagne |
| `Training` | menu d'entraînement |
| `MainMenu` | menu principal |
| `Register` | écran d'enregistrement du pilote |

D'autres membres s'ajoutent quand une activité en a besoin. Un membre manquant produit une erreur
de compilation à l'endroit où il est utilisé.

### 5.1 `neosc_r4`

```cpp
struct R4SCTypes {
    using Strike   = R4Strike<R4SCTypes>;
    using Conv     = SCConvPlayer<R4SCTypes>;
    using GameFlow = SCGameFlow<R4SCTypes>;
    using Training = SCTrainingMenu<R4SCTypes>;
    using MainMenu = SCMainMenu<R4SCTypes>;
    using Register = SCRegister<R4SCTypes>;
};

struct R4TOTypes {
    using Strike   = R4Strike<R4TOTypes>;
    using Conv     = TOConvPlayer<R4TOTypes>;
    using GameFlow = TOGameFlow<R4TOTypes>;
    using Training = SCTrainingMenu<R4TOTypes>;
    using MainMenu = TOMainMenu<R4TOTypes>;
    using Register = SCRegister<R4TOTypes>;
};

int main(int argc, char *argv[]) {
    // ... setInstance inchangés, SCStrike::setFactory supprimé ...
    GameEngine &game = GameEngine::instance();
    IActivity *first = nullptr;
    if (/* TO présent */) {
        first = new TOGameSelectMenu<R4SCTypes, R4TOTypes>();
    } else {
        first = new R4SCTypes::MainMenu();
    }
    first->init();
    game.addActivity(first);
}
```

### 5.2 Autres exécutables

| Exécutable | Traits | Point d'entrée |
|---|---|---|
| `sc` | `SCBaseTypes`, `TOBaseTypes` (`Strike = SCStrike<…>`) | `TOGameSelectMenu<SCBaseTypes, TOBaseTypes>` |
| `strike_commander_enhanced` | `EnhancedSCTypes` (`Strike = EnhancedStrike<…>`) | `EnhancedSCTypes::MainMenu` |
| `ps`, `vrpacific` | `PSBaseTypes` (`Strike = PSStrike<…>`, `Conv = PSConvPlayer<…>`) | `PSBaseTypes::GameFlow` |
| `ps_enhanced` | `EnhancedPSTypes` (`Strike = EnhancedPSStrike<…>`) | `EnhancedPSTypes::GameFlow` |
| `vrstrike` | `VRSCTypes` | `VRSCTypes::MainMenu` |
| `debugger` | `DebugSCTypes`, `DebugPSTypes`, `DebugTOTypes` | choix dans `DebugGame` |

## 6. Couches transverses (Enhanced, Debug)

Les couches s'écrivent comme des templates sur leur base, à l'image d'`EnhancedOrbit<Base>` :

```cpp
template <class Base> class DebugStrikeLayer : public Base { ... };  // ex-DebugStrike
template <class Base> class DebugFlowLayer   : public Base { ... };  // ex-DebugGameFlow
template <class Base> class DebugConvLayer   : public Base { ... };  // ex-DebugConvPlayer
```

Exemple Debug + PS :

```cpp
struct DebugPSTypes;

class DebugPSStrike : public DebugStrikeLayer<PSStrike<DebugPSTypes>> {
    void init() override;                                    // verticalOffset = 0, eye_y = 0
    void renderMissionLoadingDialog(bool &show) override;    // liste des missions PS
};

class DebugPSGameFlow : public DebugFlowLayer<PSGameFlow<DebugPSTypes>> {
    // uniquement les popups : playMidGame, playShowMap, lookAtLedger, lookAtKillBoard…
};

struct DebugPSTypes {
    using Strike   = DebugPSStrike;
    using Conv     = DebugConvLayer<PSConvPlayer<DebugPSTypes>>;
    using GameFlow = DebugPSGameFlow;
    using Training = SCTrainingMenu<DebugPSTypes>;
    using MainMenu = SCMainMenu<DebugPSTypes>;
    using Register = SCRegister<DebugPSTypes>;
    static constexpr int convCount = 313;
};
```

`DebugFlowLayer::renderConvPlayer` utilise `Types::Conv` et `Types::convCount` : une seule copie.

## 7. Ce qui disparaît

- `SCStrike::setFactory`, `SCStrike::create()` et `s_factory`.
- `EnhancedMainMenu`, `EnhancedTrainingMenu`, `EnhancedGameFlow`, `EnhancedPSGameFlow`,
  `TOMainMenu` (si `LoadTO` est traité, voir § 9), `DebugTOGameFlow`.
- `DebugPacificStrikeMISN` et `DebugPSConvPlayer` (remplacés par des compositions de couches),
  avec les recopies de membres et les relais de méthodes.
- Les copies de `flyMission`, `playConv`, `renderConvPlayer`.
- L'héritage virtuel sur `SCGameFlow` et `SCConvPlayer`, et les `static_cast<DebugStrike *>`.

## 8. Organisation du code

- Le code des classes template passe dans les en-têtes, ou dans des fichiers `*_impl.h`
  inclus par l'exécutable.
- L'instanciation explicite se fait **côté exécutable**, pour que la lib ne dépende d'aucune cible :

  ```cpp
  // neosc_r4/instantiations.cpp
  #include "SCGameFlow_impl.h"
  template class SCGameFlow<R4SCTypes>;
  template class TOGameFlow<R4TOTypes>;
  ```

- Syntaxe à respecter dans les templates : `this->membre` pour les membres hérités,
  `typename T::X` pour les types dépendants, `this->template push<...>()` pour les membres
  template d'une base dépendante.

## 9. Points d'attention

- **Variables `static` locales et membres statiques** : une copie par instanciation. Vérifier
  qu'aucune n'est supposée partagée entre `SC` et `TO` dans un même exécutable (`sc`, `neosc_r4`).
- **`TOMainMenu`** : il diffère aussi par `GameState.LoadTO`. Soit il reste une sous-classe
  (`TOMainMenu<T> : SCMainMenu<T>`), soit le chargement passe par les traits.
- **Activités dérivées de `SCShot`** (`EndMissionScene`, `MapShot`, `TOKillMarksScene`) :
  ce sont des activités, elles suivent la règle `Activity<T>`.
- **Temps de compilation et lisibilité des erreurs** : à surveiller au fil de la migration.
- **`GameEngine` / `DebugGame`** : inchangés, ils ne manipulent que des `IActivity *`.

## 10. Plan de migration

Chaque étape doit compiler et garder tous les exécutables fonctionnels.

1. Ajouter `Activity<T>` (§ 4.2) sans verrouiller `IActivity`.
2. Migrer la chaîne SC : `SCMainMenu`, `SCGameFlow`, `SCTrainingMenu`, `SCStrike`, `SCConvPlayer`,
   `SCRegister`. Déclarer les traits de `sc` et `neosc_r4`, supprimer la factory de `SCStrike`.
3. Migrer TO : `TOGameFlow`, `TOMainMenu`, `TOConvPlayer`, `TOGameSelectMenu`.
4. Migrer PS : `PSGameFlow`, `PSStrike`, `PSConvPlayer` ; traits de `ps`, `vrpacific`.
5. Migrer Enhanced : traits de `strike_commander_enhanced` et `ps_enhanced`, suppression des
   sous-classes listées au § 7.
6. Migrer Debug : couches `DebugStrikeLayer`, `DebugFlowLayer`, `DebugConvLayer` ; traits du debugger.
7. Migrer les activités restantes (`SCCredit`, `SCFlicPlayer`, `SCDogFightMenu`, `SCObjectViewer`,
   `SCShot` et dérivées, `SCNavMap`, `SCAnimationPlayer`, activités Debug).
8. Verrouiller `IActivity` (§ 4.1) : toute classe qui hérite encore directement d'`IActivity`
   ne compile plus.

## 11. Validation

- Build de tous les exécutables après chaque étape.
- Vérification manuelle par exécutable : menu principal → nouvelle partie → conversation → mission
  → retour ; entraînement ; chargement de partie ; sélection SC/TO pour `sc` et `neosc_r4`.
- Vérifier que le loadout choisi par le joueur est bien transmis au vol dans Enhanced et Debug PS
  (§ 2.2).
