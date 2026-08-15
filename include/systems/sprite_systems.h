#ifndef GENGINE_SPRITE_SYSTEMS_H_
#define GENGINE_SPRITE_SYSTEMS_H_
#include <gengine.h>
#include <default_components.h>

void SpriteInitializeBuffers();
void SpriteLogicSystem(GameObjectID gameObjectID, void** components);
void SpritePrepareRendering();
void SpriteFlushRendering(); //Call this manually in the render pass of the engine.
void SpriteFreeBuffers();

#endif
