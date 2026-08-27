#ifndef GENGINE_SPRITE_SYSTEM_H_
#define GENGINE_SPRITE_SYSTEM_H_
#include "raylib.h"
#include <gengine.h>

typedef struct
{
    Texture2D spriteSheet;
    Rectangle rect;
} GEngineSpriteSheetEntry;

void SpriteStartUp();
void SpriteFrameStart();
void SpriteSystem(GameObjectID gameObjectID, void** components);
void SpriteFrameEnd();
void SpriteCleanUp();

#endif
