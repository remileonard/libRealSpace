//
//  NSObjectViewer.cpp
//  libRealSpace
//
//  Created by fabien sanglard on 1/3/2014.
//  Copyright (c) 2014 Fabien Sanglard. All rights reserved.
//

#include "precomp.h"

SCObjectViewer::SCObjectViewer() {
    this->rotateUpDownAngle = 0;
    this->zoomFactor = 1;
    this->rotateLeftRightAngle = 0;
    this->music_playing = false;
}

SCObjectViewer::~SCObjectViewer() {}

void ConvertToUpperCase(char *sPtr) {
    while (*sPtr != '\0') {
        if (islower(*sPtr))
            *sPtr = toupper(*sPtr);
        sPtr++;
    }
}

void SCObjectViewer::ParseObjList(IffLexer *lexer) {

    // The object all follow the same path:
    const char *OBJ_PATH = "..\\..\\DATA\\OBJECTS\\";
    const char *OBJ_EXTENSION = ".IFF";

    IffChunk *chunk = lexer->GetChunkByID('OBJS');
    if (chunk == NULL) {
        printf("**Error: Cannot parse Object List (Missing OBJS chunk).\n");
        return;
    }

    ByteStream stream(chunk->data);

    size_t numObjectInList = chunk->size / 33;

    for (int objIndex = 0; objIndex < numObjectInList; objIndex++) {

        RSShowCase showCase;

        char objName[9];
        for (int k = 0; k < 9; k++)
            objName[k] = stream.ReadByte();
        ConvertToUpperCase(objName);

        for (int k = 0; k < 20; k++)
            showCase.displayName[k] = stream.ReadByte();

        char modelPath[512];
        strcpy(modelPath, OBJ_PATH);
        strcat(modelPath, objName);
        strcat(modelPath, OBJ_EXTENSION);
        TreEntry *entry = Assets.GetEntryByName(modelPath);

        if (entry == NULL) {
            printf("Object reference '%s' not found in TRE.\n", modelPath);
            continue;
        }

        showCase.entity = new RSEntity(&Assets);
        showCase.entity->InitFromRAM(entry->data, entry->size);

        uint32_t fixedPointDist = stream.ReadInt32LE();
        showCase.cameraDist = (fixedPointDist >> 8) + (fixedPointDist & 0xFF) / 255.0f;
        // showCase.cameraDist = 200000;
        showCases.push_back(showCase);
    }

    char *smoke = "..\\..\\DATA\\OBJECTS\\SMOKEGEN.IFF";
    TreEntry *smoke_entry = Assets.GetEntryByName(smoke);
    RSShowCase smokeShowCase;
    smokeShowCase.entity = new RSEntity(&Assets);
    smokeShowCase.entity->InitFromRAM(smoke_entry->data, smoke_entry->size);
    smokeShowCase.cameraDist=10;
    showCases.push_back(smokeShowCase);

}

void SCObjectViewer::OnExit(void) { Game.StopTopActivity(); }

void SCObjectViewer::OnNext(void) {
    // startTime = SDL_GetTicks();
    SCObjectViewer *that = (SCObjectViewer *)Game.GetCurrentActivity();
    that->NextObject();
}

void SCObjectViewer::OnZoomOut(void) {
    SCObjectViewer *that = (SCObjectViewer *)Game.GetCurrentActivity();
    that->zoomFactor += 0.1f;
    if (that->zoomFactor > 2.5f)
        that->zoomFactor = 2.5f;
}

void SCObjectViewer::OnZoomIn(void) {
    SCObjectViewer *that = (SCObjectViewer *)Game.GetCurrentActivity();
    that->zoomFactor -= 0.1f;
    if (that->zoomFactor < 0.4f)
        that->zoomFactor = 0.4f;
}

void SCObjectViewer::OnRotateLeft(void) {
    SCObjectViewer *that = (SCObjectViewer *)Game.GetCurrentActivity();
    that->rotateLeftRightAngle -= 10.0f;
}

void SCObjectViewer::OnRotateRight(void) {
    SCObjectViewer *that = (SCObjectViewer *)Game.GetCurrentActivity();
    that->rotateLeftRightAngle += 10.0f;
}

void SCObjectViewer::OnRotateUp(void) {
    SCObjectViewer *that = (SCObjectViewer *)Game.GetCurrentActivity();
    that->rotateUpDownAngle += 10.0f;
}

void SCObjectViewer::OnRotateDown(void) {
    SCObjectViewer *that = (SCObjectViewer *)Game.GetCurrentActivity();
    that->rotateUpDownAngle -= 10.0f;
}

void SCObjectViewer::NextObject(void) { currentObject = (currentObject + 1) % showCases.size(); }

void SCObjectViewer::Focus(void) {
    this->focused = true;
    if (!this->music_playing) {
        Mixer.SwitchBank(0);
        Mixer.PlayMusic(25);
        this->music_playing = true;
    }
}
void SCObjectViewer::UnFocus(void) {
    this->focused = false;
    if (this->music_playing) {
        Mixer.StopMusic();
        this->music_playing = false;
    }
}

void SCObjectViewer::ParseAssets(PakArchive *archive) {

    PakEntry *entry0 = archive->GetEntry(0);
    PakArchive file0;
    file0.InitFromRAM("OBJVIEW.PAK: file 0", entry0->data, entry0->size);
    title.Init(file0.GetEntry(0)->data, file0.GetEntry(0)->size);

    PakEntry *objButtonEntry = archive->GetEntry(4);
    PakArchive objButtons;
    objButtons.InitFromRAM("OBJVIEW.PAK: file 4", objButtonEntry->data, objButtonEntry->size);
    objButtons.List(stdout);

    SCButton *button;
    Point2D boardPosition = {4, 155};

    // EXIT BUTTON
    button = new SCButton();
    Point2D exitDimension = {30, 15};
    Point2D exitPosition = {boardPosition.x + 268, boardPosition.y + 15};
    button->InitBehavior(std::bind(&SCObjectViewer::OnExit, this), exitPosition, exitDimension);
    button->appearance[SCButton::APR_UP].InitWithPosition(objButtons.GetEntry(14)->data, objButtons.GetEntry(14)->size,
                                                          &exitPosition);
    button->appearance[SCButton::APR_DOWN].InitWithPosition(objButtons.GetEntry(15)->data,
                                                            objButtons.GetEntry(15)->size, &exitPosition);
    buttons.push_back(button);

    Point2D arrowDimension = {15, 15};

    // ROT RIGHT OBJ BUTTON
    button = new SCButton();

    Point2D rotRightButtonPosition = {boardPosition.x + 232, boardPosition.y + 12};
    button->InitBehavior(std::bind(&SCObjectViewer::OnRotateRight, this), rotRightButtonPosition, arrowDimension);
    button->appearance[SCButton::APR_UP].InitWithPosition(objButtons.GetEntry(12)->data, objButtons.GetEntry(12)->size,
                                                          &rotRightButtonPosition);
    button->appearance[SCButton::APR_DOWN].InitWithPosition(objButtons.GetEntry(13)->data,
                                                            objButtons.GetEntry(13)->size, &rotRightButtonPosition);
    buttons.push_back(button);

    // ROT LEFT OBJ BUTTON
    button = new SCButton();
    Point2D rotLeftButtonPosition = {boardPosition.x + 174, boardPosition.y + 12};
    button->InitBehavior(std::bind(&SCObjectViewer::OnRotateLeft, this), rotLeftButtonPosition, arrowDimension);
    button->appearance[SCButton::APR_UP].InitWithPosition(objButtons.GetEntry(10)->data, objButtons.GetEntry(10)->size,
                                                          &rotLeftButtonPosition);
    button->appearance[SCButton::APR_DOWN].InitWithPosition(objButtons.GetEntry(11)->data,
                                                            objButtons.GetEntry(11)->size, &rotLeftButtonPosition);
    buttons.push_back(button);

    // ROT DOWN OBJ BUTTON
    button = new SCButton();
    Point2D rotDownButtonPosition = {boardPosition.x + 198, boardPosition.y + 24};
    button->InitBehavior(std::bind(&SCObjectViewer::OnRotateDown, this), rotDownButtonPosition, arrowDimension);
    button->appearance[SCButton::APR_UP].InitWithPosition(objButtons.GetEntry(8)->data, objButtons.GetEntry(8)->size,
                                                          &rotDownButtonPosition);
    button->appearance[SCButton::APR_DOWN].InitWithPosition(objButtons.GetEntry(9)->data, objButtons.GetEntry(9)->size,
                                                            &rotDownButtonPosition);
    buttons.push_back(button);

    // ROT UP OBJ BUTTON
    button = new SCButton();
    Point2D rotUpButtonPosition = {boardPosition.x + 198, boardPosition.y + 6};
    button->InitBehavior(std::bind(&SCObjectViewer::OnRotateUp, this), rotUpButtonPosition, arrowDimension);
    button->appearance[SCButton::APR_UP].InitWithPosition(objButtons.GetEntry(6)->data, objButtons.GetEntry(6)->size,
                                                          &rotUpButtonPosition);
    button->appearance[SCButton::APR_DOWN].InitWithPosition(objButtons.GetEntry(7)->data, objButtons.GetEntry(7)->size,
                                                            &rotUpButtonPosition);
    buttons.push_back(button);

    // ZOOM OUT OBJ BUTTON
    button = new SCButton();
    Point2D zoomOutButtonPosition = {boardPosition.x + 122, boardPosition.y + 25};
    button->InitBehavior(std::bind(&SCObjectViewer::OnZoomOut, this), zoomOutButtonPosition, arrowDimension);
    button->appearance[SCButton::APR_UP].InitWithPosition(objButtons.GetEntry(4)->data, objButtons.GetEntry(4)->size,
                                                          &zoomOutButtonPosition);
    button->appearance[SCButton::APR_DOWN].InitWithPosition(objButtons.GetEntry(5)->data, objButtons.GetEntry(5)->size,
                                                            &zoomOutButtonPosition);
    buttons.push_back(button);

    // ZOOM IN OBJ BUTTON
    button = new SCButton();
    Point2D zoomInButtonPosition = {boardPosition.x + 121, boardPosition.y + 7};
    button->InitBehavior(std::bind(&SCObjectViewer::OnZoomIn, this), zoomInButtonPosition, arrowDimension);
    button->appearance[SCButton::APR_UP].InitWithPosition(objButtons.GetEntry(2)->data, objButtons.GetEntry(2)->size,
                                                          &zoomInButtonPosition);
    button->appearance[SCButton::APR_DOWN].InitWithPosition(objButtons.GetEntry(3)->data, objButtons.GetEntry(3)->size,
                                                            &zoomInButtonPosition);
    buttons.push_back(button);

    // NEXT OBJ BUTTON
    Point2D nextDimension = {75, 15};
    button = new SCButton();
    Point2D nextButtonPosition = {boardPosition.x + 10, boardPosition.y + 15};
    button->InitBehavior(std::bind(&SCObjectViewer::OnNext, this), nextButtonPosition, nextDimension);
    button->appearance[SCButton::APR_UP].InitWithPosition(objButtons.GetEntry(0)->data, objButtons.GetEntry(0)->size,
                                                          &nextButtonPosition);
    button->appearance[SCButton::APR_DOWN].InitWithPosition(objButtons.GetEntry(1)->data, objButtons.GetEntry(1)->size,
                                                            &nextButtonPosition);
    buttons.push_back(button);

    PakEntry *entry8 = archive->GetEntry(8);
    PakArchive file8;
    file8.InitFromRAM("OBJVIEW.PAK: file 8", entry8->data, entry8->size);
    bluePrint.Init(file8.GetEntry(0)->data, file8.GetEntry(0)->size);

    IffLexer lexer;
    FileData *paletteFile = Assets.GetFileData("PALETTE.IFF");
    lexer.InitFromRAM(paletteFile->data, paletteFile->size);

    RSPalette palette;
    palette.InitFromIFF(&lexer);
    this->palette = *palette.GetColorPalette();
}

void SCObjectViewer::Init(void) {

    TreEntry *objViewIFF =
        Assets.GetEntryByName("..\\..\\DATA\\GAMEFLOW\\OBJVIEW.IFF");
    TreEntry *objViewPAK =
        Assets.GetEntryByName("..\\..\\DATA\\GAMEFLOW\\OBJVIEW.PAK");

    PakArchive assets;
    assets.InitFromRAM("OBJVIEW.PAK", objViewPAK->data, objViewPAK->size);
    ParseAssets(&assets);

    IffLexer objToDisplay;
    objToDisplay.InitFromRAM(objViewIFF->data, objViewIFF->size);

    ParseObjList(&objToDisplay);


    currentObject = 0;

    startTime = SDL_GetTicks();
}

void SCObjectViewer::RunFrame(void) {

    CheckButtons();

    VGA.Activate();
    VGA.GetFrameBuffer()->Clear();
    VGA.SetPalette(&this->palette);

    VGA.SetPalette(&this->palette);

    // Draw static
    VGA.GetFrameBuffer()->DrawShape(&bluePrint);

    VGA.VSync();

    /**/
    // Ok now time to draw the model
    glClear(GL_DEPTH_BUFFER_BIT);

    uint32_t currentTime = SDL_GetTicks();
    uint32_t totalTime = currentTime - startTime;

    RSShowCase showCase = showCases[currentObject];

    Point3D newPosition;
    newPosition.x = showCase.cameraDist / 150 * cosf(totalTime / 2000.0f);
    newPosition.y = showCase.cameraDist / 350;
    newPosition.z = showCase.cameraDist / 150 * sinf(totalTime / 2000.0f);

    Renderer.GetCamera()->SetPosition(&newPosition);
    Point3D lookAt = {0, 0, 0};
    Renderer.GetCamera()->LookAt(&lookAt);

    Point3D light;
    light.x = 4 * cosf(-1 * totalTime / 20000.0f);
    light.y = 4;
    light.z = 4 * sinf(-1 * totalTime / 20000.0f);
    Renderer.SetLight(&light);

    glMatrixMode(GL_PROJECTION);
    Matrix *projection = Renderer.GetCamera()->GetProjectionMatrix();
    glLoadMatrixf(projection->ToGL());

    glMatrixMode(GL_MODELVIEW);
    Matrix *view = Renderer.GetCamera()->GetViewMatrix();
    glLoadMatrixf(view->ToGL());
    glPushMatrix();
    glRotatef(this->rotateLeftRightAngle, 1, 0, 0);
    glPushMatrix();
    glRotatef(this->rotateUpDownAngle, 0, 0, 1);

    glScalef(1 / this->zoomFactor, 1 / this->zoomFactor, 1 / this->zoomFactor);
    Renderer.DrawModel(showCases[currentObject].entity, LOD_LEVEL_MAX);
    glPopMatrix();
    glPushMatrix();
    glDisable(GL_DEPTH_TEST);

    VGA.SwithBuffers();
    VGA.Activate();
    VGA.GetFrameBuffer()->Clear();
    VGA.SetPalette(&this->palette);

    VGA.GetFrameBuffer()->DrawShape(&title);

    DrawButtons();

    // Draw Mouse
    Mouse.Draw();

    VGA.VSync();
    VGA.SwithBuffers();
    /**/
}