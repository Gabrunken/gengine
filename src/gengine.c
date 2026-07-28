#include <gengine.h>

#include <stdio.h>

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

typedef struct
{
	bool initialized;
} GEngineContext;

static GEngineContext _context;

bool GEngineStart()
{
	if (_context.initialized) {
		GENGINE_LOG_MISUSE("engine is already initialized");
		return true;
	}

	GENGINE_LOG_NOTE("engine initialized");
	_context.initialized = true;
	return true;
}

void GEngineTerminate()
{
	if (!_context.initialized) {
		GENGINE_LOG_MISUSE("engine is not yet initialized");
		return;
	}

	GENGINE_LOG_NOTE("engine terminated");
	_context.initialized = false;
}
