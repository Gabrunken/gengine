#ifndef GENGINE_SPRITE_SYSTEMS_H_
#define GENGINE_SPRITE_SYSTEMS_H_
#include <gengine.h>
#include <default_components.h>

void SpriteStartUp();
void SpriteFrameStart();
void SpriteSystem(GameObjectID gameObjectID, void** components);
void SpriteFrameEnd();
void SpriteCleanUp();

#endif
