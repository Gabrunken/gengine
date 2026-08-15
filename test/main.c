#include "default_components.h"
#include "raylib.h"
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
	sprite.texture = tile;
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
		if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
			GEngine->backgroundColor = tints[GetRandomValue(0, 9)];
		}

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

		DrawText(TextFormat("FPS: %i", GetFPS()), 10, 10, 20, GREEN);
		GEngineProcessFrame();
	}

	GEngineTerminate();

	UnloadTexture(tile);
    return 0;
}
