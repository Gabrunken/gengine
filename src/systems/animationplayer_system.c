#include "dyarray.h"
#include "gengine_types.h"
#include "sparse_set.h"
#include <animationplayer_system.h>
#include <default_components.h>
#include <gengine.h>

extern GEnginePublicContext _publicContext;

typedef struct
{
	GameObjectID gameObject;
	uint32_t lastKeyframeIdx;
	float timeProgression;
} AnimationPlayingState;

static struct SparseSet animationStates;

void AnimationPlayerStartUp()
{
	SparseSetCreate(&animationStates, 100, sizeof(AnimationPlayingState));
}

void AnimationPlayerFrameStart()
{
	dyarray eventQueue = GEngineGetEventQueue(_publicContext.defaultEventTypes.componentDetached);
	for (size_t i = 0; i < eventQueue.elementCount; i++)
	{
		ComponentDetachedEvent* event = DyArrayGetElement((dyarray*)&eventQueue, i);
		if (event->componentTypeID == _publicContext.defaultComponents.animationPlayer)
		{
			AnimationPlayerComponent* component = GEngineGetComponent(event->gameObjectID, _publicContext.defaultComponents.animationPlayer);
			//It might actually have the component, a ComponentAttachedEvent might have been pushed after.
			if (!component) {
				SparseSetRemoveElement(&animationStates, event->gameObjectID.id);
				continue;
			}

			//It has the component, don't remove.
		}
	}

	eventQueue = GEngineGetEventQueue(_publicContext.defaultEventTypes.componentAttached);
	for (size_t i = 0; i < eventQueue.elementCount; i++)
	{
		ComponentAttachedEvent* event = DyArrayGetElement((dyarray*)&eventQueue, i);
		if (event->componentTypeID == _publicContext.defaultComponents.animationPlayer)
		{
			AnimationPlayerComponent* component = GEngineGetComponent(event->gameObjectID, _publicContext.defaultComponents.animationPlayer);
			//It might actually not have the component, a ComponentDetachedEvent might have been pushed after.
			if (!component) {
				SparseSetRemoveElement(&animationStates, event->gameObjectID.id);
				continue;
			}

			//It has the component
			AnimationPlayingState state = {0};
			state.gameObject = event->gameObjectID;
			SparseSetAddElement(&animationStates, event->gameObjectID.id, &state);
		}
	}
}

void AnimationPlayerSystem(GameObjectID gameObjectID, void** components)
{
	AnimationPlayerComponent* player = components[0];
}

void AnimationPlayerFrameEnd()
{

}

void AnimationPlayerCleanUp()
{
	SparseSetFree(&animationStates);
}
