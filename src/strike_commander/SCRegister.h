//
//  SCRegister.h
//  libRealSpace
//
//  Created by fabien sanglard on 1/31/2014.
//  Copyright (c) 2014 Fabien Sanglard. All rights reserved.
//

#pragma once

#include "../engine/keyboard.h"
#include "SCState.h"
class SCRegister : public IActivity {
public:
    SCRegister();
    ~SCRegister();

    void init();
    void runFrame();
    void checkKeyboard();
    void checkZones();
protected:
    SCState &GameState = SCState::getInstance();
private:
    InputActionSystem::SpecialKeyCallback m_prevSpecialKeyCB = nullptr; // AJOUT
    int m_tabSuppressFrames{0};
    RLEShape book;
    std::string *current_entry{nullptr};
    RSFont *font{nullptr};

    Keyboard* m_keyboard = nullptr;  // AJOUT

    // Trois éditeurs
    std::shared_ptr<Keyboard::TextEditor> m_editorFirstName;
    std::shared_ptr<Keyboard::TextEditor> m_editorName;
    std::shared_ptr<Keyboard::TextEditor> m_editorCallsign;
    std::vector<SCZone*> zones;
    int m_activeEditor = 0;

    // Zones cliquables
    SCZone* m_zoneFirstName = nullptr;
    SCZone* m_zoneName = nullptr;
    SCZone* m_zoneCallsign = nullptr;

    void activateEditor(int index);
    void commitActiveEditor();
};
