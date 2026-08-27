#ifndef GENGINE_TYPES_H_
#define GENGINE_TYPES_H_

#include "raylib.h"
#include <stdint.h>
typedef struct GEngineScene GEngineScene; //Forward declaration for opaque pointer.

typedef struct {size_t id; size_t gen;} GameObjectID;
typedef uint8_t GEngineComponentTypeID;
typedef size_t GEngineSystemID;

enum GEngineSystemType
{
    GENGINE_SUBSYSTEM_TYPE_INPUT,
    GENGINE_SUBSYSTEM_TYPE_LOGIC,
    GENGINE_SUBSYSTEM_TYPE_PHYSICS,
    GENGINE_SUBSYSTEM_TYPE_RENDER,
};

typedef struct
{
    struct
    {
        GEngineComponentTypeID transform2D;
        GEngineComponentTypeID sprite;
    } defaultComponents;

    Camera2D mainCamera2D;
    Camera3D mainCamera3D;
    Color backgroundColor;

    bool gizmosEnabled;
} GEnginePublicContext;

#endif
