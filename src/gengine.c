#include "dyarray.h"
#include "raylib.h"
#include "raymath.h"
#include <gengine.h>

#include <gecs.h>
#include <stdlib.h>
#include <string.h>

#include <default_components.h>
#include <default_systems.h>

#ifdef GENGINE_DEBUG_LOG
#define GENGINE_LOG_MISUSE(format, ...)\
do {printf("\033[35m""GENGINE API MISUSE from %s - " format "\033[39m\n", __func__ __VA_OPT__(, __VA_ARGS__));} while(0)
#define GENGINE_LOG_ERROR(format, ...)\
do {printf("\033[31m""GENGINE ERROR from %s - " format "\033[39m\n", __func__ __VA_OPT__(, __VA_ARGS__));} while(0)
#define GENGINE_LOG_WARNING(format, ...)\
do {printf("\033[33m""GENGINE WARNING from %s - " format "\033[39m\n", __func__ __VA_OPT__(, __VA_ARGS__));} while(0)
#define GENGINE_LOG_NOTE(format, ...)\
do {printf("\033[39m""GENGINE NOTE from %s - " format "\033[39m\n", __func__ __VA_OPT__(, __VA_ARGS__));} while(0)
#else
#define GENGINE_LOG_MISUSE(format, ...)
#define GENGINE_LOG_ERROR(format, ...)
#define GENGINE_LOG_WARNING(format, ...)
#define GENGINE_LOG_NOTE(format, ...)
#endif

#define GENGINE_INVALID_SYSTEM_ID GECS_INVALID_SYSTEM_ID
#define GENGINE_INVALID_COMPONENT_TYPE_ID GECS_INVALID_COMPONENT_TYPE_ID

typedef struct
{
	void (*StartUp)(void);
    void (*CleanUp)(void);
    void (*FrameStart)(void);
	GEngineSystemID system;
	void (*FrameEnd)(void);

	enum GEngineSystemType type;
} GEngineSubSystem;

typedef struct GEngineScene
{
	GECSSnapshot snapshot;
	char name[GENGINE_SCENE_NAME_MAX_LENGTH + 1];
} GEngineScene;

typedef struct
{
	GEngineSubSystem subsystem;
	enum GEngineSystemType type;
	bool runOnPause;
} GEngineSubSystemInfo;

typedef enum
{
	GENGINE_GIZMOS_TEXT,
	GENGINE_GIZMOS_SPHERE,
	GENGINE_GIZMOS_ARROW_2D,
	GENGINE_GIZMOS_ARROW_3D,
	GENGINE_GIZMOS_LINE,
	GENGINE_GIZMOS_RECT,
} GEngineGizmosCommandType;

typedef struct
{
	GEngineGizmosCommandType type;

	union
	{
		struct {const char* str; Vector2 pos; int fontSize; Color color;} text;
		struct {Vector2 pos; Vector2 dir; float thickness; Color color;} arrow2D;
		struct {Vector2 start; Vector2 end; Color color;} line;
		struct {Rectangle rect; Color color; bool fill;} rect;
	} args;
} GEngineGizmosCommand2D;

typedef struct
{
	GEngineGizmosCommandType type;

	union
	{
		struct {Vector3 pos; float radius; Color color; bool wireframe;} sphere;
		struct {Vector3 pos; Vector3 dir; float radius; Color color;} arrow3D;
	} args;
} GEngineGizmosCommand3D;

typedef struct
{
	bool initialized;
	dyarray renderSubsystems;
	dyarray logicSubsystems;
	dyarray physicsSubsystems;
	dyarray inputSubsystems;

	dyarray gizmosRenderingCommandQueue2D;
	dyarray gizmosRenderingCommandQueue3D;

	bool gameStarted;
	bool gamePaused;
} GEnginePrivateContext;

static GEnginePrivateContext _privateContext;
GEnginePublicContext _publicContext;

void _GEngineFlushGizmos();

GEnginePublicContext* GEngineInitialize(const char* windowTitle, unsigned short windowWidth, unsigned short windowHeight)
{
	if (_privateContext.initialized) {
		GENGINE_LOG_MISUSE("engine is already initialized");
		return NULL;
	}

	if (!DyArrayCreate(&_privateContext.inputSubsystems, sizeof(GEngineSubSystemInfo), 10) 	 ||
		!DyArrayCreate(&_privateContext.logicSubsystems, sizeof(GEngineSubSystemInfo), 10) 	 ||
		!DyArrayCreate(&_privateContext.physicsSubsystems, sizeof(GEngineSubSystemInfo), 10) ||
		!DyArrayCreate(&_privateContext.renderSubsystems, sizeof(GEngineSubSystemInfo), 10)) {
		goto error;
	}

	if (!DyArrayCreate(&_privateContext.gizmosRenderingCommandQueue2D, sizeof(GEngineGizmosCommand2D), 100) ||
		!DyArrayCreate(&_privateContext.gizmosRenderingCommandQueue3D, sizeof(GEngineGizmosCommand3D), 100)) {
		goto error;
	}

	_privateContext.initialized = true;

	/*
	 * ECS Initialization
	 * Component and System registration.
	 */

	GECS_Init();

	_publicContext.defaultComponents.transform2D = GECS_RegisterComponent(sizeof(Transform2DComponent), "Transform2D",
	 3,
	 GENGINE_FIELD_TYPE_VECTOR2, "position",
	 GENGINE_FIELD_TYPE_VECTOR2, "scale",
	 GENGINE_FIELD_TYPE_FLOAT, "rotation");

	_publicContext.defaultComponents.sprite = GECS_RegisterComponent(sizeof(SpriteComponent), "Sprite",
	 4,
	 GENGINE_FIELD_TYPE_SPRITESHEET_ENTRY, "spriteSheetEntry",
	 GENGINE_FIELD_TYPE_VECTOR2, "pivot",
	 GENGINE_FIELD_TYPE_COLOR, "tint",
	 GENGINE_FIELD_TYPE_UINT16_T, "depth");

	GEngineRegisterSubSystem(
		SpriteStartUp,
		SpriteCleanUp,
		SpriteFrameStart,
		SpriteSystem,
		SpriteFrameEnd,
		GENGINE_SUBSYSTEM_TYPE_RENDER, true, 2,
			_publicContext.defaultComponents.transform2D,
			_publicContext.defaultComponents.sprite);

	InitWindow(windowWidth, windowHeight, windowTitle);

	_publicContext.backgroundColor = BLACK;

	_publicContext.mainCamera2D.zoom = 1.0f; //Normal scale

	_publicContext.mainCamera3D.target = (Vector3){0.0f, 0.0f, -1.0f};
	_publicContext.mainCamera3D.up = (Vector3){0.0f, 1.0f, 0.0f};
	_publicContext.mainCamera3D.fovy = 75.0f; //Fov
	_publicContext.mainCamera3D.projection = CAMERA_PERSPECTIVE;

	_publicContext.gizmosEnabled = true;

	GENGINE_LOG_NOTE("engine initialized");
	return &_publicContext;

	error:
	if (_privateContext.inputSubsystems.buf)
		DyArrayFree(&_privateContext.inputSubsystems);
	if (_privateContext.logicSubsystems.buf)
		DyArrayFree(&_privateContext.logicSubsystems);
	if (_privateContext.physicsSubsystems.buf)
		DyArrayFree(&_privateContext.physicsSubsystems);
	if (_privateContext.renderSubsystems.buf)
		DyArrayFree(&_privateContext.renderSubsystems);

	GENGINE_LOG_ERROR("Failed to allocate memory for the system");
	return NULL;
}

void GEngineTerminate()
{
	if (!_privateContext.initialized) {
		GENGINE_LOG_MISUSE("engine is not yet initialized");
		return;
	}

	DyArrayFree(&_privateContext.inputSubsystems);
	DyArrayFree(&_privateContext.logicSubsystems);
	DyArrayFree(&_privateContext.physicsSubsystems);
	DyArrayFree(&_privateContext.renderSubsystems);

	DyArrayFree(&_privateContext.gizmosRenderingCommandQueue2D);
	DyArrayFree(&_privateContext.gizmosRenderingCommandQueue3D);

	CloseWindow();

	GECS_CleanUp();

	GENGINE_LOG_NOTE("engine terminated");
	_privateContext.initialized = false;
}

GEngineSystemID GEngineRegisterSubSystem(
		void (*StartUp)(void),
    	void (*CleanUp)(void),
     	void (*FrameStart)(void),
      	void (*systemCallback)(GameObjectID, void**),
		void (*FrameEnd)(void),

		enum GEngineSystemType type, bool runOnPause, int componentCount, ...)
{
	if (!_privateContext.initialized) {
		GENGINE_LOG_MISUSE("engine is not yet initialized");
		return GENGINE_INVALID_COMPONENT_TYPE_ID;
	}

	va_list args;
	va_start(args, componentCount);
	GEngineSystemID id = GECS_vRegisterSystem((void (*)(EntityID, void**))systemCallback, componentCount, args);
	va_end(args);

	if (id == GENGINE_INVALID_SYSTEM_ID) {
		GENGINE_LOG_ERROR("Failed to register system");
		return GENGINE_INVALID_SYSTEM_ID;
	}

	GEngineSubSystemInfo systemInfo = {0};
	systemInfo.subsystem.system = id;
	systemInfo.subsystem.StartUp = StartUp;
	systemInfo.subsystem.FrameStart = FrameStart;
	systemInfo.subsystem.FrameEnd = FrameEnd;
	systemInfo.subsystem.CleanUp = CleanUp;
	systemInfo.type = type;
	systemInfo.runOnPause = runOnPause;

	switch (type)
	{
	case GENGINE_SUBSYSTEM_TYPE_RENDER:
		DyArrayAddElement(&_privateContext.renderSubsystems, &systemInfo);
		break;
	case GENGINE_SUBSYSTEM_TYPE_LOGIC:
		DyArrayAddElement(&_privateContext.logicSubsystems, &systemInfo);
		break;
	case GENGINE_SUBSYSTEM_TYPE_PHYSICS:
		DyArrayAddElement(&_privateContext.physicsSubsystems, &systemInfo);
		break;
	case GENGINE_SUBSYSTEM_TYPE_INPUT:
		DyArrayAddElement(&_privateContext.inputSubsystems, &systemInfo);
		break;
	default:
		break;
	}

	return id;
}

GEngineComponentTypeID GEngineRegisterComponent(size_t size, const char* name, uint32_t fieldCount, ...)
{
	if (!_privateContext.initialized) {
		GENGINE_LOG_MISUSE("engine is not yet initialized");
		return GENGINE_INVALID_COMPONENT_TYPE_ID;
	}

	va_list args;
	va_start(args, fieldCount);
	ComponentTypeID id = GECS_vRegisterComponent(size, name, fieldCount, args);
	va_end(args);

	if (id == GENGINE_INVALID_COMPONENT_TYPE_ID) {
		GENGINE_LOG_ERROR("Failed to register component type");
		return GENGINE_INVALID_COMPONENT_TYPE_ID;
	}

	return (GEngineComponentTypeID)id;
}

void GEngineStartGame()
{
	if (!_privateContext.initialized) {
		GENGINE_LOG_MISUSE("engine is not yet initialized");
		return;
	}

	if (_privateContext.gameStarted) {
		GENGINE_LOG_MISUSE("game already started");
		return;
	}

	for (size_t i = 0; i < _privateContext.inputSubsystems.elementCount; i++)
	{
		GEngineSubSystemInfo* systemInfo = DyArrayGetElement(&_privateContext.inputSubsystems, i);
		if (systemInfo->subsystem.StartUp)
			systemInfo->subsystem.StartUp();
	}

	for (size_t i = 0; i < _privateContext.logicSubsystems.elementCount; i++)
	{
		GEngineSubSystemInfo* systemInfo = DyArrayGetElement(&_privateContext.logicSubsystems, i);
		if (systemInfo->subsystem.StartUp)
			systemInfo->subsystem.StartUp();
	}

	for (size_t i = 0; i < _privateContext.physicsSubsystems.elementCount; i++)
	{
		GEngineSubSystemInfo* systemInfo = DyArrayGetElement(&_privateContext.physicsSubsystems, i);
		if (systemInfo->subsystem.StartUp)
			systemInfo->subsystem.StartUp();
	}

	for (size_t i = 0; i < _privateContext.renderSubsystems.elementCount; i++)
	{
		GEngineSubSystemInfo* systemInfo = DyArrayGetElement(&_privateContext.renderSubsystems, i);
		if (systemInfo->subsystem.StartUp)
			systemInfo->subsystem.StartUp();
	}

	_privateContext.gameStarted = true;
}

void GEngineProcessFrame()
{
	if (!_privateContext.initialized) {
		GENGINE_LOG_MISUSE("engine is not yet initialized");
		return;
	}

	if (!_privateContext.gameStarted) {
		GENGINE_LOG_MISUSE("game has not been started yet");
		return;
	}

	for (size_t i = 0; i < _privateContext.inputSubsystems.elementCount; i++) {
		GEngineSubSystemInfo* systemInfo = DyArrayGetElement(&_privateContext.inputSubsystems, i);
		if (_privateContext.gamePaused && !systemInfo->runOnPause) continue;

		if (systemInfo->subsystem.FrameStart)
			systemInfo->subsystem.FrameStart();

		SystemID id = (SystemID)systemInfo->subsystem.system;
		GECS_ExecuteSystem(id);

		if (systemInfo->subsystem.FrameEnd)
			systemInfo->subsystem.FrameEnd();
	}

	for (size_t i = 0; i < _privateContext.logicSubsystems.elementCount; i++) {
		GEngineSubSystemInfo* systemInfo = DyArrayGetElement(&_privateContext.logicSubsystems, i);
		if (_privateContext.gamePaused && !systemInfo->runOnPause) continue;

		if (systemInfo->subsystem.FrameStart)
			systemInfo->subsystem.FrameStart();

		SystemID id = (SystemID)systemInfo->subsystem.system;
		GECS_ExecuteSystem(id);

		if (systemInfo->subsystem.FrameEnd)
			systemInfo->subsystem.FrameEnd();
	}

	for (size_t i = 0; i < _privateContext.physicsSubsystems.elementCount; i++) {
		GEngineSubSystemInfo* systemInfo = DyArrayGetElement(&_privateContext.physicsSubsystems, i);
		if (_privateContext.gamePaused && !systemInfo->runOnPause) continue;

		if (systemInfo->subsystem.FrameStart)
			systemInfo->subsystem.FrameStart();

		SystemID id = (SystemID)systemInfo->subsystem.system;
		GECS_ExecuteSystem(id);

		if (systemInfo->subsystem.FrameEnd)
			systemInfo->subsystem.FrameEnd();
	}

	BeginDrawing();
	ClearBackground(_publicContext.backgroundColor);

	for (size_t i = 0; i < _privateContext.renderSubsystems.elementCount; i++) {
		GEngineSubSystemInfo* systemInfo = DyArrayGetElement(&_privateContext.renderSubsystems, i);
		if (_privateContext.gamePaused && !systemInfo->runOnPause) continue;

		if (systemInfo->subsystem.FrameStart)
			systemInfo->subsystem.FrameStart();

		SystemID id = (SystemID)systemInfo->subsystem.system;
		GECS_ExecuteSystem(id);

		if (systemInfo->subsystem.FrameEnd)
			systemInfo->subsystem.FrameEnd();
	}

	if (_publicContext.gizmosEnabled)
	{
		_GEngineFlushGizmos();
	}

	EndDrawing();

	GECS_ProcessFrameEnd();
}

void GEngineEndGame()
{
	if (!_privateContext.initialized) {
		GENGINE_LOG_MISUSE("engine is not yet initialized");
		return;
	}

	if (!_privateContext.gameStarted) {
		GENGINE_LOG_MISUSE("game has not been started yet");
		return;
	}

	for (size_t i = 0; i < _privateContext.inputSubsystems.elementCount; i++)
	{
		GEngineSubSystemInfo* systemInfo = DyArrayGetElement(&_privateContext.inputSubsystems, i);
		if (systemInfo->subsystem.CleanUp)
			systemInfo->subsystem.CleanUp();
	}

	for (size_t i = 0; i < _privateContext.logicSubsystems.elementCount; i++)
	{
		GEngineSubSystemInfo* systemInfo = DyArrayGetElement(&_privateContext.logicSubsystems, i);
		if (systemInfo->subsystem.CleanUp)
			systemInfo->subsystem.CleanUp();
	}

	for (size_t i = 0; i < _privateContext.physicsSubsystems.elementCount; i++)
	{
		GEngineSubSystemInfo* systemInfo = DyArrayGetElement(&_privateContext.physicsSubsystems, i);
		if (systemInfo->subsystem.CleanUp)
			systemInfo->subsystem.CleanUp();
	}

	for (size_t i = 0; i < _privateContext.renderSubsystems.elementCount; i++)
	{
		GEngineSubSystemInfo* systemInfo = DyArrayGetElement(&_privateContext.renderSubsystems, i);
		if (systemInfo->subsystem.CleanUp)
			systemInfo->subsystem.CleanUp();
	}

	GEngineMakeNewScene();

	_privateContext.gameStarted = false;
}

bool GEngineGameWantsToRun()
{
	if (!_privateContext.initialized) {
		GENGINE_LOG_MISUSE("engine is not yet initialized");
		return false;
	}

	if (!_privateContext.gameStarted) {
		GENGINE_LOG_MISUSE("game has not been started yet");
		return false;
	}

	if (WindowShouldClose()) {
		GEngineEndGame();
		return false;
	}

	return _privateContext.gameStarted;
}

void GEnginePauseGame()
{
	if (!_privateContext.initialized) {
		GENGINE_LOG_MISUSE("engine is not yet initialized");
		return;
	}

	if (!_privateContext.gameStarted) {
		GENGINE_LOG_MISUSE("game has not been started yet");
		return;
	}

	_privateContext.gamePaused = true;
}

bool GEngineIsGamePaused()
{
	if (!_privateContext.initialized) {
		GENGINE_LOG_MISUSE("engine is not yet initialized");
		return false;
	}

	if (!_privateContext.gameStarted) {
		GENGINE_LOG_MISUSE("game has not been started yet");
		return false;
	}

	return _privateContext.gamePaused;
}

void GEngineResumeGame()
{
	if (!_privateContext.initialized) {
		GENGINE_LOG_MISUSE("engine is not yet initialized");
		return;
	}

	if (!_privateContext.gameStarted) {
		GENGINE_LOG_MISUSE("game has not been started yet");
		return;
	}

	_privateContext.gamePaused = false;
}

void GEngineMakeNewScene()
{
	if (!_privateContext.initialized) {
		GENGINE_LOG_MISUSE("engine is not yet initialized");
		return;
	}

	GECS_ClearECS();
}

/*
 * ========== GIZMOS ==========
 */

void GEngineGizmosText(const char* text, Vector2 pos, int fontSize, Color color)
{
	if (!_privateContext.initialized) {
		GENGINE_LOG_MISUSE("engine is not yet initialized");
		return;
	}

	if (!text) {
		GENGINE_LOG_MISUSE("text is NULL");
		return;
	}

	else if (!fontSize) {
		GENGINE_LOG_MISUSE("fontSize is 0");
		return;
	}

	GEngineGizmosCommand2D cmd = {0};
	cmd.type = GENGINE_GIZMOS_TEXT;

	cmd.args.text.str = _strdup(text);
	cmd.args.text.pos = pos;
	cmd.args.text.fontSize = fontSize;
	cmd.args.text.color = color;
	DyArrayAddElement(&_privateContext.gizmosRenderingCommandQueue2D, &cmd);
}

void GEngineGizmosSphere(Vector3 position, float radius, Color color, bool wireframe)
{
	if (!_privateContext.initialized) {
		GENGINE_LOG_MISUSE("engine is not yet initialized");
		return;
	}

	GEngineGizmosCommand3D cmd = {0};
	cmd.type = GENGINE_GIZMOS_SPHERE;

	cmd.args.sphere.pos = position;
	cmd.args.sphere.radius = radius;
	cmd.args.sphere.color = color;
	cmd.args.sphere.wireframe = wireframe;
	DyArrayAddElement(&_privateContext.gizmosRenderingCommandQueue3D, &cmd);
}

void GEngineGizmosArrow2D(Vector2 pos, Vector2 dir, float thickness, Color color)
{
	if (!_privateContext.initialized) {
		GENGINE_LOG_MISUSE("engine is not yet initialized");
		return;
	}

	GEngineGizmosCommand2D cmd = {0};
	cmd.type = GENGINE_GIZMOS_ARROW_2D;

	cmd.args.arrow2D.pos = pos;
	cmd.args.arrow2D.dir = dir;
	cmd.args.arrow2D.color = color;
	cmd.args.arrow2D.thickness = thickness;
	DyArrayAddElement(&_privateContext.gizmosRenderingCommandQueue2D, &cmd);
}

void GEngineGizmosArrow3D(Vector3 pos, Vector3 dir, float radius, Color color)
{
	if (!_privateContext.initialized) {
		GENGINE_LOG_MISUSE("engine is not yet initialized");
		return;
	}

	GEngineGizmosCommand3D cmd = {0};
	cmd.type = GENGINE_GIZMOS_ARROW_3D;

	cmd.args.arrow3D.pos = pos;
	cmd.args.arrow3D.dir = dir;
	cmd.args.arrow3D.radius = radius;
	cmd.args.arrow3D.color = color;
	DyArrayAddElement(&_privateContext.gizmosRenderingCommandQueue3D, &cmd);
}

void GEngineGizmosLine(Vector2 start, Vector2 end, Color color)
{
	if (!_privateContext.initialized) {
		GENGINE_LOG_MISUSE("engine is not yet initialized");
		return;
	}

	GEngineGizmosCommand2D cmd = {0};
	cmd.type = GENGINE_GIZMOS_LINE;

	cmd.args.line.start = start;
	cmd.args.line.end = end;
	cmd.args.line.color = color;
	DyArrayAddElement(&_privateContext.gizmosRenderingCommandQueue2D, &cmd);
}

void GEngineGizmosRect(Rectangle rect, Color color, bool fill)
{
	if (!_privateContext.initialized) {
		GENGINE_LOG_MISUSE("engine is not yet initialized");
		return;
	}

	GEngineGizmosCommand2D cmd = {0};
	cmd.type = GENGINE_GIZMOS_RECT;

	cmd.args.rect.rect = rect;
	cmd.args.rect.color = color;
	cmd.args.rect.fill = fill;
	DyArrayAddElement(&_privateContext.gizmosRenderingCommandQueue2D, &cmd);
}

//Don't worry abound BeginDrawing since the caller cares for it, but do call BeginMode2/3D.
void _GEngineFlushGizmos()
{
	BeginMode2D(_publicContext.mainCamera2D);

	for (size_t i = 0; i < _privateContext.gizmosRenderingCommandQueue2D.elementCount; i++)
	{
		GEngineGizmosCommand2D* cmd = DyArrayGetElement(&_privateContext.gizmosRenderingCommandQueue2D, i);

		switch (cmd->type)
		{
			case GENGINE_GIZMOS_TEXT:
				DrawText(cmd->args.text.str, cmd->args.text.pos.x, cmd->args.text.pos.y, cmd->args.text.fontSize, cmd->args.text.color);
				free((void*)cmd->args.text.str);
				break;
			case GENGINE_GIZMOS_ARROW_2D:
				float length = Vector2Length(cmd->args.arrow2D.dir);
			    if (length == 0.0f) return;

			    Vector2 end = Vector2Add(cmd->args.arrow2D.pos, cmd->args.arrow2D.dir);

			    Vector2 normDir = Vector2Scale(cmd->args.arrow2D.dir, 1.0f / length);

			    float tipLength = 3.0f * cmd->args.arrow2D.thickness;

			    if (tipLength > length) tipLength = length;

			    float tipWidth = tipLength;

			    Vector2 bodyEnd = Vector2Subtract(end, Vector2Scale(normDir, tipLength));

			    DrawLineEx(cmd->args.arrow2D.pos, bodyEnd, cmd->args.arrow2D.thickness, cmd->args.arrow2D.color);

			    Vector2 perp = { -normDir.y, normDir.x };

			    Vector2 p1 = end;
			    Vector2 p2 = Vector2Add(bodyEnd, Vector2Scale(perp, tipWidth / 2.0f));
			    Vector2 p3 = Vector2Subtract(bodyEnd, Vector2Scale(perp, tipWidth / 2.0f));

			    DrawTriangle(p1, p2, p3, cmd->args.arrow2D.color);
			    DrawTriangle(p1, p3, p2, cmd->args.arrow2D.color);
				break;
			case GENGINE_GIZMOS_LINE:
				DrawLine(cmd->args.line.start.x, cmd->args.line.start.y, cmd->args.line.end.x, cmd->args.line.end.y, cmd->args.line.color);
				break;
			case GENGINE_GIZMOS_RECT:
				if (cmd->args.rect.fill) {
					DrawRectangleRec(cmd->args.rect.rect, cmd->args.rect.color);
					break;
				}

				DrawRectangleLines(cmd->args.rect.rect.x, cmd->args.rect.rect.y, cmd->args.rect.rect.width, cmd->args.rect.rect.height, cmd->args.rect.color);
				break;
			default:
				break;
		}
	}

	EndMode2D();

	BeginMode3D(_publicContext.mainCamera3D);

	for (size_t i = 0; i < _privateContext.gizmosRenderingCommandQueue3D.elementCount; i++)
	{
		GEngineGizmosCommand3D* cmd = DyArrayGetElement(&_privateContext.gizmosRenderingCommandQueue3D, i);

		switch (cmd->type)
		{
			case GENGINE_GIZMOS_SPHERE:
				if (cmd->args.sphere.wireframe) {
					DrawSphereWires(cmd->args.sphere.pos, cmd->args.sphere.radius, 8, 16, cmd->args.sphere.color);
					break;
				}

				DrawSphere(cmd->args.sphere.pos, cmd->args.sphere.radius, cmd->args.sphere.color);
				break;
			case GENGINE_GIZMOS_ARROW_3D:
				float length = Vector3Length(cmd->args.arrow3D.dir);
				if (length == 0.0f) return;

				Vector3 end = Vector3Add(cmd->args.arrow3D.pos, cmd->args.arrow3D.dir);

				Vector3 normDir = Vector3Scale(cmd->args.arrow3D.dir, 1.0f / length);

				float tipLength = cmd->args.arrow3D.radius * 4.0f;
				if (tipLength > length) tipLength = length;

				Vector3 bodyEnd = Vector3Subtract(end, Vector3Scale(normDir, tipLength));

				DrawCylinderEx(cmd->args.arrow3D.pos, bodyEnd, cmd->args.arrow3D.radius, cmd->args.arrow3D.radius, 8, cmd->args.arrow3D.color); // Corpo
				DrawCylinderEx(bodyEnd, end, cmd->args.arrow3D.radius * 2.5f, 0.0f, 8, cmd->args.arrow3D.color); // Punta
				break;
			default:
				break;
		}
	}

	EndMode3D();

	DyArrayClear(&_privateContext.gizmosRenderingCommandQueue2D);
	DyArrayClear(&_privateContext.gizmosRenderingCommandQueue3D);
}

Rectangle GEngineGetCamera2DRect()
{
	//Should take enlarged bounding box to take rotation into account

	Vector2 camTopLeft = GetScreenToWorld2D((Vector2){0, 0}, _publicContext.mainCamera2D);

	Vector2 camBottomRight = GetScreenToWorld2D((Vector2){GetScreenWidth(), GetScreenHeight()}, _publicContext.mainCamera2D);

	return (Rectangle){
	    camTopLeft.x,
	    camTopLeft.y,
	    camBottomRight.x - camTopLeft.x,
	    camBottomRight.y - camTopLeft.y
	};
}

/*
 * WORK IN PROGRESS FROM HERE
 */

GameObjectID GEngineCreateGameObject(const char* name)
{
	if (!_privateContext.initialized) {
		GENGINE_LOG_MISUSE("engine is not yet initialized");
		return (GameObjectID){0};
	}

	EntityID id = GECS_CreateEntity(name);
	return (GameObjectID){id.id, id.gen};
}

//The heavy lifting of checking if an entity exists, if a component exists etc... is done by the ecs, so no worries.
void GEngineAttachComponent(GameObjectID entity, GEngineComponentTypeID componentTypeID, void* componentData)
{
	if (!_privateContext.initialized) {
		GENGINE_LOG_MISUSE("engine is not yet initialized");
		return;
	}

	GECS_AttachComponent((EntityID){entity.id, entity.gen}, componentTypeID, componentData);
}

void GEngineDetachComponent(GameObjectID entity, GEngineComponentTypeID componentTypeID)
{
	if (!_privateContext.initialized) {
		GENGINE_LOG_MISUSE("engine is not yet initialized");
		return;
	}

	GECS_DetachComponent((EntityID){entity.id, entity.gen}, componentTypeID);
}

void* GEngineGetComponent(GameObjectID entity, GEngineComponentTypeID componentTypeID)
{
	if (!_privateContext.initialized) {
		GENGINE_LOG_MISUSE("engine is not yet initialized");
		return NULL;
	}

	return GECS_GetComponent((EntityID){entity.id, entity.gen}, componentTypeID);
}

GEngineScene* GEngineSaveScene(const char* name)
{
	if (!_privateContext.initialized) {
		GENGINE_LOG_MISUSE("engine is not yet initialized");
		return NULL;
	}

	if (!name) {
		GENGINE_LOG_MISUSE("name is NULL");
		return NULL;
	}

	GEngineScene* scene = calloc(1, sizeof(GEngineScene));
	if (!scene) {
		GENGINE_LOG_ERROR("calloc failed");
		return NULL;
	}

	scene->snapshot = GECS_MakeSnapshot();
	if (!GECS_IsSnapshotValid(&scene->snapshot)) {
		GENGINE_LOG_ERROR("failed to save scene");
		return NULL;
	}

	strncpy(scene->name, name, GENGINE_SCENE_NAME_MAX_LENGTH + 1);

	return scene;
}

void GEngineLoadScene(const GEngineScene* scene)
{
	if (!_privateContext.initialized) {
		GENGINE_LOG_MISUSE("engine is not yet initialized");
		return;
	}

	if (!scene) {
		GENGINE_LOG_MISUSE("scene is NULL");
		return;
	}

	if (!GECS_IsSnapshotValid(&scene->snapshot)) {
		GENGINE_LOG_MISUSE("scene is not valid");
		return;
	}

	GECS_LoadSnapshot(&scene->snapshot);
}

void GEngineFreeScene(GEngineScene** scene)
{
	if (!_privateContext.initialized) {
		GENGINE_LOG_MISUSE("engine is not yet initialized");
		return;
	}

	if (!scene || !(*scene)) {
		GENGINE_LOG_MISUSE("scene is NULL");
		return;
	}

	if (!GECS_IsSnapshotValid(&(*scene)->snapshot)) {
		GENGINE_LOG_MISUSE("scene is not valid");
		return;
	}

	GECS_FreeSnapshot(&(*scene)->snapshot);

	free(*scene);
	*scene = NULL;
}

const char* GEngineGetSceneName(GEngineScene* scene)
{
	if (!_privateContext.initialized) {
		GENGINE_LOG_MISUSE("engine is not yet initialized");
		return NULL;
	}

	if (!scene) {
		GENGINE_LOG_MISUSE("scene is NULL");
		return NULL;
	}

	return scene->name;
}

bool GEngineIsSceneValid(const GEngineScene* scene)
{
	if (!_privateContext.initialized) {
		GENGINE_LOG_MISUSE("engine is not yet initialized");
		return NULL;
	}

	if (!scene) {
		GENGINE_LOG_MISUSE("scene is NULL");
		return NULL;
	}

	return GECS_IsSnapshotValid(&scene->snapshot);
}

void GEngineSaveSceneInDisk(const GEngineScene* scene, const char* filePath)
{
	if (!_privateContext.initialized) {
		GENGINE_LOG_MISUSE("engine is not yet initialized");
		return;
	}

	if (!scene) {
		GENGINE_LOG_MISUSE("scene is NULL");
		return;
	}

	if (!GECS_IsSnapshotValid(&scene->snapshot)) {
		GENGINE_LOG_MISUSE("scene is not valid");
		return;
	}

	//Write name before
	/*
	FILE* file = fopen(filePath, "wb");
	if (!file) {
		GENGINE_LOG_ERROR("failed to open or create file at path %s", filePath);
		return;
	}

	fwrite(scene->name, 1, sizeof(scene->name), file);

	fclose(file);
	*/

	GECS_SaveSnapshotInDisk(&scene->snapshot, filePath);
}

void GEngineSaveCurrentSceneInDisk(const char* sceneName, const char* filePath)
{
	if (!_privateContext.initialized) {
		GENGINE_LOG_MISUSE("engine is not yet initialized");
		return;
	}

	if (!sceneName) {
		GENGINE_LOG_MISUSE("sceneName is NULL");
		return;
	}

	if (!filePath) {
		GENGINE_LOG_MISUSE("filePath is NULL");
		return;
	}

	//Write name before
	/*
	FILE* file = fopen(filePath, "wb");
	if (!file) {
		GENGINE_LOG_ERROR("failed to open or create file at path %s", filePath);
		return;
	}

	fwrite(sceneName, 1, GENGINE_SCENE_NAME_MAX_LENGTH + 1, file);

	fclose(file);
	*/

	GECS_MakeAndSaveSnapshotInDisk(filePath);
}

GEngineScene* GEngineMakeSceneFromDisk(const char* filePath)
{
	if (!_privateContext.initialized) {
		GENGINE_LOG_MISUSE("engine is not yet initialized");
		return NULL;
	}

	if (!filePath) {
		GENGINE_LOG_MISUSE("filePath is NULL");
		return NULL;
	}

	GEngineScene* scene = calloc(1, sizeof(GEngineScene));
	if (!scene) {
		GENGINE_LOG_ERROR("calloc failed");
		return NULL;
	}

	//Read name before
	/*
	FILE* file = fopen(filePath, "rb");
	if (!file) {
		GENGINE_LOG_ERROR("failed to open file at path %s", filePath);
		return NULL;
	}

	fread(scene->name, 1, sizeof(scene->name), file);

	fclose(file);
	*/

	scene->snapshot = GECS_MakeSnapshotFromDisk(filePath);
	return scene;
}

void GEngineLoadSceneFromDisk(const char* filePath)
{
	if (!_privateContext.initialized) {
		GENGINE_LOG_MISUSE("engine is not yet initialized");
		return;
	}

	if (!filePath) {
		GENGINE_LOG_MISUSE("filePath is NULL");
		return;
	}

	GECS_MakeAndLoadSnapshotFromDisk(filePath);
}
