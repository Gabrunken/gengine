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
//Returns channel idx (returns 0 on failure)
// IMPORTANT NOTE, IDX RIGHT NOW ARE NOT STABLE, THEY ARE PHYSICAL INDICES SO IF YOU REMOVE A CHANNEL THEY WILL FUCK UP,
// SAME FOR THE KEYFRAMES
uint32_t GEngineAddAnimationChannel(Animation* animation, GEngineComponentTypeID targetComponent, const char* targetField);
//Same here
void GEngineRemoveAnimationChannel(Animation* animation, uint32_t channelIdx);
//Returns keyframe idx (returns 0 on failure)
uint32_t GEngineAnimationAddKeyframe(Animation* animation, uint32_t channelIdx, void* data, float timestamp, InterpolationType interpolationType);

void GEngineAnimationRemoveKeyframe(Animation* animation, uint32_t channelIdx, uint32_t keyframeIdx);

void GEngineAnimationModifyKeyframeData(Animation* animation, uint32_t channelIdx, uint32_t keyframeIdx, void* newData);

#endif
