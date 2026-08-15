#include "default_components.h"
#include "dyarray.h"
#include "raylib.h"
#include <sprite_system.h>
#include <stdlib.h>

extern GEnginePublicContext _publicContext;

static dyarray drawCommandQueue;

static Rectangle cameraRect;

typedef struct
{
	Transform2DComponent transform;
	SpriteComponent sprite;
} DrawCommand;

void SpriteStartUp()
{
	if (drawCommandQueue.buf) return;
	DyArrayCreate(&drawCommandQueue, sizeof(DrawCommand), 1000);
}

void SpriteFrameStart()
{
	Vector2 camTopLeft = GetScreenToWorld2D((Vector2){0, 0}, _publicContext.mainCamera2D);

	Vector2 camBottomRight = GetScreenToWorld2D((Vector2){GetScreenWidth(), GetScreenHeight()}, _publicContext.mainCamera2D);

	cameraRect = (Rectangle){
	    camTopLeft.x,
	    camTopLeft.y,
	    camBottomRight.x - camTopLeft.x,
	    camBottomRight.y - camTopLeft.y
	};

	//Should take enlarged bounding box to take rotation into account
}

void SpriteSystem(GameObjectID gameObjectID, void** components)
{
	Transform2DComponent* transform = components[0];
	SpriteComponent* sprite = components[1];
	uint16_t depth = sprite->depth; //Lower means further away (in the background)

	float realWidth = sprite->texture.width * transform->scale.x;
    float realHeight = sprite->texture.height * transform->scale.y;

    Rectangle spriteRect = {
        transform->position.x - (realWidth / 2.0f),
        transform->position.y - (realHeight / 2.0f),
        realWidth,
        realHeight
    };

	//Frustum culling
	if (!CheckCollisionRecs(cameraRect, spriteRect)) {
        return;
    }

	DrawCommand cmd = {*transform, *sprite};
	DyArrayAddElement(&drawCommandQueue, &cmd);
}

static int CompareDrawCommandIndices(const void* a, const void* b)
{
    uint32_t indexA = *(const uint32_t*)a;
    uint32_t indexB = *(const uint32_t*)b;

    DrawCommand* cmdA = DyArrayGetElement(&drawCommandQueue, indexA);
    DrawCommand* cmdB = DyArrayGetElement(&drawCommandQueue, indexB);

    return (cmdA->sprite.depth - cmdB->sprite.depth);
}

void SpriteFrameEnd()
{
	size_t count = drawCommandQueue.elementCount;
	if (count == 0) return;

	uint32_t* sortIndices = malloc(count * sizeof(uint32_t));
	for (uint32_t i = 0; i < count; i++) {
	    sortIndices[i] = i;
	}

	qsort(sortIndices, count, sizeof(uint32_t), CompareDrawCommandIndices);

	BeginMode2D(_publicContext.mainCamera2D);

	for (size_t i = 0; i < count; i++)
	{
	    uint32_t targetIndex = sortIndices[i];
	    DrawCommand* cmd = DyArrayGetElement(&drawCommandQueue, targetIndex);

		Rectangle dest = {cmd->transform.position.x, cmd->transform.position.y,
			cmd->transform.scale.x * cmd->sprite.texture.width, cmd->transform.scale.y * cmd->sprite.texture.height};

		DrawTexturePro(
			cmd->sprite.texture,
			(Rectangle){0,0,cmd->sprite.texture.width,cmd->sprite.texture.height},
		 	dest, (Vector2){dest.width / 2.0f, dest.height / 2.0f}, cmd->transform.rotation, cmd->sprite.tint);
	}

	EndMode2D();

	free(sortIndices);
	drawCommandQueue.elementCount = 0;
}

void SpriteCleanUp()
{
	if (!drawCommandQueue.buf) return;
	DyArrayFree(&drawCommandQueue);
}
