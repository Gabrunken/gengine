#include "dyarray.h"
#include "gengine_types.h"
#include "raylib.h"
#include "raymath.h"
#include "sparse_set.h"
#include <animationplayer_system.h>
#include <default_components.h>
#include <gengine.h>

extern GEnginePublicContext _publicContext;

typedef struct
{
	GameObjectID gameObject;
	dyarray /* of uint32_t */ lastKeyframeIdxs; //One for each channel
	float timeProgression; //Seconds from the start of the animation
	bool shouldBePlaying; //Does this animation should be playing right now? Animation ended and no loop, not playing. Animation paused, not playing.
} AnimationPlayingState;

static struct SparseSet animationStates;

void AnimationPlayerStartUp()
{
	SparseSetCreate(&animationStates, 100, sizeof(AnimationPlayingState));
}

void AnimationPlayerFrameStart()
{
	//At each frame start, check the event queue for any ComponentAttached and ComponentDetached events,
	//to check if an "AnimationComponent" has been attached to some entity; if yes,
	//register it in the active PlayingState set.
	//Multiple ComponentAttached and ComponentDetached events might have been pushed in the same
	//frame, because of this, do a final check on that GameObject by seeing if in the ECS it actually has
	//that component or not.

	//Here we check for component detachment (remove from PlayingState set)
	dyarray eventQueue = GEngineGetEventQueue(_publicContext.defaultEventTypes.componentDetached);
	for (size_t i = 0; i < eventQueue.elementCount; i++)
	{
		ComponentDetachedEvent* event = DyArrayGetElement((dyarray*)&eventQueue, i);
		if (event->componentTypeID == _publicContext.defaultComponents.animationPlayer)
		{
			AnimationPlayerComponent* component = GEngineGetComponent(event->gameObjectID, _publicContext.defaultComponents.animationPlayer);
			//Does it ACTUALLY not have that component?
			if (!component) {
				AnimationPlayingState* state = SparseSetGetElement(&animationStates, event->gameObjectID.id);
				DyArrayFree(&state->lastKeyframeIdxs);
				SparseSetRemoveElement(&animationStates, event->gameObjectID.id);
				continue;
			}

			//It has the component, don't remove from the PlayingState set.
		}
	}

	//Here it is the same, just for the ComponentAttached event.
	eventQueue = GEngineGetEventQueue(_publicContext.defaultEventTypes.componentAttached);
	for (size_t i = 0; i < eventQueue.elementCount; i++)
	{
		ComponentAttachedEvent* event = DyArrayGetElement((dyarray*)&eventQueue, i);
		if (event->componentTypeID == _publicContext.defaultComponents.animationPlayer)
		{
			AnimationPlayerComponent* component = GEngineGetComponent(event->gameObjectID, _publicContext.defaultComponents.animationPlayer);
			//Does it not have the component, contrary to what the event says?
			if (!component) {
				AnimationPlayingState* state = SparseSetGetElement(&animationStates, event->gameObjectID.id);
				DyArrayFree(&state->lastKeyframeIdxs);
				SparseSetRemoveElement(&animationStates, event->gameObjectID.id);
				continue;
			}

			//It has the component
			AnimationPlayingState state = {0};
			state.gameObject = event->gameObjectID;
			state.shouldBePlaying = true;

			DyArrayCreate(&state.lastKeyframeIdxs, sizeof(uint32_t), 20);
			SparseSetAddElement(&animationStates, event->gameObjectID.id, &state);
		}
	}
}

void ProcessAnimationState(AnimationPlayingState* animationPlayingState, AnimationPlayerComponent* animPlayer)
{
	if (!animationPlayingState->shouldBePlaying) return;

	Animation animation = animPlayer->animation;

	//For every channel in the animation, run its keyframes
	for (size_t channelIdx = 0; channelIdx < animation.animationChannels.elementCount; channelIdx++)
	{
		AnimationChannel* channelPtr = DyArrayGetElement(&animation.animationChannels, channelIdx);
		//Get the last used keyframe on this channel
		uint32_t* lastUsedKeyframeIdx = (uint32_t*)DyArrayGetElement(&animationPlayingState->lastKeyframeIdxs, channelIdx);

		if (channelPtr->keyframes.elementCount < 2) {
			printf("ProcessAnimationState fail: on GameObject of ID %zu and Generation %zu, "
				"the channel of index %zu has an invalid number of keyframes (%zu).\nAt least 2 are required.\n",
				animationPlayingState->gameObject.id, animationPlayingState->gameObject.gen, channelIdx, channelPtr->keyframes.elementCount);
			continue;
		}

		//Get keyframe data
		Keyframe* keyframe = DyArrayGetElement(&channelPtr->keyframes, *lastUsedKeyframeIdx);

		//Check if this channel has completed the animation
		if (channelPtr->keyframes.elementCount == *lastUsedKeyframeIdx + 1) {
			//Important for looping, check if this channel was the longest one (indicating end of entire animation)
			if (animationPlayingState->timeProgression >= animation.duration)
			{
				if (!animPlayer->loopAnimation)
				{
					animationPlayingState->shouldBePlaying = false;
					return;
				}

				//Loop
				animationPlayingState->timeProgression = 0.0f;
				for (size_t i = 0; i < animationPlayingState->lastKeyframeIdxs.elementCount; i++)
				{
					uint32_t* idx = DyArrayGetElement(&animationPlayingState->lastKeyframeIdxs, i);
					*idx = 0;
					return; //Let's skip this frame to prevent non homogeneous channel progression.
				}
			}

			//Go on with other channels.
			continue;
		}

		Keyframe* nextKeyframe = DyArrayGetElement(&channelPtr->keyframes, *(lastUsedKeyframeIdx + 1));
		//t goes from 0 to 1 and it's the interpolation progress from this keyframe to the next.
		float t = (animationPlayingState->timeProgression - keyframe->timestamp) / (animationPlayingState->timeProgression - nextKeyframe->timestamp);
		animationPlayingState->timeProgression += GetFrameTime();

		//Use Raylib's Vector2 Lerp and others for interpolation, they come handy!
		//Vector2 InterpolateVec2(Vector2 a, Vector2 b, float t); ...
		switch (keyframe->interpolationType)
		{
			case GENGINE_INTERPOLATION_LINEAR:
				t = t; //Its the same!
				break;
			case GENGINE_INTERPOLATION_QUADRATIC:
				t *= t; //t^2
				break;
			default:
				printf("ProcessAnimationState fail: on GameObject of ID %zu and Generation %zu, "
					"the channel of index %zu has an unknown interpolation curve type.\n",
					animationPlayingState->gameObject.id, animationPlayingState->gameObject.gen, channelIdx);
				break;
		}

		//Get the component data that this channel targets
		void* targetComponentPtr = GEngineGetComponent(animationPlayingState->gameObject, channelPtr->componentTypeID);

		//Down here there are all the primitive types the engine is able to interpolate.
		switch (channelPtr->componentFieldInfo.type)
		{
			case GENGINE_FIELD_TYPE_FLOAT:
				//float* value = (float*)((char*)targetComponentPtr + channelPtr->componentFieldInfo.offset);

				break;

			case GENGINE_FIELD_TYPE_VECTOR2:
				Vector2* valueToInterpolate = (Vector2*)((char*)targetComponentPtr + channelPtr->componentFieldInfo.offset);
				Vector2* currentKeyframe = keyframe->data;
				Vector2* targetKeyframe = nextKeyframe->data;

				*valueToInterpolate = Vector2Lerp(*currentKeyframe, *targetKeyframe, t);
				break;

			case GENGINE_FIELD_TYPE_VECTOR3:
				break;

			case GENGINE_FIELD_TYPE_COLOR: //What changes between Vec3 and Color is the 4th parameter (alpha), it doesn't differ from a vec4 interpolation.
				break;

			default:
				printf("ProcessAnimationState fail: on GameObject of ID %zu and Generation %zu, "
					"the channel of index %zu has an unknown field type, which the system doesn't know how to interpolate.\n",
					animationPlayingState->gameObject.id, animationPlayingState->gameObject.gen, channelIdx);
				break;
		}

		//Check if we should move on to the next keyframe
		if (keyframe->timestamp <= animationPlayingState->timeProgression) {
			*lastUsedKeyframeIdx += 1;
		}
	}
}

void AnimationPlayerSystem(GameObjectID gameObjectID, void** components)
{
	AnimationPlayerComponent* player = components[0];

	AnimationPlayingState* state = SparseSetGetElement(&animationStates, gameObjectID.id);
	if (!state) {
		//We shouldn't get here... I don't know what happened.
		printf("AnimationPlayerSystem fail, the AnimationPlayingState for an entity with id %zu and gen %zu has not been found.\n",
			gameObjectID.id, gameObjectID.gen);
		return;
	}

	//We have the AnimationPlayerComponent, we have the AnimationPlayingState, let's play the animation!
	ProcessAnimationState(state, player);
}

void AnimationPlayerFrameEnd()
{

}

void AnimationPlayerCleanUp()
{
	SparseSetFree(&animationStates);
}
