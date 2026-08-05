#include "dyarray.h"
#include "raylib.h"
#include <gengine.h>

#include <stdarg.h>
#include <stdio.h>

#include <gecs.h>
#include <stdlib.h>
#include <string.h>

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
} GEngineContext;

static GEngineContext _context;

bool GEngineInitialize(const char* windowTitle, unsigned short windowWidth, unsigned short windowHeight)
{
	if (_context.initialized) {
		GENGINE_LOG_MISUSE("engine is already initialized");
		return true;
	}

	if (!DyArrayCreate(&_context.startSystems, sizeof(GEngineSystemInfo), 10)) {
		goto error;
	}

	if (!DyArrayCreate(&_context.endSystems, sizeof(GEngineSystemInfo), 10)) {
		goto error;
	}

	if (!DyArrayCreate(&_context.renderSystems, sizeof(GEngineSystemInfo), 10)) {
		goto error;
	}

	if (!DyArrayCreate(&_context.logicSystems, sizeof(GEngineSystemInfo), 10)) {
		goto error;
	}

	if (!DyArrayCreate(&_context.physicsSystems, sizeof(GEngineSystemInfo), 10)) {
		goto error;
	}

	if (!DyArrayCreate(&_context.inputSystems, sizeof(GEngineSystemInfo), 10)) {
		goto error;
	}

	GECS_Init();

	InitWindow(windowWidth, windowHeight, windowTitle);

	GENGINE_LOG_NOTE("engine initialized");
	_context.initialized = true;
	return true;

	error:
	if (_context.startSystems.buf)
		DyArrayFree(&_context.startSystems);
	if (_context.endSystems.buf)
		DyArrayFree(&_context.endSystems);
	if (_context.renderSystems.buf)
		DyArrayFree(&_context.renderSystems);
	if (_context.logicSystems.buf)
		DyArrayFree(&_context.logicSystems);
	if (_context.physicsSystems.buf)
		DyArrayFree(&_context.physicsSystems);
	if (_context.inputSystems.buf)
		DyArrayFree(&_context.inputSystems);

	GENGINE_LOG_ERROR("Failed to allocate memory for the system");
	return false;
}

void GEngineTerminate()
{
	if (!_context.initialized) {
		GENGINE_LOG_MISUSE("engine is not yet initialized");
		return;
	}

	CloseWindow();

	GECS_CleanUp();

	GENGINE_LOG_NOTE("engine terminated");
	_context.initialized = false;
}

GEngineSystemID GEngineRegisterSystem(void (*callback)(GameObjectID, void**), bool runOnPause, enum GEngineSystemType type, int componentCount, ...)
{
	if (!_context.initialized) {
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
		DyArrayAddElement(&_context.startSystems, &systemInfo);
		break;
	case SYSTEMTYPE_END:
		DyArrayAddElement(&_context.endSystems, &systemInfo);
		break;
	case SYSTEMTYPE_RENDER:
		DyArrayAddElement(&_context.renderSystems, &systemInfo);
		break;
	case SYSTEMTYPE_LOGIC:
		DyArrayAddElement(&_context.logicSystems, &systemInfo);
		break;
	case SYSTEMTYPE_PHYSICS:
		DyArrayAddElement(&_context.physicsSystems, &systemInfo);
		break;
	case SYSTEMTYPE_INPUT:
		DyArrayAddElement(&_context.inputSystems, &systemInfo);
		break;
	default:
		break;
	}

	return id;
}

GEngineComponentTypeID GEngineRegisterComponent(size_t size, const char* name, uint32_t fieldCount, ...)
{
	if (!_context.initialized) {
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
	if (!_context.initialized) {
		GENGINE_LOG_MISUSE("engine is not yet initialized");
		return;
	}

	if (_context.gameStarted) {
		GENGINE_LOG_MISUSE("game already started");
		return;
	}

	for (size_t i = 0; i < _context.startSystems.elementCount; i++)
	{
		GEngineSystemInfo* systemInfo = DyArrayGetElement(&_context.startSystems, i);
		SystemID id = (SystemID)systemInfo->id;
		GECS_ExecuteSystem(id);
	}

	_context.gameStarted = true;
}

bool GEngineGameWantsToRun()
{
	if (!_context.initialized) {
		GENGINE_LOG_MISUSE("engine is not yet initialized");
		return false;
	}

	if (!_context.gameStarted) {
		GENGINE_LOG_MISUSE("game has not been started yet");
		return false;
	}

	return _context.gameStarted;
}

void GEnginePauseGame()
{
	if (!_context.initialized) {
		GENGINE_LOG_MISUSE("engine is not yet initialized");
		return;
	}

	if (!_context.gameStarted) {
		GENGINE_LOG_MISUSE("game has not been started yet");
		return;
	}

	_context.gamePaused = true;
}

bool GEngineIsGamePaused()
{
	if (!_context.initialized) {
		GENGINE_LOG_MISUSE("engine is not yet initialized");
		return false;
	}

	if (!_context.gameStarted) {
		GENGINE_LOG_MISUSE("game has not been started yet");
		return false;
	}

	return _context.gamePaused;
}

void GEngineResumeGame()
{
	if (!_context.initialized) {
		GENGINE_LOG_MISUSE("engine is not yet initialized");
		return;
	}

	if (!_context.gameStarted) {
		GENGINE_LOG_MISUSE("game has not been started yet");
		return;
	}

	_context.gamePaused = false;
}

void GEngineRunGame()
{
	if (!_context.initialized) {
		GENGINE_LOG_MISUSE("engine is not yet initialized");
		return;
	}

	if (!_context.gameStarted) {
		GENGINE_LOG_MISUSE("game has not been started yet");
		return;
	}

	//Input
	for (size_t i = 0; i < _context.inputSystems.elementCount; i++) {
		GEngineSystemInfo* systemInfo = DyArrayGetElement(&_context.inputSystems, i);
		if (_context.gamePaused && !systemInfo->runOnPause) continue;
		SystemID id = (SystemID)systemInfo->id;
		GECS_ExecuteSystem(id);
	}

	//Logic
	for (size_t i = 0; i < _context.logicSystems.elementCount; i++) {
		GEngineSystemInfo* systemInfo = DyArrayGetElement(&_context.logicSystems, i);
		if (_context.gamePaused && !systemInfo->runOnPause) continue;
		SystemID id = (SystemID)systemInfo->id;
		GECS_ExecuteSystem(id);
	}

	//Physics
	for (size_t i = 0; i < _context.physicsSystems.elementCount; i++) {
		GEngineSystemInfo* systemInfo = DyArrayGetElement(&_context.physicsSystems, i);
		if (_context.gamePaused && !systemInfo->runOnPause) continue;
		SystemID id = (SystemID)systemInfo->id;
		GECS_ExecuteSystem(id);
	}

	render:
	//Rendering
	for (size_t i = 0; i < _context.renderSystems.elementCount; i++) {
		GEngineSystemInfo* systemInfo = DyArrayGetElement(&_context.renderSystems, i);
		if (_context.gamePaused && !systemInfo->runOnPause) continue;
		SystemID id = (SystemID)systemInfo->id;
		GECS_ExecuteSystem(id);
	}
}

void GEngineEndGame()
{
	if (!_context.initialized) {
		GENGINE_LOG_MISUSE("engine is not yet initialized");
		return;
	}

	if (!_context.gameStarted) {
		GENGINE_LOG_MISUSE("game has not been started yet");
		return;
	}

	for (size_t i = 0; i < _context.endSystems.elementCount; i++)
	{
		GEngineSystemInfo* systemInfo = DyArrayGetElement(&_context.endSystems, i);
		SystemID id = (SystemID)systemInfo->id;
		GECS_ExecuteSystem(id);
	}

	GEngineMakeNewScene();

	_context.gameStarted = false;
}

void GEngineMakeNewScene()
{
	if (!_context.initialized) {
		GENGINE_LOG_MISUSE("engine is not yet initialized");
		return;
	}

	GECS_ClearECS();
}

GEngineScene* GEngineSaveScene(const char* name)
{
	if (!_context.initialized) {
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
	if (!_context.initialized) {
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
	if (!_context.initialized) {
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
	if (!_context.initialized) {
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
	if (!_context.initialized) {
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
	if (!_context.initialized) {
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
	if (!_context.initialized) {
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
	if (!_context.initialized) {
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
	if (!_context.initialized) {
		GENGINE_LOG_MISUSE("engine is not yet initialized");
		return;
	}

	if (!filePath) {
		GENGINE_LOG_MISUSE("filePath is NULL");
		return;
	}

	GECS_MakeAndLoadSnapshotFromDisk(filePath);
}
