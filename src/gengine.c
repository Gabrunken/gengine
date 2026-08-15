#include "dyarray.h"
#include "raylib.h"
#include "sprite_system.h"
#include <gengine.h>

#include <stdarg.h>
#include <stdio.h>

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

typedef struct
{
	bool initialized;
	dyarray renderSubsystems;
	dyarray logicSubsystems;
	dyarray physicsSubsystems;
	dyarray inputSubsystems;

	bool gameStarted;
	bool gamePaused;
} GEnginePrivateContext;

static GEnginePrivateContext _privateContext;
GEnginePublicContext _publicContext;

GEnginePublicContext* GEngineInitialize(const char* windowTitle, unsigned short windowWidth, unsigned short windowHeight)
{
	if (_privateContext.initialized) {
		GENGINE_LOG_MISUSE("engine is already initialized");
		return NULL;
	}

	if (!DyArrayCreate(&_privateContext.renderSubsystems, sizeof(GEngineSubSystemInfo), 10) ||
		!DyArrayCreate(&_privateContext.logicSubsystems, sizeof(GEngineSubSystemInfo), 10) ||
		!DyArrayCreate(&_privateContext.physicsSubsystems, sizeof(GEngineSubSystemInfo), 10) ||
		!DyArrayCreate(&_privateContext.inputSubsystems, sizeof(GEngineSubSystemInfo), 10)){
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
	 3,
	 GENGINE_FIELD_TYPE_TEXTURE, "texture",
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

	_publicContext.mainCamera2D.target = (Vector2){ 0.0f, 0.0f };
	_publicContext.mainCamera2D.rotation = 0.0f;
	_publicContext.mainCamera2D.zoom = 1.0f; //Normal scale

	_publicContext.mainCamera3D.up = (Vector3){0.0f, 1.0f, 0.0f};
	_publicContext.mainCamera3D.fovy = 75.0f; //Fov
	_publicContext.mainCamera3D.projection = CAMERA_PERSPECTIVE;

	GENGINE_LOG_NOTE("engine initialized");
	return &_publicContext;

	error:
	if (_privateContext.renderSubsystems.buf)
		DyArrayFree(&_privateContext.renderSubsystems);
	if (_privateContext.logicSubsystems.buf)
		DyArrayFree(&_privateContext.logicSubsystems);
	if (_privateContext.physicsSubsystems.buf)
		DyArrayFree(&_privateContext.physicsSubsystems);
	if (_privateContext.inputSubsystems.buf)
		DyArrayFree(&_privateContext.inputSubsystems);

	GENGINE_LOG_ERROR("Failed to allocate memory for the system");
	return NULL;
}

void GEngineTerminate()
{
	if (!_privateContext.initialized) {
		GENGINE_LOG_MISUSE("engine is not yet initialized");
		return;
	}

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

	_publicContext.time = GetTime();
	_publicContext.frameDeltaTime = GetFrameTime();

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
		systemInfo->subsystem.CleanUp();
	}

	for (size_t i = 0; i < _privateContext.logicSubsystems.elementCount; i++)
	{
		GEngineSubSystemInfo* systemInfo = DyArrayGetElement(&_privateContext.logicSubsystems, i);
		systemInfo->subsystem.CleanUp();
	}

	for (size_t i = 0; i < _privateContext.physicsSubsystems.elementCount; i++)
	{
		GEngineSubSystemInfo* systemInfo = DyArrayGetElement(&_privateContext.physicsSubsystems, i);
		systemInfo->subsystem.CleanUp();
	}

	for (size_t i = 0; i < _privateContext.renderSubsystems.elementCount; i++)
	{
		GEngineSubSystemInfo* systemInfo = DyArrayGetElement(&_privateContext.renderSubsystems, i);
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
