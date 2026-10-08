#ifndef GENGINE_DEFAULT_COMPONENTS_H_
#define GENGINE_DEFAULT_COMPONENTS_H_

#include "gecs.h"
#include "gengine_types.h"
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
    GENGINE_FIELD_TYPE_ANIMATION,
} GEngineFieldType;

typedef struct
{
    Vector2 position;
    Vector2 scale;
    float rotation;
} Transform2DComponent;

typedef struct
{
    Texture2D spriteSheet;
    Rectangle rect;
} SpriteSheetEntry;

typedef struct
{
    SpriteSheetEntry spriteSheetEntry;
    Vector2 pivot;
    Color tint;
    uint16_t depth;
} SpriteComponent;

typedef enum
{
    GENGINE_INTERPOLATION_LINEAR,
    GENGINE_INTERPOLATION_QUADRATIC,
} InterpolationType;

typedef struct
{
    void* data; //Use AnimationChannel field info to interpret the data type
    float timestamp;
    InterpolationType interpolationType; //Interpolation type from this to the next keyframe.
} Keyframe;

typedef struct
{
    dyarray keyframes; //Contains keyframe data (identify data type through field info)
    ComponentFieldInfo componentFieldInfo; //Used for fetching the right data type and memory position in the Component data struct
    GEngineComponentTypeID componentTypeID; //Which component for this GameObject are we going to animate?
    float duration;
} AnimationChannel;

typedef struct
{
    dyarray animationChannels; //Contains AnimationChannel(s)
    float duration; //Basically how much time the longest animation channel plays.
} Animation; //All these resources are exposed but they are not made to be modified manually!

typedef struct
{
    Animation animation; //EVERY RESOURCE INSIDE A COMPONENT WILL BE STORED AS IDs FROM A CENTRALIZED RESOURCE SYSTEM, SO THAT SERIALIZATION WORKS.
    bool loopAnimation;
} AnimationPlayerComponent;

#endif
