#include "precomp.h"
#include "../engine/EventManager.h"
#include <boost/filesystem.hpp>


void SCFileRequester::cancel() {
    this->opened = false;
}

SCFileRequester::SCFileRequester(std::function<void(std::string)> callback, uint8_t shape_id_offset)  {
    this->callback = callback;
    this->shape_id_offset = shape_id_offset;
    RSImageSet *uiImageSet = new RSImageSet();
    TreEntry *tre = Assets.GetEntryByName("..\\..\\DATA\\GAMEFLOW\\SAVELOAD.SHP");
    PakArchive *pak = new PakArchive();
    pak->InitFromRAM("SAVELOAD.SHP", tre->data, tre->size);
    uiImageSet->InitFromPakArchive(pak,0);
    this->uiImageSet = uiImageSet;
    this->color_offset = 49;
    SCButton *button;

    Point2D frp = {(320-uiImageSet->GetShape(0)->GetWidth())/2, (200-uiImageSet->GetShape(0)->GetHeight())/2};
    Point2D buttonDimension = {75, 12};
    Point2D loadGamePosition = {frp.x+54, frp.y+97};
    Point2D cancelPosition = {frp.x+10, frp.y+112};
    Point2D quitToDosPosition = {frp.x+95, frp.y+112};

    Point2D arrowDimentions = {20, 11};
    Point2D fileUp = {frp.x+9, frp.y+41};
    Point2D fileDown = {frp.x+9, frp.y+83};

    Point2D loadTitlePosition = {frp.x+36, frp.y+6};
    uiImageSet->GetShape(1)->position=loadTitlePosition;

    button = new SCButton();
    button->initBehavior(std::bind(&SCFileRequester::loadFile, this), loadGamePosition, buttonDimension);
    button->appearance[SCButton::APR_UP]=*uiImageSet->GetShape(shape_id_offset+4);
    button->appearance[SCButton::APR_UP].position=loadGamePosition;
    button->appearance[SCButton::APR_DOWN]=*uiImageSet->GetShape(shape_id_offset+5);
    button->appearance[SCButton::APR_DOWN].position=loadGamePosition;
    button->setEnable(true);
    buttons.push_back(button);

    button = new SCButton();
    button->initBehavior(std::bind(&SCFileRequester::cancel, this), cancelPosition, buttonDimension);
    button->appearance[SCButton::APR_UP]=*uiImageSet->GetShape(shape_id_offset+6);
    button->appearance[SCButton::APR_UP].position=cancelPosition;
    button->appearance[SCButton::APR_DOWN]=*uiImageSet->GetShape(shape_id_offset+7);
    button->appearance[SCButton::APR_DOWN].position=cancelPosition;
    button->setEnable(true);
    buttons.push_back(button);

    button = new SCButton();
    button->initBehavior(nullptr, quitToDosPosition, buttonDimension);
    button->appearance[SCButton::APR_UP]=*uiImageSet->GetShape(shape_id_offset+8);
    button->appearance[SCButton::APR_UP].position=quitToDosPosition;
    button->appearance[SCButton::APR_DOWN]=*uiImageSet->GetShape(shape_id_offset+9);
    button->appearance[SCButton::APR_DOWN].position=quitToDosPosition;
    button->setEnable(true);
    buttons.push_back(button);

    button = new SCButton();
    button->initBehavior(std::bind(&SCFileRequester::fileUp, this), fileUp, arrowDimentions);
    button->appearance[SCButton::APR_UP]=*uiImageSet->GetShape(shape_id_offset+10);
    button->appearance[SCButton::APR_UP].position=fileUp;
    button->appearance[SCButton::APR_DOWN]=*uiImageSet->GetShape(shape_id_offset+11);
    button->appearance[SCButton::APR_DOWN].position=fileUp;
    button->setEnable(true);
    buttons.push_back(button);

    button = new SCButton();
    button->initBehavior(std::bind(&SCFileRequester::fileDown, this), fileDown, arrowDimentions);
    button->appearance[SCButton::APR_UP]=*uiImageSet->GetShape(shape_id_offset+12);
    button->appearance[SCButton::APR_UP].position=fileDown;
    button->appearance[SCButton::APR_DOWN]=*uiImageSet->GetShape(shape_id_offset+13);
    button->appearance[SCButton::APR_DOWN].position=fileDown;
    button->setEnable(true);
    buttons.push_back(button);

    this->font = FontManager.GetFont("..\\..\\DATA\\FONTS\\SM-FONT.SHP");
    TreEntry *optPalettesEntry = Assets.GetEntryByName(Assets.optpals_filename);
    
    optPals.InitFromRAM("OPTPALS.PAK", optPalettesEntry->data, optPalettesEntry->size);
    rawpal = optPals.GetEntry(4)->data;
}

SCFileRequester::SCFileRequester(std::function<void(std::string)> callback, uint8_t shape_id_offset, bool save)  {
    this->callback = callback;
    this->save_mode = save;
    this->shape_id_offset = shape_id_offset;
    RSImageSet *uiImageSet = new RSImageSet();
    TreEntry *tre = Assets.GetEntryByName("..\\..\\DATA\\GAMEFLOW\\SAVELOAD.SHP");
    PakArchive *pak = new PakArchive();
    pak->InitFromRAM("SAVELOAD.SHP", tre->data, tre->size);
    uiImageSet->InitFromPakArchive(pak,0);
    this->uiImageSet = uiImageSet;
    this->color_offset = 49;
    SCButton *button;

    Point2D frp = {(320-uiImageSet->GetShape(0)->GetWidth())/2, (200-uiImageSet->GetShape(0)->GetHeight())/2};
    Point2D buttonDimension = {75, 12};
    Point2D loadGamePosition = {frp.x+54, frp.y+97};
    Point2D cancelPosition = {frp.x+10, frp.y+112};
    Point2D quitToDosPosition = {frp.x+95, frp.y+112};

    Point2D arrowDimentions = {20, 11};
    Point2D fileUp = {frp.x+9, frp.y+41};
    Point2D fileDown = {frp.x+9, frp.y+83};

    Point2D loadTitlePosition = {frp.x+36, frp.y+6};
    uiImageSet->GetShape(1)->position=loadTitlePosition;

    button = new SCButton();
    button->initBehavior(std::bind(&SCFileRequester::loadFile, this), loadGamePosition, buttonDimension);
    button->appearance[SCButton::APR_UP]=*uiImageSet->GetShape(shape_id_offset+2);
    button->appearance[SCButton::APR_UP].position=loadGamePosition;
    button->appearance[SCButton::APR_DOWN]=*uiImageSet->GetShape(shape_id_offset+3);
    button->appearance[SCButton::APR_DOWN].position=loadGamePosition;
    button->setEnable(true);
    buttons.push_back(button);

    button = new SCButton();
    button->initBehavior(std::bind(&SCFileRequester::cancel, this), cancelPosition, buttonDimension);
    button->appearance[SCButton::APR_UP]=*uiImageSet->GetShape(shape_id_offset+6);
    button->appearance[SCButton::APR_UP].position=cancelPosition;
    button->appearance[SCButton::APR_DOWN]=*uiImageSet->GetShape(shape_id_offset+7);
    button->appearance[SCButton::APR_DOWN].position=cancelPosition;
    button->setEnable(true);
    buttons.push_back(button);

    button = new SCButton();
    button->initBehavior(nullptr, quitToDosPosition, buttonDimension);
    button->appearance[SCButton::APR_UP]=*uiImageSet->GetShape(shape_id_offset+8);
    button->appearance[SCButton::APR_UP].position=quitToDosPosition;
    button->appearance[SCButton::APR_DOWN]=*uiImageSet->GetShape(shape_id_offset+9);
    button->appearance[SCButton::APR_DOWN].position=quitToDosPosition;
    button->setEnable(true);
    buttons.push_back(button);

    button = new SCButton();
    button->initBehavior(std::bind(&SCFileRequester::fileUp, this), fileUp, arrowDimentions);
    button->appearance[SCButton::APR_UP]=*uiImageSet->GetShape(shape_id_offset+10);
    button->appearance[SCButton::APR_UP].position=fileUp;
    button->appearance[SCButton::APR_DOWN]=*uiImageSet->GetShape(shape_id_offset+11);
    button->appearance[SCButton::APR_DOWN].position=fileUp;
    button->setEnable(true);
    buttons.push_back(button);

    button = new SCButton();
    button->initBehavior(std::bind(&SCFileRequester::fileDown, this), fileDown, arrowDimentions);
    button->appearance[SCButton::APR_UP]=*uiImageSet->GetShape(shape_id_offset+12);
    button->appearance[SCButton::APR_UP].position=fileDown;
    button->appearance[SCButton::APR_DOWN]=*uiImageSet->GetShape(shape_id_offset+13);
    button->appearance[SCButton::APR_DOWN].position=fileDown;
    button->setEnable(true);
    buttons.push_back(button);

    this->font = FontManager.GetFont("..\\..\\DATA\\FONTS\\SM-FONT.SHP");
    TreEntry *optPalettesEntry = Assets.GetEntryByName(Assets.optpals_filename);
    
    optPals.InitFromRAM("OPTPALS.PAK", optPalettesEntry->data, optPalettesEntry->size);
    rawpal = optPals.GetEntry(4)->data;

    Point2D dialog = this->dialogCorner();
    Point2D yesPosition = { dialog.x + 18, dialog.y + 25 };
    Point2D noPosition = { dialog.x + 69, dialog.y + 25 };
    button = new SCButton();
    button->initBehavior(std::bind(&SCFileRequester::overwriteYes, this), yesPosition, { 46, 12 });
    button->appearance[SCButton::APR_UP] = *uiImageSet->GetShape(shape_id_offset + 17);
    button->appearance[SCButton::APR_UP].position = yesPosition;
    button->appearance[SCButton::APR_DOWN] = *uiImageSet->GetShape(shape_id_offset + 18);
    button->appearance[SCButton::APR_DOWN].position = yesPosition;
    button->setEnable(true);
    this->confirm_buttons.push_back(button);

    button = new SCButton();
    button->initBehavior(std::bind(&SCFileRequester::overwriteNo, this), noPosition, { 44, 12 });
    button->appearance[SCButton::APR_UP] = *uiImageSet->GetShape(shape_id_offset + 19);
    button->appearance[SCButton::APR_UP].position = noPosition;
    button->appearance[SCButton::APR_DOWN] = *uiImageSet->GetShape(shape_id_offset + 20);
    button->appearance[SCButton::APR_DOWN].position = noPosition;
    button->setEnable(true);
    this->confirm_buttons.push_back(button);
}

SCFileRequester::~SCFileRequester() {
    if (this->m_isEditingText) {
        EventManager::getInstance().stopTextInput();
        this->m_isEditingText = false;
    }
    delete this->uiImageSet;
}
// Positions dans le cadre (pixels depuis son coin) : zone de texte x 32 à 152, y 27 à 35 ; liste x 32 à 152, y 42 à 90
// (6 lignes de 8) ; rail de l'ascenseur x 14 à 23, y 54 à 80.
Point2D SCFileRequester::frameCorner() {
    RLEShape *shape = this->uiImageSet->GetShape(this->shape_id_offset + 0);
    Point2D corner = { (320 - shape->GetWidth()) / 2, (200 - shape->GetHeight()) / 2 };
    return corner;
}

void SCFileRequester::draw(FrameBuffer *fb) {
    checkButtons();
    checkZones();
    FrameBuffer *fb2 = new FrameBuffer(320, 200);
    fb2->fillWithColor(0);
    RLEShape *shape = this->uiImageSet->GetShape(shape_id_offset+0);
    fb2->drawShape(shape);
    
    fb->blitLargeBuffer(fb2->framebuffer, 320, 200, shape->GetLeft(), shape->GetTop(), (320-shape->GetWidth())/2, (200-shape->GetHeight())/2, 182, 131);
    if (!this->save_mode) {
        RLEShape *shape2 = this->uiImageSet->GetShape(this->shape_id_offset+1);
        Point2D shape2Pos = {
            0,
            0
        };
        shape2->SetPosition(&shape2Pos);
        fb2->fillWithColor(255);
        fb2->drawShape(shape2);
        fb->blitLargeBuffer(
            fb2->framebuffer,
            320,
            200,
            0,
            0,
            (320-shape->GetWidth())/2+(shape2->GetWidth()/2)-18,
            (200-shape->GetHeight())/2+(shape2->GetHeight()/2)+1,
            107,
            12
        );
    }
    delete fb2;
    for (auto btn: this->buttons) {
        fb->drawShape(&btn->appearance[btn->getAppearance()]);
    }
    Point2D corner = this->frameCorner();
    std::string text = this->current_file;
    if (this->m_isEditingText && (SDL_GetTicks() / 250) % 2 == 0) {
        text += "_";
    }
    // Lettres de SM-FONT.SHP en couleur 15 : décalage = couleur voulue - 15. Zone de texte : couleur 0
    // (PaletteScreen_DispatchWidgetLayoutB_11363 appelé avec 0) ; liste : texte noir, ligne choisie en blanc sur noir
    // (FileList_Draw_4C4C6, couleurs de UiColors_Setup_FBE7).
    uint8_t black = this->nearestColor(0, 0, 0);
    uint8_t white = this->nearestColor(63, 63, 63);
    fb->printText(this->font, { corner.x + 34, corner.y + 29 }, text, (uint8_t)(0 - 15));

    for (int row = 0; row < VISIBLE_FILES; row++) {
        int index = this->first_file + row;
        if (index >= (int)this->files.size()) {
            break;
        }
        int y = corner.y + 42 + row * ROW_HEIGHT;
        if (index == this->selectd_file_index) {
            for (int line = y - 1; line < y + ROW_HEIGHT - 1; line++) {
                fb->line(corner.x + 32, line, corner.x + 152, line, black);
            }
            fb->printText(this->font, { corner.x + 34, y }, this->files[index], (uint8_t)(white - 15));
        } else {
            fb->printText(this->font, { corner.x + 34, y }, this->files[index], (uint8_t)(black - 15));
        }
    }

    RLEShape *thumb = this->uiImageSet->GetShape(this->shape_id_offset + 14);
    int max_first = (int)this->files.size() - VISIBLE_FILES;
    int thumb_y = corner.y + 54;
    if (max_first > 0) {
        thumb_y += (80 - 54 - 4) * this->first_file / max_first;
    }
    Point2D thumb_position = { corner.x + 14, thumb_y };
    thumb->SetPosition(&thumb_position);
    fb->drawShape(thumb);

    if (this->confirm_overwrite) {
        Point2D dialog = this->dialogCorner();
        RLEShape *box = this->uiImageSet->GetShape(this->shape_id_offset + 15);
        box->SetPosition(&dialog);
        fb->drawShape(box);
        RLEShape *overwrite = this->uiImageSet->GetShape(this->shape_id_offset + 16);
        Point2D overwrite_position = { dialog.x + 18, dialog.y + 3 };
        overwrite->SetPosition(&overwrite_position);
        fb->drawShape(overwrite);
        for (auto btn : this->confirm_buttons) {
            fb->drawShape(&btn->appearance[btn->getAppearance()]);
        }
    }
}

uint8_t SCFileRequester::nearestColor(int r, int g, int b) {
    VGAPalette *palette = VGA.getPalette();
    int best = 0x7FFF;
    uint8_t found = 0;
    for (int i = 0; i < 256; i++) {
        Texel *texel = palette->GetRGBColor((uint8_t)i);
        int dr = std::abs(r - (texel->r * 63 + 127) / 255);
        int dg = std::abs(g - (texel->g * 63 + 127) / 255);
        int db = std::abs(b - (texel->b * 63 + 127) / 255);
        if (dr + dg + db < best) {
            best = dr + dg + db;
            found = (uint8_t)i;
        }
    }
    return found;
}

// Dialogue (image 15 du jeu, 132 x 44) centré à l'écran.
Point2D SCFileRequester::dialogCorner() {
    Point2D corner = { (320 - 132) / 2, (200 - 44) / 2 };
    return corner;
}
void SCFileRequester::checkevents() {
    // Texteingabe ueber den EventManager: Er leert die SDL-Ereigniswarteschlange, SDL_PeepEvents sieht keine Tasten mehr.
    // Der InputActionSystem-Textmodus wird bewusst nicht benutzt (er friert die Maus ein).
    EventManager &em = EventManager::getInstance();
    if (this->confirm_overwrite) {
        if (em.isKeyJustPressed(SDL_SCANCODE_ESCAPE)) {
            this->overwriteNo();
        }
        return;
    }
    if (em.isKeyJustPressed(SDL_SCANCODE_ESCAPE)) {
        if (this->m_isEditingText) {
            this->deactivateTextEditor();
        } else {
            this->opened = false;
        }
    }
    if (em.isKeyJustPressed(SDL_SCANCODE_KP_PLUS)) {
        this->color_offset = (this->color_offset + 1) % 256;
    }
    if (em.isKeyJustPressed(SDL_SCANCODE_KP_MINUS)) {
        this->color_offset = (this->color_offset + 255) % 256;
    }
    if (em.isKeyJustPressed(SDL_SCANCODE_KP_MULTIPLY)) {
        this->palette_index = (this->palette_index + 1) % 40;
    }
    if (em.isKeyJustPressed(SDL_SCANCODE_KP_DIVIDE)) {
        this->palette_index = (this->palette_index + 39) % 40;
    }
    if (!this->m_isEditingText) {
        return;
    }
    if (em.isKeyJustPressed(SDL_SCANCODE_RETURN) || em.isKeyJustPressed(SDL_SCANCODE_KP_ENTER)) {
        this->deactivateTextEditor();
        return;
    }
    if (em.isKeyJustPressed(SDL_SCANCODE_BACKSPACE) || em.isKeyJustPressed(SDL_SCANCODE_DELETE)) {
        if (!this->current_file.empty()) {
            this->current_file.pop_back();
        }
    }
    for (const std::string &txt : em.getTextInput()) {
        for (char c : txt) {
            if (c >= 97 && c <= 122) {
                this->current_file += (char)(c - 97 + 65);
            } else if ((c >= 65 && c <= 90) || (c >= 48 && c <= 57) || c == 46) {
                this->current_file += c;
            }
        }
    }
}

// Clic sur la zone de texte : la saisie va au nom de fichier.
void SCFileRequester::activateTextEditor(void *unused, int id) {
    EventManager &em = EventManager::getInstance();
    // Callbacks eines frueher zerstoerten TextEditors (z.B. aus SCRegister) entfernen, sonst Absturz beim ersten Zeichen
    em.setTextInputCallback(nullptr);
    em.setKeyPressCallback(nullptr);
    em.startTextInput();
    this->m_isEditingText = true;
    this->selectd_file_index = -1;
    printf("REQUESTER: saisie du nom de fichier\n");
}

void SCFileRequester::deactivateTextEditor() {
    if (!this->m_isEditingText) {
        return;
    }
    EventManager::getInstance().stopTextInput();
    this->m_isEditingText = false;
    printf("REQUESTER: fin de saisie, nom %s\n", this->current_file.c_str());
}

SCButton *SCFileRequester::checkButtons(void) {
    std::vector<SCButton *> &active = this->confirm_overwrite ? this->confirm_buttons : this->buttons;
    for (size_t i = 0; i < active.size(); i++) {

        SCButton *button = active[i];

        if (!button->isEnabled())
            continue;

        if (Mouse.getPosition().x < button->position.x ||
            Mouse.getPosition().x > button->position.x + button->dimension.x ||
            Mouse.getPosition().y < button->position.y ||
            Mouse.getPosition().y > button->position.y + button->dimension.y) {
            button->setAppearance(SCButton::APR_UP);
            continue;
        }
        // HIT !
        Mouse.setMode(SCMouse::VISOR);

        if (Mouse.buttons[MouseButton::LEFT].event == MouseButton::PRESSED)
            button->setAppearance(SCButton::APR_DOWN);

        // If the mouse button has just been released: trigger action.
        if (Mouse.buttons[MouseButton::LEFT].event == MouseButton::RELEASED)
            button->onAction();

        return button;
    }

    Mouse.setMode(SCMouse::CURSOR);
    return NULL;
}
void SCFileRequester::loadFiles() {
    files.clear();
    files.shrink_to_fit();
    this->first_file = 0;
    this->selectd_file_index = -1;
    for (const auto &entry : boost::filesystem::directory_iterator(".")) {
        if (entry.is_regular_file()) {
            std::string fileName = entry.path().filename().string();
            if (fileName.size() >= 4 && (fileName.substr(fileName.size() - 4) == ".sav" || fileName.substr(fileName.size() - 4) == ".SAV")) {
                files.push_back(fileName);
            }
        }
    }
    printf("REQUESTER: %zu fichiers de sauvegarde\n", this->files.size());
}
void SCFileRequester::fileUp() {
    if (this->first_file > 0) {
        this->first_file--;
    }
}
void SCFileRequester::fileDown() {
    if (this->first_file + VISIBLE_FILES < (int)this->files.size()) {
        this->first_file++;
    }
}
void SCFileRequester::scrollFromMouse(int mouse_y) {
    int max_first = (int)this->files.size() - VISIBLE_FILES;
    if (max_first <= 0) {
        this->first_file = 0;
        return;
    }
    Point2D corner = this->frameCorner();
    int track = mouse_y - (corner.y + 54);
    if (track < 0) {
        track = 0;
    }
    if (track > 80 - 54 - 4) {
        track = 80 - 54 - 4;
    }
    this->first_file = (track * max_first + (80 - 54 - 4) / 2) / (80 - 54 - 4);
}
// Liste, zone de texte et rail de l'ascenseur (frameCorner).
SCZone * SCFileRequester::checkZones() {
    if (this->confirm_overwrite) {
        return nullptr;
    }
    Point2D corner = this->frameCorner();
    Point2D mouse = Mouse.getPosition();
    MouseButton::EventType event = Mouse.buttons[MouseButton::LEFT].event;
    if (this->dragging_thumb) {
        this->scrollFromMouse(mouse.y);
        if (event == MouseButton::RELEASED) {
            this->dragging_thumb = false;
        }
        return nullptr;
    }
    bool in_track = mouse.x >= corner.x + 14 && mouse.x <= corner.x + 23 && mouse.y >= corner.y + 54 && mouse.y <= corner.y + 80;
    if (in_track) {
        Mouse.setMode(SCMouse::VISOR);
        if (event == MouseButton::PRESSED) {
            this->dragging_thumb = true;
            this->scrollFromMouse(mouse.y);
        }
        return nullptr;
    }
    bool in_text = mouse.x >= corner.x + 32 && mouse.x <= corner.x + 152 && mouse.y >= corner.y + 27 && mouse.y <= corner.y + 35;
    if (in_text) {
        Mouse.setMode(SCMouse::VISOR);
        if (event == MouseButton::RELEASED) {
            this->activateTextEditor(nullptr, 0);
        }
        return nullptr;
    }
    bool in_list = mouse.x >= corner.x + 32 && mouse.x <= corner.x + 152 && mouse.y >= corner.y + 41 && mouse.y < corner.y + 41 + VISIBLE_FILES * ROW_HEIGHT;
    if (in_list) {
        int index = this->first_file + (mouse.y - (corner.y + 41)) / ROW_HEIGHT;
        if (index < (int)this->files.size()) {
            Mouse.setMode(SCMouse::VISOR);
            if (event == MouseButton::RELEASED) {
                this->selectFile(nullptr, index);
            }
        }
    }
    return nullptr;
}
void SCFileRequester::selectFile(void *unused, int index) {
    this->deactivateTextEditor();
    current_file = files[index];
    this->selectd_file_index = index;
    printf("REQUESTER: fichier %d choisi, %s\n", index, current_file.c_str());
}
void SCFileRequester::loadFile() {
    if (this->save_mode && this->fileExists(this->current_file)) {
        this->deactivateTextEditor();
        this->confirm_overwrite = true;
        printf("REQUESTER: %s existe, demande de confirmation\n", this->current_file.c_str());
        return;
    }
    this->saveRequested();
}

void SCFileRequester::saveRequested() {
    this->requested_file = this->current_file;
    this->opened = false;
    this->callback(this->requested_file);
}

// Nom complété de .SAV comme SCGameFlow::saveGame, comparé sans tenir compte des majuscules.
bool SCFileRequester::fileExists(const std::string &name) {
    if (name.empty()) {
        return false;
    }
    std::string wanted = name;
    std::transform(wanted.begin(), wanted.end(), wanted.begin(), ::toupper);
    if (wanted.size() < 4 || wanted.substr(wanted.size() - 4) != ".SAV") {
        wanted += ".SAV";
    }
    for (const std::string &file : this->files) {
        std::string upper = file;
        std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
        if (upper == wanted) {
            return true;
        }
    }
    return false;
}

void SCFileRequester::overwriteYes() {
    this->confirm_overwrite = false;
    printf("REQUESTER: remplacement de %s confirme\n", this->current_file.c_str());
    this->saveRequested();
}

void SCFileRequester::overwriteNo() {
    this->confirm_overwrite = false;
    printf("REQUESTER: remplacement de %s annule\n", this->current_file.c_str());
}
