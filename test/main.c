#include "default_components.h"
#include "gengine.h"
#include "raylib.h"
#include <stddef.h>

#define INTERPOLATION_LINEAR 0

// Stato globale del test
static GameObjectID testEntity;
static Animation testAnim;
static bool isAnimPaused = false;
static GEnginePublicContext* ctx = NULL;

// --- SUBSYSTEM LOGIC (Input + Logica) ---
void Logic_FrameStart(void) {
    if (IsKeyPressed(KEY_SPACE)) {
        isAnimPaused = !isAnimPaused;
        if (isAnimPaused) {
            GEnginePauseAnimation(testEntity);
        } else {
            GEngineResumeAnimation(testEntity);
        }
    }
}

// Callback obbligatoria per il Subsystem Logic (richiede 1 componente)
void Logic_Callback(GameObjectID id, void** components) {
    (void)id;
    (void)components;
    // In questo test specifico non ci serve manipolare dati qui,
    // l'animazione sta già modificando la memoria dietro le quinte
}


// --- SUBSYSTEM RENDER ---
void Render_FrameStart(void) {
    // L'engine ha già fatto BeginDrawing() e ClearBackground().
    // Noi apriamo solo la telecamera.
    BeginMode2D(ctx->mainCamera2D);
}

// L'iteratore dell'ECS passa direttamente i componenti richiesti
void Render_Callback(GameObjectID id, void** components) {
    (void)id;
    // components[0] è garantito essere il Transform2D, perché è l'unico che abbiamo registrato per questo sistema
    Transform2DComponent* transform = (Transform2DComponent*)components[0];

    DrawCircleV(transform->position, 25.0f, MAROON);
}

void Render_FrameEnd(void) {
    // Chiudiamo la telecamera
    EndMode2D();

    // UI in Screen Space (fuori dalla matrice della telecamera)
    DrawText("Test Animazione tramite Reflection", 10, 10, 20, DARKGRAY);
    DrawText(isAnimPaused ? "STATO: IN PAUSA (Premi Spazio)" : "STATO: IN RIPRODUZIONE", 10, 40, 20, isAnimPaused ? RED : BLUE);
}

// --- MAIN ---
int main(void) {
    ctx = GEngineInitialize("GEngine - Animation API Test", 800, 600);
    if (!ctx) return -1;

    // Configurazione del contesto
    ctx->backgroundColor = RAYWHITE;
    ctx->mainCamera2D.zoom = 1.0f;
    ctx->mainCamera2D.target = (Vector2){ 0.0f, 0.0f };
    ctx->mainCamera2D.offset = (Vector2){ 0.0f, 0.0f };


    // 2. Registrazione Subsystems (Notare l'uso di GENGINE_SUBSYSTEM_TYPE_LOGIC e il passaggio della callback)
    GEngineRegisterSubSystem(NULL, NULL, Logic_FrameStart, Logic_Callback, NULL, GENGINE_SUBSYSTEM_TYPE_LOGIC, false, 1, ctx->defaultComponents.transform2D);
    GEngineRegisterSubSystem(NULL, NULL, Render_FrameStart, Render_Callback, Render_FrameEnd, GENGINE_SUBSYSTEM_TYPE_RENDER, false, 1, ctx->defaultComponents.transform2D);

    // 3. Creazione Entità
    // Sostituisci con le tue reali API ECS per lo spawn e l'attach
    testEntity = GEngineCreateGameObject("name");
    Transform2DComponent initialTransform = { .position = {100, 300}, .scale = {1, 1}, .rotation = 0.0f };
    GEngineAttachComponent(testEntity, ctx->defaultComponents.transform2D, &initialTransform);

    // 4. Inizializzazione Animazione
    testAnim = GEngineCreateAnimation();
    uint32_t posChannel = GEngineAddAnimationChannel(&testAnim, ctx->defaultComponents.transform2D, "position");

    if (posChannel != 0) {
        Vector2 kf1 = { 100.0f, 300.0f };
        Vector2 kf2 = { 700.0f, 300.0f };
        Vector2 kf3 = { 400.0f, 500.0f };
        Vector2 kf4 = { 100.0f, 300.0f };

        GEngineAnimationAddKeyframe(&testAnim, posChannel, &kf1, 0.0f, GENGINE_INTERPOLATION_QUADRATIC);
        GEngineAnimationAddKeyframe(&testAnim, posChannel, &kf2, 1.5f, GENGINE_INTERPOLATION_QUADRATIC);
        GEngineAnimationAddKeyframe(&testAnim, posChannel, &kf3, 3.0f, GENGINE_INTERPOLATION_QUADRATIC);
        GEngineAnimationAddKeyframe(&testAnim, posChannel, &kf4, 4.5f, GENGINE_INTERPOLATION_QUADRATIC);
    }

    AnimationPlayerComponent comp = {.animation = testAnim, .loopAnimation = true};
    GEngineAttachComponent(testEntity, ctx->defaultComponents.animationPlayer, &comp);
   	//GEnginePlayAnimation(testEntity, &testAnim);

    // 5. Game Loop
    GEngineStartGame();
    while (GEngineGameWantsToRun()) {
        GEngineProcessFrame();
    }

    // 6. Pulizia
    GEngineFreeAnimation(&testAnim);
    GEngineTerminate();
    return 0;
}
