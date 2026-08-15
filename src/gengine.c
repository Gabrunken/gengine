#include "dyarray.h"
#include "raylib.h"
#include "sprite_systems.h"
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

typedef struct GEngineScene
{
	GECSSnapshot snapshot;
	char name[GENGINE_SCENE_NAME_MAX_LENGTH + 1];
} GEngineScene;

typedef struct
{
	GEngineSystemID id;
	enum GEngineSystemType type;
	bool runOnPause;
} GEngineSystemInfo;

typedef struct
{
	bool initialized;
	dyarray startSystems; //They all contain GEngineSystemInfo
	dyarray endSystems;
	dyarray renderSystems;
	dyarray logicSystems;
	dyarray physicsSystems;
	dyarray inputSystems;

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

	if (!DyArrayCreate(&_privateContext.startSystems, sizeof(GEngineSystemInfo), 10) ||
	  	!DyArrayCreate(&_privateContext.endSystems, sizeof(GEngineSystemInfo), 10) ||
		!DyArrayCreate(&_privateContext.renderSystems, sizeof(GEngineSystemInfo), 10) ||
		!DyArrayCreate(&_privateContext.logicSystems, sizeof(GEngineSystemInfo), 10) ||
		!DyArrayCreate(&_privateContext.physicsSystems, sizeof(GEngineSystemInfo), 10) ||
		!DyArrayCreate(&_privateContext.inputSystems, sizeof(GEngineSystemInfo), 10)){
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

	GEngineRegisterSystem(SpriteLogicSystem, true, SYSTEMTYPE_LOGIC, 2,
			_publicContext.defaultComponents.transform2D,
			_publicContext.defaultComponents.sprite);

	_publicContext.backgroundColor = BLACK;

	_publicContext.mainCamera2D.zoom = 1.0f; //Normal scale
	_publicContext.mainCamera3D.up = (Vector3){0.0f, 1.0f, 0.0f};
	_publicContext.mainCamera3D.fovy = 75.0f; //Fov
	_publicContext.mainCamera3D.projection = CAMERA_PERSPECTIVE;

	SpriteInitializeBuffers();

	InitWindow(windowWidth, windowHeight, windowTitle);

	GENGINE_LOG_NOTE("engine initialized");
	return &_publicContext;

	error:
	if (_privateContext.startSystems.buf)
		DyArrayFree(&_privateContext.startSystems);
	if (_privateContext.endSystems.buf)
		DyArrayFree(&_privateContext.endSystems);
	if (_privateContext.renderSystems.buf)
		DyArrayFree(&_privateContext.renderSystems);
	if (_privateContext.logicSystems.buf)
		DyArrayFree(&_privateContext.logicSystems);
	if (_privateContext.physicsSystems.buf)
		DyArrayFree(&_privateContext.physicsSystems);
	if (_privateContext.inputSystems.buf)
		DyArrayFree(&_privateContext.inputSystems);

	GENGINE_LOG_ERROR("Failed to allocate memory for the system");
	return NULL;
}

void GEngineTerminate()
{
	if (!_privateContext.initialized) {
		GENGINE_LOG_MISUSE("engine is not yet initialized");
		return;
	}

	SpriteFreeBuffers();

	CloseWindow();

	GECS_CleanUp();

	GENGINE_LOG_NOTE("engine terminated");
	_privateContext.initialized = false;
}

GEngineSystemID GEngineRegisterSystem(void (*callback)(GameObjectID, void**), bool runOnPause, enum GEngineSystemType type, int componentCount, ...)
{
	if (!_privateContext.initialized) {
		GENGINE_LOG_MISUSE("engine is not yet initialized");
		return GENGINE_INVALID_COMPONENT_TYPE_ID;
	}

	va_list args;
	va_start(args, componentCount);
	GEngineSystemID id = GECS_vRegisterSystem((void (*)(EntityID, void**))callback, componentCount, args);
	va_end(args);

	if (id == GENGINE_INVALID_SYSTEM_ID) {
		GENGINE_LOG_ERROR("Failed to register system");
		return GENGINE_INVALID_SYSTEM_ID;
	}

	GEngineSystemInfo systemInfo = {0};
	systemInfo.id = id;
	systemInfo.type = type;
	systemInfo.runOnPause = runOnPause;

	switch (type)
	{
	case SYSTEMTYPE_START:
		DyArrayAddElement(&_privateContext.startSystems, &systemInfo);
		break;
	case SYSTEMTYPE_END:
		DyArrayAddElement(&_privateContext.endSystems, &systemInfo);
		break;
	case SYSTEMTYPE_RENDER:
		DyArrayAddElement(&_privateContext.renderSystems, &systemInfo);
		break;
	case SYSTEMTYPE_LOGIC:
		DyArrayAddElement(&_privateContext.logicSystems, &systemInfo);
		break;
	case SYSTEMTYPE_PHYSICS:
		DyArrayAddElement(&_privateContext.physicsSystems, &systemInfo);
		break;
	case SYSTEMTYPE_INPUT:
		DyArrayAddElement(&_privateContext.inputSystems, &systemInfo);
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

	for (size_t i = 0; i < _privateContext.startSystems.elementCount; i++)
	{
		GEngineSystemInfo* systemInfo = DyArrayGetElement(&_privateContext.startSystems, i);
		SystemID id = (SystemID)systemInfo->id;
		GECS_ExecuteSystem(id);
	}

	_privateContext.gameStarted = true;
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

	//Input
	for (size_t i = 0; i < _privateContext.inputSystems.elementCount; i++) {
		GEngineSystemInfo* systemInfo = DyArrayGetElement(&_privateContext.inputSystems, i);
		if (_privateContext.gamePaused && !systemInfo->runOnPause) continue;
		SystemID id = (SystemID)systemInfo->id;
		GECS_ExecuteSystem(id);
	}

	//Logic
	for (size_t i = 0; i < _privateContext.logicSystems.elementCount; i++) {
		GEngineSystemInfo* systemInfo = DyArrayGetElement(&_privateContext.logicSystems, i);
		if (_privateContext.gamePaused && !systemInfo->runOnPause) continue;
		SystemID id = (SystemID)systemInfo->id;
		GECS_ExecuteSystem(id);
	}

	//Physics
	for (size_t i = 0; i < _privateContext.physicsSystems.elementCount; i++) {
		GEngineSystemInfo* systemInfo = DyArrayGetElement(&_privateContext.physicsSystems, i);
		if (_privateContext.gamePaused && !systemInfo->runOnPause) continue;
		SystemID id = (SystemID)systemInfo->id;
		GECS_ExecuteSystem(id);
	}

	BeginDrawing();
	ClearBackground(_publicContext.backgroundColor);

	BeginMode3D(_publicContext.mainCamera3D);

	//Rendering
	for (size_t i = 0; i < _privateContext.renderSystems.elementCount; i++) {
		GEngineSystemInfo* systemInfo = DyArrayGetElement(&_privateContext.renderSystems, i);
		if (_privateContext.gamePaused && !systemInfo->runOnPause) continue;
		SystemID id = (SystemID)systemInfo->id;
		GECS_ExecuteSystem(id);
	}

	EndMode3D();

	SpritePrepareRendering();

	BeginMode2D(_publicContext.mainCamera2D);
	SpriteFlushRendering();
	EndMode2D();
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

	for (size_t i = 0; i < _privateContext.endSystems.elementCount; i++)
	{
		GEngineSystemInfo* systemInfo = DyArrayGetElement(&_privateContext.endSystems, i);
		SystemID id = (SystemID)systemInfo->id;
		GECS_ExecuteSystem(id);
	}

	GEngineMakeNewScene();

	_privateContext.gameStarted = false;
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
