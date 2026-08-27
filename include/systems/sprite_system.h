#ifndef GENGINE_SPRITE_SYSTEM_H_
#define GENGINE_SPRITE_SYSTEM_H_
#include <gengine.h>

void SpriteStartUp();
void SpriteFrameStart();
void SpriteSystem(GameObjectID gameObjectID, void** components);
void SpriteFrameEnd();
void SpriteCleanUp();

#endif
