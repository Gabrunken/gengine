#ifndef GENGINE_H_
#define GENGINE_H_

//#include <gecs.h> //MAYBE DON'T EXPOSE THIS

/*
 *      Gabro's Game Engine
 * Define "GENGINE_DEBUG_LOG" globally to enable console debug messages.
 */

/*
 * NOTE FOR MYSELF
 * Every rendering function of raylib MUST be called inside a registered RENDER type subsystem.
 * Not doing so is undefined behaviour, so if you need anything drawn, do it inside a render subsystem.
 * ANOTHER NOTE
 * When you draw, it is your responsibility to wrap your draw commands in BeginMode2/3D followed by EndMode2/3D,
 * since some systems might use the 2D camera and some might need the 3D camera, your choice.
 * But don't call BeginDrawing / EndDrawing!!! they are already called.
 */

#include <dyarray.h>
#include <default_components.h>
#include <default_systems.h>
#include <limits.h>
#include <stdint.h>
#include <gizmos.h>
#define GENGINE_SCENE_NAME_MAX_LENGTH 23

#include <gengine_types.h>

/*
 * Initialize the GEngine library and creates the window.
 * Check if the return value is NULL or not to verify initialization success.
 */
GEnginePublicContext* GEngineInitialize(const char* windowTitle, unsigned short windowWidth, unsigned short windowHeight);

/*
 * Clean up the GEngine library.
 */
void GEngineTerminate();

/*
 * Registers a system in the engine.
 * Set "runOnPause" to true if you want this system to still execute on game pause.
 * SystemType = START will only execute on GEngineGameStart
 * SystemType = END will only execute on GEngineGameEnd
 * DEV NOTE: DOCUMENT BETTER ON VARIADIC (same as gecs RegisterSystem).
 */
GEngineSystemID GEngineRegisterSubSystem(
        void (*StartUp)(void),
   	    void (*CleanUp)(void),
   	    void (*FrameStart)(void),
    	void (*systemCallback)(GameObjectID, void**),
		void (*FrameEnd)(void),

		enum GEngineSystemType type, bool runOnPause, int componentCount, ...);

/*
 * Registers a component type in the engine.
 * @param size The size in bytes of the singular component.
 * @param name The name of this component type.
 * @brief Register a component type in the system.
 * @param fieldCount The number of elements (fields) this component consists of.
 *
 * IMPORTANT NOTE: until decided otherwise, the fields must be KNOWN fields.
 * Meaning that they must be present inside the GEngineFieldType enum.
 *
 * The variadic parameter is used to describe the elements this component has, coupled with the
 * previous argument "fieldCount", to provide introspection information for the system.
 * This parameter is made of "fieldType" and "fieldName" and "fieldOffset" triplets, so for each field in the component,
 * insert the type, name and offset in this order. The fields must be EXACTLY ordered and layed out as
 * you would use them in memory.
 *
 * IMPORTANT NOTE: to retrieve the member offset (if it's in a struct obviously), use offsetof().
 *
 * IMPORTANT NOTE: again, if the component is a struct, it MUST NOT have custom, unknown structs inside.
 * If you put your own struct inside a component as field, it will have an unknown field type,
 * causing undefined behaviour.
 *
 * @return The unique id assigned to the newly registered component type.
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
void GEngineProcessFrame();

/*
 * Call this whenever you want to end the game, setting GEngineGameWantsToRun to false.
 */
void GEngineEndGame();

Rectangle GEngineGetCamera2DRect();

/*
 * Register an event type inside the system.
 * "eventStructSize" is the size in bytes of the event's struct, containing its payload. (you define that)
 * On failure, it returns GENGINE_INVALID_EVENT_TYPE.
 */
EventType GEngineRegisterEventType(size_t eventStructSize);

/*
 * Get the current's frame event queue, for a specified type of event.
 * Note, do not fucking modify the returned array, it is READ-ONLY.
 */
const dyarray GEngineGetEventQueue(EventType type);

/*
 * Push an event in the queue.
 * "eventData" is a pointer to the Event struct corresponding to "type".
 */
void GEnginePushEvent(EventType type, void* eventData);

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
 * component type. NULL is the component type does not exist.
 */
const ComponentTypeInfo* GEngineGetComponentTypeInfo(GEngineComponentTypeID componentTypeID);

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
void GEngineDisableGameObject(GameObjectID entity);

/*
 * @brief Activate this entity, re-enabling the interaction with any system.
 * @param entity The entity ID to activate.
 */
void GEngineEnableGameObject(GameObjectID entity);

/*
 * @brief Checks if an entity is active.
 * @param entity The target entity.
 * @return True is the entity is active, False otherwise.
 */
bool GEngineIsGameObjectEnabled(GameObjectID entity);

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

/*
 * Built-in systems based functions
 */

/*
 * Get the visible rect of a sprite, which may be transformed and scaled (rotation doesn't work right now)
 */
Rectangle GEngineGetVisibleSpriteRectangle(SpriteComponent* spriteComponent, Transform2DComponent* transformComponent);

#endif
