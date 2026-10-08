#ifndef GENGINE_ANIMATION_PLAYER_H_
#define GENGINE_ANIMATION_PLAYER_H_

#include <gengine_types.h>
#include <default_components.h>

void AnimationPlayerStartUp();
void AnimationPlayerFrameStart();
void AnimationPlayerSystem(GameObjectID gameObjectID, void** components);
void AnimationPlayerFrameEnd();
void AnimationPlayerCleanUp();

//Allocate a new Animation resource
Animation GEngineCreateAnimation();
//You must call this when you're done with an animation
void GEngineFreeAnimation(Animation* animation);
//Returns channel idx
uint32_t GEngineAddAnimationChannel(Animation* animation, GEngineComponentTypeID targetComponent, const char* targetField);
//Same here
void GEngineRemoveAnimationChannel(Animation* animation, uint32_t channelIdx);
//Returns keyframe idx
uint32_t GEngineAnimationAddKeyframe(Animation* animation, uint32_t channelIdx, void* data, float timestamp);

void GEngineAnimationRemoveKeyframe(Animation* animation, uint32_t channelIdx, uint32_t keyframeIdx);

void GEngineAnimationModifyKeyframe(Animation* animation, uint32_t channelIdx, uint32_t keyframeIdx, void* newData);

#endif
