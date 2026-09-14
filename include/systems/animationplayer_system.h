#ifndef GENGINE_ANIMATION_PLAYER_H_
#define GENGINE_ANIMATION_PLAYER_H_

#include <gengine_types.h>

void AnimationPlayerStartUp();
void AnimationPlayerFrameStart();
void AnimationPlayerSystem(GameObjectID gameObjectID, void** components);
void AnimationPlayerFrameEnd();
void AnimationPlayerCleanUp();

#endif
