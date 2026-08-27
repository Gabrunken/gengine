#ifndef GENGINE_RIGIDBODY2D_SYSTEM_H_
#define GENGINE_RIGIDBODY2D_SYSTEM_H_
#include <gengine.h>

void Rigidbody2DStartUp();
void Rigidbody2DFrameStart();
void Rigidbody2DSystem(GameObjectID gameObject, void** components);
void Rigidbody2DFrameEnd();
void Rigidbody2DCleanUp();

#endif
