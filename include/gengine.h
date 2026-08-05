#ifndef GENGINE_H_
#define GENGINE_H_

//#include <gecs.h> //MAYBE DON'T EXPOSE THIS

/*
 *      Gabro's Game Engine
 * Define "GENGINE_DEBUG_LOG" to enable console debug messages.
 */

#include <stdint.h>
#define GENGINE_SCENE_NAME_MAX_LENGTH 23

/*
 * Initialize the GEngine library and creates the window.
 */
bool GEngineInitialize(const char* windowTitle, unsigned short windowWidth, unsigned short windowHeight);

/*
 * Clean up the GEngine library.
 */
void GEngineTerminate();

typedef struct GEngineScene GEngineScene; //Forward declaration for opaque pointer.

typedef struct {size_t id; size_t gen;} GameObjectID;
typedef uint8_t GEngineComponentTypeID;
typedef size_t GEngineSystemID;

enum GEngineSystemType
{
    SYSTEMTYPE_RENDER,
    SYSTEMTYPE_LOGIC,
    SYSTEMTYPE_PHYSICS,
    SYSTEMTYPE_INPUT,
    SYSTEMTYPE_START,
    SYSTEMTYPE_END
};

/*
 * Registers a system in the engine.
 * Set "runOnPause" to true if you want this system to still execute on game pause.
 * SystemType = START will only execute on GEngineGameStart
 * SystemType = END will only execute on GEngineGameEnd
 * DEV NOTE: DOCUMENT BETTER ON VARIADIC (same as gecs RegisterSystem).
 */
GEngineSystemID GEngineRegisterSystem(void (*callback)(GameObjectID, void**), bool runOnPause, enum GEngineSystemType type, int componentCount, ...);

/*
 * Registers a component type in the engine.
 */
GEngineComponentTypeID GEngineRegisterComponent(size_t size, const char* name, uint32_t fieldCount, ...);

/*
 * Starts the game with the current scene instance.
 */
void GEngineStartGame();

/*
 * Check this every frame to know if the games wants to run or it ended.
 */
bool GEngineGameWantsToRun();

/*
 * Pause the game, stopping every system that has not been marked with "runOnPause".
 */
void GEnginePauseGame();

/*
 * Check if the game is paused.
 */
bool GEngineIsGamePaused();

/*
 * Resumes game.
 */
void GEngineResumeGame();

/*
 * Run every system, handling logic, updating and rendering the game.
 * Even if the game is paused, you still need to call this since it handles which systems still need to be executed and which not.
 */
void GEngineRunGame();

/*
 * Call this whenever you want to end the game, setting GEngineGameWantsToRun to false.
 */
void GEngineEndGame();

/*
 * ======================================
 * ======================================
 * ========== WORK IN PROGRESS ==========
 * ======================================
 * ======================================
 *
 * This section is not them main part of the engine, it is mostly a wrapper for the ecs.
 * The major engine functionality are the functions above, the game loop.
 * First i'll do those, then i'll look and see what to do with those below.
 */

/*
 * Makes a new empty scene, discarding the current one.
 * If you need it saved, call GEngineSaveScene or GEngineSaveSceneInDisk.
 */
void GEngineMakeNewScene();

/*
 * Saves the current engine state in a scene.
 * Scenes must be freed after use.
 */
GEngineScene* GEngineSaveScene(const char* name);

/*
 * Loads a passed scene in the engine, freeing the current one.
 */
void GEngineLoadScene(const GEngineScene* scene);

/*
 * Serializes a passed scene in disk.
 * DEVNOTE: remember to serialize also name.
 */
void GEngineSaveSceneInDisk(const GEngineScene* scene, const char* filePath);

/*
 * Serializes the current engine scene in the disk.
 */
void GEngineSaveCurrentSceneInDisk(const char* sceneName, const char* filePath);

/*
 * Makes a scene instance by deserializing it from disk.
 */
GEngineScene* GEngineMakeSceneFromDisk(const char* filePath);

/*
 * Loads a scene directly in the engine by deserializing it from disk.
 */
void GEngineLoadSceneFromDisk(const char* filePath);

/*
 * @brief Frees the memory allocated by the passed scene.
 */
void GEngineFreeScene(GEngineScene** scene);

/*
 * Retrieve the name of a scene.
 */
const char* GEngineGetSceneName(GEngineScene* scene);

/*
 * @brief Check if the passed scene is valid.
 * @return True if the scene is valid, False otherwise.
 */
bool GEngineIsSceneValid(const GEngineScene* scene);

/*
 * @brief Get a component type meta data.
 * @param componentTypeID The id of the requested component type.
 * @return A pointer to a read-only struct that contains the meta data for this
 * component type.
 */
//const ComponentTypeInfo* GEngineGetComponentTypeInfo(ComponentTypeID componentTypeID);

/*
 * @brief Creates an entity in the system.
 * More than 1 entity can have the same name at the same time.
 * @param name The name of the entity.
 * @return The newly created entity on success. GECS_INVALID_ID on failure.
 */
GameObjectID GEngineCreateGameObject(const char* name);

/*
 * @brief Deletes an existing entity and its associated components.
 * @param entity The target's entity ID.
 */
void GEngineDeleteGameObject(GameObjectID entity);

/*
 * @brief Checks if an entity exists.
 * @param entity The target's entity ID.
 * @return True if the entity exists, false otherwise.
 */
bool GECS_DoesGameObjectExist(GameObjectID entity);

/*
 * @brief Deactivate this entity, disabling any system from interacting with it.
 * @param entity The entity ID to deactivate.
 */
void GEngineDisableEntity(GameObjectID entity);

/*
 * @brief Activate this entity, re-enabling the interaction with any system.
 * @param entity The entity ID to activate.
 */
void GEngineEnableEntity(GameObjectID entity);

/*
 * @brief Checks if an entity is active.
 * @param entity The target entity.
 * @return True is the entity is active, False otherwise.
 */
bool GEngineIsEntityEnabled(GameObjectID entity);

/*
 * @brief Deactivate this entity's specified component, disabling any system from interacting with it.
 * @param entity The entity ID to deactivate.
 * @param componentTypeID The target entity's component id.
 */
void GEngineDisableGameObjectComponent(GameObjectID entity, GEngineComponentTypeID componentTypeID);

/*
 * @brief Activate this entity's specified component, re-enabling the interaction with any system.
 * @param entity The entity ID to activate.
 * @param componentTypeID The target entity's component id.
 */
void GEngineEnableGameObjectComponent(GameObjectID entity, GEngineComponentTypeID componentTypeID);

/*
 * @brief Checks if an entity's specified component is active.
 * @param entity The target entity.
 * @param componentTypeID The target entity's component id.
 * @return True is the entity is active, False otherwise.
 */
bool GEngineIsGameObjectComponentEnabled(GameObjectID entity, GEngineComponentTypeID componentTypeID);

/*
 * @brief Attach a registered component to an existing entity.
 * You cannot attach the same component type to the same entity more than once.
 * @param componentData An allcated buffer long as the component type's size.
 */
void GEngineAttachComponent(GameObjectID entity, GEngineComponentTypeID componentTypeID, void* componentData);

/*
 * @brief Detach a registered component from an existing entity.
 */
void GEngineDetachComponent(GameObjectID entity, GEngineComponentTypeID componentTypeID);

/*
 * @brief Retrieves the component object from a specified existing entity.
 * Is it useful to check if an entity has a component.
 * @return The retrieved component data on success. NULL if the entity doesn't have the component.
 */
void* GEngineGetComponent(GameObjectID entity, GEngineComponentTypeID componentTypeID);

/*
 * @brief Fast way to know if an entity has a component
 */
//Not sure if i want this, might as well use GetComponent.
//bool GECS_DoesEntityHaveComponent(EntityID entity, ComponentTypeID componentTypeID);

/*
 * @brief Retrieves a specified entity's info.
 * @param entity The target entity's ID.
 * @return The entity's read-only info struct pointer.
 */
//const EntityInfo* GEngineGetGameObjectInfo(GameObjectID entity);

#endif
