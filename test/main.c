#include "default_components.h"
#include "gengine.h"
#include "raylib.h"

// 1. Stress test estremo: 2400 Game Objects attivi, aggiornati e animati in parallelo
#define GRID_COLS 60
#define GRID_ROWS 40
#define ENTITY_COUNT (GRID_COLS * GRID_ROWS)
#define INTERPOLATION_LINEAR 0

static GEnginePublicContext* ctx = NULL;

// --- SUBSYSTEM RENDER ---
void Render_FrameStart(void) {
    BeginMode2D(ctx->mainCamera2D);
}

void Render_Callback(GameObjectID id, void** components) {
    Transform2DComponent* transform = (Transform2DComponent*)components[0];

    // Spostiamo i colori verso sfumature elettriche e violacee in base alla scala microscopica
    Color color = {
        (unsigned char)(transform->scale.x * 110), // Rosso dinamico in base alla dilatazione
        30,
        (unsigned char)(100 + (transform->scale.y * 60)), // Blu/Magenta reattivo
        255
    };

    // Raggio molto più piccolo (3.0f di base) per far risaltare il pattern
    DrawCircleV(transform->position, 3.0f * transform->scale.x, color);
}

void Render_FrameEnd(void) {
    EndMode2D();

    // Test dell'efficacia visiva ed FPS (se supportato dall'engine)
    DrawText(TextFormat("ENTITA' ANIMATE: %d", ENTITY_COUNT), 10, 10, 20, LIME);
    DrawText("Stress Test: Reflection + ECS Interpolation", 10, 40, 18, DARKGRAY);
    DrawText("STATO: IN RIPRODUZIONE", 10, 65, 18, BLUE);
}

// --- MAIN ---
int main(void) {
    ctx = GEngineInitialize("GEngine - Swarm Animation Demo", 800, 600);
    if (!ctx) return -1;
    ctx->backgroundColor = (Color){ 15, 15, 15, 255 };

    // Registrazione del Subsystem di rendering
    GEngineRegisterSubSystem(NULL, NULL, Render_FrameStart, Render_Callback, Render_FrameEnd,
                             GENGINE_SUBSYSTEM_TYPE_RENDER, false, 1, ctx->defaultComponents.transform2D);

    // 2. Setup delle coordinate per adattare 2400 elementi nello schermo
    float spacingX = 800.0f / GRID_COLS;
    float spacingY = 600.0f / GRID_ROWS;

    for (int y = 0; y < GRID_ROWS; y++) {
        for (int x = 0; x < GRID_COLS; x++) {
            GameObjectID entity = GEngineCreateGameObject("djdl");

            // Calcolo del posizionamento micro
            Vector2 startPos = { (x * spacingX) + (spacingX / 2), (y * spacingY) + (spacingY / 2) };
            Transform2DComponent transform = { .position = startPos, .scale = {1, 1}, .rotation = 0.0f };
            GEngineAttachComponent(entity, ctx->defaultComponents.transform2D, &transform);

            Animation anim = GEngineCreateAnimation();
            uint32_t scaleChannel = GEngineAddAnimationChannel(&anim, ctx->defaultComponents.transform2D, "scale");

            if (scaleChannel != 0) {
                // Sfasamento molto più compresso per accentuare l'effetto onda
                float timeOffset = (x * 0.02f) + (y * 0.02f);
                Vector2 kf1 = { 0.2f, 0.2f };
                Vector2 kf2 = { 2.5f, 2.5f }; // Scale superiori per ingrandimenti organici
                Vector2 kf3 = { 0.2f, 0.2f };

                GEngineAnimationAddKeyframe(&anim, scaleChannel, &kf1, 0.0f + timeOffset, GENGINE_INTERPOLATION_QUADRATIC);
                GEngineAnimationAddKeyframe(&anim, scaleChannel, &kf2, 1.0f + timeOffset, GENGINE_INTERPOLATION_QUADRATIC);
                GEngineAnimationAddKeyframe(&anim, scaleChannel, &kf3, 2.0f + timeOffset, GENGINE_INTERPOLATION_QUADRATIC);
            }

            AnimationPlayerComponent playerComp = {
                .animation = anim,
                .loopAnimation = true,
                .playOnStart = true
            };
            GEngineAttachComponent(entity, ctx->defaultComponents.animationPlayer, &playerComp);
        }
    }

    GEngineStartGame();
    while (GEngineGameWantsToRun()) {
        GEngineProcessFrame();
    }

    GEngineTerminate();
    return 0;
}
