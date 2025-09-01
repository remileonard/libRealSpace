//
//  SCRegister.cpp
//  libRealSpace
//
//  Created by fabien sanglard on 1/31/2014.
//  Copyright (c) 2014 Fabien Sanglard. All rights reserved.
//

#include "precomp.h"
SCRegister::SCRegister() {
    m_keyboard = Game->getKeyboard();

    // Actions nécessaires (à ajouter dans enum InputAction)
    m_keyboard->registerAction(InputAction::KEY_TAB);
    m_keyboard->registerAction(InputAction::KEY_RETURN);
    m_keyboard->bindKeyToAction(InputAction::KEY_TAB, SDL_SCANCODE_TAB);
    m_keyboard->bindKeyToAction(InputAction::KEY_RETURN, SDL_SCANCODE_RETURN);
    m_keyboard->bindMouseButtonToAction(InputAction::MOUSE_LEFT, SDL_BUTTON_LEFT);
    // Création des éditeurs
    m_editorFirstName = m_keyboard->createTextEditor();
    m_editorName      = m_keyboard->createTextEditor();
    m_editorCallsign  = m_keyboard->createTextEditor();
}

SCRegister::~SCRegister() {
    m_keyboard = nullptr;
    InputActionSystem::getInstance().setSpecialKeyCallback(m_prevSpecialKeyCB);
    m_keyboard = nullptr;
}

void SCRegister::init() {
    TreEntry* entryMountain = Assets.GetEntryByName(Assets.optshps_filename);
    PakArchive pak;
    pak.InitFromRAM("",entryMountain->data,entryMountain->size);

    PakArchive bookPak;
    bookPak.InitFromRAM("subPak board",
                        pak.GetEntry(OptionShapeID::START_GAME_REGISTRATION)->data ,
                        pak.GetEntry(OptionShapeID::START_GAME_REGISTRATION)->size);
    book.init(bookPak.GetEntry(0)->data, bookPak.GetEntry(0)->size);

    VGAPalette* rendererPalette = VGA.getPalette();
    this->palette = *rendererPalette;

    TreEntry* palettesEntry = Assets.GetEntryByName(Assets.optpals_filename);
    PakArchive palettesPak;
    palettesPak.InitFromRAM("OPTSHPS.PAK",palettesEntry->data,palettesEntry->size);

    ByteStream paletteReader;
    paletteReader.Set(palettesPak.GetEntry(OPTPALS_PAK_STARTGAME_REGISTRATION)->data);
    this->palette.ReadPatch(&paletteReader);

    this->font = FontManager.GetFont("..\\..\\DATA\\FONTS\\SM-FONT.SHP");

    m_zoneName = new SCZone();
    m_zoneName->id = 0;
    m_zoneName->position = {87,99};
    m_zoneName->dimension = {60,6};
    m_zoneName->active = true;
    m_zoneName->onclick = [this](void*, int){ activateEditor(0); };
    zones.push_back(m_zoneName);

    m_zoneFirstName = new SCZone();
    m_zoneFirstName->id = 1;
    m_zoneFirstName->position = {171,99};
    m_zoneFirstName->dimension = {60,6};
    m_zoneFirstName->active = true;
    m_zoneFirstName->onclick = [this](void*, int){ activateEditor(1); };
    zones.push_back(m_zoneFirstName);

    m_zoneCallsign = new SCZone();
    m_zoneCallsign->id = 2;
    m_zoneCallsign->position = {120,105};
    m_zoneCallsign->dimension = {60,6};
    m_zoneCallsign->active = true;
    m_zoneCallsign->onclick = [this](void*, int){ activateEditor(2); };
    zones.push_back(m_zoneCallsign);

    m_editorFirstName->setText(GameState.player_firstname);
    m_editorName->setText(GameState.player_name);
    m_editorCallsign->setText(GameState.player_callsign);

    activateEditor(0);
}

void SCRegister::activateEditor(int index) {
    if (index < 0 || index > 2) return;

    commitActiveEditor();

    m_activeEditor = index;

    m_editorFirstName->setActive(false);
    m_editorName->setActive(false);
    m_editorCallsign->setActive(false);

    SDL_Rect r;
    switch (m_activeEditor) {
        case 1: // Firstname
            r = { m_zoneFirstName->position.x, m_zoneFirstName->position.y,
                  m_zoneFirstName->dimension.x, m_zoneFirstName->dimension.y };
            m_editorFirstName->setText(GameState.player_firstname);
            m_editorFirstName->setActive(true, &r);
            break;
        case 0: // Name
            r = { m_zoneName->position.x, m_zoneName->position.y,
                  m_zoneName->dimension.x, m_zoneName->dimension.y };
            m_editorName->setText(GameState.player_name);
            m_editorName->setActive(true, &r);
            break;
        case 2: // Callsign
            r = { m_zoneCallsign->position.x, m_zoneCallsign->position.y,
                  m_zoneCallsign->dimension.x, m_zoneCallsign->dimension.y };
            m_editorCallsign->setText(GameState.player_callsign);
            m_editorCallsign->setActive(true, &r);
            break;
    }
}

void SCRegister::commitActiveEditor() {
    switch (m_activeEditor) {
        case 1:
            GameState.player_firstname = m_editorFirstName->getText();
            break;
        case 0:
            GameState.player_name = m_editorName->getText();
            break;
        case 2:
            GameState.player_callsign = m_editorCallsign->getText();
            break;
    }
}

void SCRegister::checkKeyboard(void) {
    checkZones();
    if (!m_keyboard) return;
    if (m_tabSuppressFrames > 0) m_tabSuppressFrames--;
    
    commitActiveEditor();

    if (m_tabSuppressFrames == 0 && m_keyboard->isActionJustPressed(InputAction::KEY_TAB)) {
        int next = (m_activeEditor + 1) % 3;
        activateEditor(next);
        m_tabSuppressFrames = 1;
    }

    if (m_keyboard->isActionJustPressed(InputAction::KEY_RETURN)) {
        if (!GameState.player_callsign.empty() &&
            !GameState.player_firstname.empty() &&
            !GameState.player_name.empty()) {

            for (int i=0; i<256; i++)
                GameState.requierd_flags[i] = false;

            m_editorFirstName->setActive(false);
            m_editorName->setActive(false);
            m_editorCallsign->setActive(false);

            stop();
        } else {
            if (GameState.player_firstname.empty())
                activateEditor(0);
            else if (GameState.player_name.empty())
                activateEditor(1);
            else if (GameState.player_callsign.empty())
                activateEditor(2);
            else
                activateEditor( (m_activeEditor+1)%3 );
        }
    }
}
void SCRegister::checkZones() {
    for (auto zone : zones) {
        if (zone->active) {
            if (Mouse.getPosition().x > zone->position.x &&
                Mouse.getPosition().x < zone->position.x + zone->dimension.x &&
                Mouse.getPosition().y > zone->position.y &&
                Mouse.getPosition().y < zone->position.y + zone->dimension.y) {
                // HIT !
                Mouse.setMode(SCMouse::VISOR);

                if (Mouse.buttons[MouseButton::LEFT].event == MouseButton::PRESSED) {
                    Mouse.buttons[MouseButton::LEFT].event = MouseButton::NONE;
                    zone->onclick(nullptr, zone->id);

                }
                return;
            }
        }
    }

    Mouse.setMode(SCMouse::CURSOR);
}
void SCRegister::runFrame(void) {
    checkButtons();
    checkKeyboard();

    VGA.activate();
    FrameBuffer *fb = VGA.getFrameBuffer();
    fb->clear();
    VGA.setPalette(&this->palette);

    
    fb->drawShape(&book);

    std::string firstNameDisplay = (m_activeEditor==1) ? m_editorFirstName->getDisplayText()
                                                       : GameState.player_firstname;
    std::string nameDisplay      = (m_activeEditor==0) ? m_editorName->getDisplayText()
                                                       : GameState.player_name;
    std::string callsignDisplay  = (m_activeEditor==2) ? m_editorCallsign->getDisplayText()
                                                       : GameState.player_callsign;

    fb->printText(this->font, {172,104}, firstNameDisplay, 0);
    fb->printText(this->font, {88,104},  nameDisplay, 0);
    fb->printText(this->font, {122,110}, callsignDisplay, 0);

    Mouse.draw();

    VGA.vSync();
}