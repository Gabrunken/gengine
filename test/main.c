#include "default_components.h"
#include "gizmos.h"
#include "raylib.h"
#include <math.h>
#include <stdio.h>
#include <gengine.h>
#include <stdint.h>

Texture2D tile;

Color tints[10] = {
	YELLOW,
	ORANGE,
	PINK,
	RED,
	GREEN,
	LIME,
	DARKGREEN,
	SKYBLUE,
	PURPLE,
	VIOLET,
};

GEnginePublicContext* GEngine;

void InstantiateTileAtMousePosition(int depth)
{
	GameObjectID gameObject = GEngineCreateGameObject("tile");

	Vector2 mousePos = GetMousePosition();
	Vector2 finalPos = GetScreenToWorld2D(mousePos, GEngine->mainCamera2D);

	Transform2DComponent transform;
	transform.position = finalPos;
	transform.scale = (Vector2){1,1};
	transform.rotation = 0;
	GEngineAttachComponent(gameObject, GEngine->defaultComponents.transform2D, &transform);

	SpriteComponent sprite;
	sprite.spriteSheetEntry.spriteSheet = tile;
	sprite.spriteSheetEntry.rect = (Rectangle){0,0,sprite.spriteSheetEntry.spriteSheet.width,sprite.spriteSheetEntry.spriteSheet.height};
	sprite.tint = tints[GetRandomValue(0, 9)];
	sprite.depth = depth;
	GEngineAttachComponent(gameObject, GEngine->defaultComponents.sprite, &sprite);
}

int main()
{
	GEngine = GEngineInitialize("GEngine Test", 512, 512);
	if (!GEngine) {
		printf("Failed to initialize gengine, terminate...");
		return 1;
	}

	GEngine->backgroundColor = DARKBLUE;

	tile = LoadTexture("tile.png");

	GEngineStartGame();
	while (GEngineGameWantsToRun())
	{
		if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
			int depth = GetRandomValue(0, UINT16_MAX);
			InstantiateTileAtMousePosition(depth);
		}

		if (IsKeyDown(KEY_D)) {
			GEngine->mainCamera2D.offset.x -= 250 * GetFrameTime();
		}

		if (IsKeyDown(KEY_A)) {
			GEngine->mainCamera2D.offset.x += 250 * GetFrameTime();
		}

		if (IsKeyDown(KEY_W)) {
			GEngine->mainCamera2D.offset.y += 250 * GetFrameTime();
		}

		if (IsKeyDown(KEY_S)) {
			GEngine->mainCamera2D.offset.y -= 250 * GetFrameTime();
		}

		if (IsKeyDown(KEY_Q)) {
			GEngine->mainCamera2D.rotation += 20 * GetFrameTime();
		}

		if (IsKeyDown(KEY_E)) {
			GEngine->mainCamera2D.rotation -= 20 * GetFrameTime();
		}

		if (IsKeyPressed(KEY_M)) {
			GEngine->gizmosEnabled = !GEngine->gizmosEnabled;
		}

		Rectangle cameraRect = GEngineGetCamera2DRect();
		cameraRect.x += 10;
		cameraRect.y += 10;
		cameraRect.width -= 20;
		cameraRect.height -= 20;

		//UpdateCamera(&GEngine->mainCamera3D, CAMERA_FREE);

		GEngineGizmosText(TextFormat("FPS: %i", GetFPS()), (Vector2){10, 10}, 20, GREEN);
		//GEngineGizmosRect(cameraRect, RED, false);
		GEngineGizmosArrow3D((Vector3){0, 0, -10}, (Vector3){0, sin(GetTime()) * 5, 0}, 0.1f, RED);
		GEngineGizmosArrow2D((Vector2){0, 0}, (Vector2){cameraRect.x, cameraRect.y}, 5, GREEN);
		GEngineProcessFrame();
	}

	GEngineTerminate();

	UnloadTexture(tile);
    return 0;
}
