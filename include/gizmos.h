#ifndef GENGINE_GIZMOS_H_
#define GENGINE_GIZMOS_H_
#include <raylib.h>

/*
 * Gizmos are renderable primitives which are moslty used for debug rendering.
 * Their biggest advantage is that this draw commands can be called anywhere at anytime,
 * not bound to a RENDER type system, as long as GEngine is initialized of course.
 */

void GEngineGizmosText(const char* text, Vector2 pos, int fontSize, Color color);
void GEngineGizmosSphere(Vector3 position, float radius, Color color, bool wireframe);
void GEngineGizmosArrow2D(Vector2 pos, Vector2 dir, float thickness, Color color);
void GEngineGizmosArrow3D(Vector3 pos, Vector3 dir, float radius, Color color);
void GEngineGizmosLine(Vector2 start, Vector2 end, Color color);
void GEngineGizmosRect(Rectangle rect, Color color, bool fill);

#endif
