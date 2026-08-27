#ifndef GENGINE_DEFAULT_COMPONENTS_H_
#define GENGINE_DEFAULT_COMPONENTS_H_

#include "raylib.h"
#include <sprite_system.h>
#include <stdint.h>

typedef enum
{
    GENGINE_FIELD_TYPE_NONE,
    GENGINE_FIELD_TYPE_BOOL,
    GENGINE_FIELD_TYPE_CHAR,
    GENGINE_FIELD_TYPE_UINT8_T,
    GENGINE_FIELD_TYPE_UINT16_T,
    GENGINE_FIELD_TYPE_UINT32_T,
    GENGINE_FIELD_TYPE_UINT64_T,
    GENGINE_FIELD_TYPE_INT8_T,
    GENGINE_FIELD_TYPE_INT16_T,
    GENGINE_FIELD_TYPE_INT32_T,
    GENGINE_FIELD_TYPE_INT64_T,
    GENGINE_FIELD_TYPE_FLOAT,
    GENGINE_FIELD_TYPE_DOUBLE,
    GENGINE_FIELD_TYPE_STRING,
    GENGINE_FIELD_TYPE_TEXTURE,
    GENGINE_FIELD_TYPE_COLOR,

    GENGINE_FIELD_TYPE_VECTOR2,
    GENGINE_FIELD_TYPE_VECTOR3,

    GENGINE_FIELD_TYPE_SPRITESHEET_ENTRY,
} GEngineFieldType;

typedef struct
{
    Vector2 position;
    Vector2 scale;
    float rotation;
} Transform2DComponent;

typedef struct
{
    GEngineSpriteSheetEntry spriteSheetEntry;
    Color tint;
    uint16_t depth;
} SpriteComponent;

#endif
