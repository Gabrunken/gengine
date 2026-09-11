#include "gengine.h"
#include "raylib.h"
#include <stdio.h>

#define MAX_ENTITIES 100
#define LOG_MESSAGES 5

// Stato simulato per il test visivo
static Vector2 visualEntities[MAX_ENTITIES];
static int entityCount = 0;
static GameObjectID nextID = {1, 1};
static GEnginePublicContext* ctx = NULL;

static char eventLog[LOG_MESSAGES][64] = {0};
static void AddLogMessage(const char* message) {
    for (int i = LOG_MESSAGES - 1; i > 0; i--) {
        snprintf(eventLog[i], 64, "%s", eventLog[i-1]);
    }
    snprintf(eventLog[0], 64, "%s", message);
}

// Callback fittizia obbligatoria per bypassare il check del subsystem
void DummyCallback(GameObjectID id, void** components) {
    (void)id;
    (void)components;
}

// --- SUBSYSTEM 1: INPUT ---
void Input_FrameStart(void) {
    if (IsKeyPressed(KEY_SPACE)) {
        GameObjectCreatedEvent ev = { .id = nextID };
        nextID.id++;

        GEnginePushEvent(ctx->defaultEventTypes.gameObjectCreated, &ev);
        AddLogMessage("[-] FRAME N: PUSH Creazione (Spazio)");
    }

    if (IsKeyPressed(KEY_BACKSPACE) && entityCount > 0) {
        GameObjectDeletedEvent ev = { .id = {nextID.id - 1, 1} };
        nextID.id--;

        GEnginePushEvent(ctx->defaultEventTypes.gameObjectDeleted, &ev);
        AddLogMessage("[-] FRAME N: PUSH Distruzione (Backspace)");
    }
}

// --- SUBSYSTEM 2: LOGICA ---
void Logic_FrameStart(void) {
    const dyarray createQueue = GEngineGetEventQueue(ctx->defaultEventTypes.gameObjectCreated);
    for (size_t i = 0; i < createQueue.elementCount; i++) {
        if (entityCount < MAX_ENTITIES) {
            visualEntities[entityCount] = (Vector2){ GetRandomValue(100, 700), GetRandomValue(100, 500) };
            entityCount++;
            AddLogMessage("[+] FRAME N+1: READ Creazione Eseguita");
        }
    }

    const dyarray deleteQueue = GEngineGetEventQueue(ctx->defaultEventTypes.gameObjectDeleted);
    for (size_t i = 0; i < deleteQueue.elementCount; i++) {
        if (entityCount > 0) {
            entityCount--;
            AddLogMessage("[+] FRAME N+1: READ Distruzione Eseguita");
        }
    }
}

// --- SUBSYSTEM 3: RENDER ---
void Render_FrameStart(void) {
    BeginMode2D(ctx->mainCamera2D);
}

void Render_FrameEnd(void) {
    for (int i = 0; i < entityCount; i++) {
        DrawCircleV(visualEntities[i], 20.0f, MAROON);
        DrawText(TextFormat("ID:%d", i+1), visualEntities[i].x - 15, visualEntities[i].y - 30, 10, DARKGRAY);
    }
    EndMode2D();

    DrawText("Premi SPAZIO per spawnare (Push Evento)", 10, 10, 20, DARKGRAY);
    DrawText("Premi BACKSPACE per distruggere (Push Evento)", 10, 40, 20, DARKGRAY);

    for (int i = 0; i < LOG_MESSAGES; i++) {
        DrawText(eventLog[i], 10, 530 + (i * 15), 10, i == 0 ? DARKGREEN : GRAY);
    }
}

// --- MAIN ---
int main(void) {
    ctx = GEngineInitialize("GEngine - Event Double Buffer Test", 800, 600);
    if (!ctx) return -1;

    ctx->backgroundColor = RAYWHITE;
    ctx->mainCamera2D.zoom = 1.0f;

    // Registriamo un componente fittizio senza campi (size = sizeof(int) per sicurezza di allocazione)
    GEngineComponentTypeID dummyComp = GEngineRegisterComponent(sizeof(int), "DummyComponent", 0);

    // Registrazione dei Subsystem passando la callback vuota, 1 componente, e l'ID del dummyComp
    GEngineRegisterSubSystem(NULL, NULL, Input_FrameStart, DummyCallback, NULL, GENGINE_SUBSYSTEM_TYPE_INPUT, false, 1, dummyComp);
    GEngineRegisterSubSystem(NULL, NULL, Logic_FrameStart, DummyCallback, NULL, GENGINE_SUBSYSTEM_TYPE_LOGIC, false, 1, dummyComp);
    GEngineRegisterSubSystem(NULL, NULL, Render_FrameStart, DummyCallback, Render_FrameEnd, GENGINE_SUBSYSTEM_TYPE_RENDER, false, 1, dummyComp);

    GEngineStartGame();

    while (GEngineGameWantsToRun()) {
        GEngineProcessFrame();
    }

    GEngineTerminate();
    return 0;
}
