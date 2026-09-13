#include "gengine.h"
#include <stdio.h>
#include <stddef.h> // Obbligatorio per usare offsetof()

// 1. Definiamo un componente con tipi di dimensioni diverse per generare padding
typedef struct {
    uint8_t id;         // 1 byte
                        // 3 byte di padding automatico del C
    float speed;        // 4 byte
    Vector2 position;   // 8 byte (assumendo struct di 2 float)
    bool isAlive;       // 1 byte
                        // 3 byte di padding finale per allineamento
} MovementData;

int main(void) {
    // Inizializza l'engine (potrebbe essere necessario se l'allocatore di tipi dipende dal Context)
    GEnginePublicContext* ctx = GEngineInitialize("Reflection Test", 800, 600);
    if (!ctx) return -1;

    // 2. Registriamo il componente passando le triplette: (Tipo, Nome, Offset)
    GEngineComponentTypeID moveCompID = GEngineRegisterComponent(
        sizeof(MovementData),
        "MovementData",
        4, // Numero esatto di campi
        GENGINE_FIELD_TYPE_UINT8_T,  "id",       offsetof(MovementData, id),
        GENGINE_FIELD_TYPE_FLOAT,    "speed",    offsetof(MovementData, speed),
        GENGINE_FIELD_TYPE_VECTOR2,  "position", offsetof(MovementData, position),
        GENGINE_FIELD_TYPE_BOOL,     "isAlive",  offsetof(MovementData, isAlive)
    );

    // 3. Recuperiamo le informazioni tramite l'API
    const ComponentTypeInfo* info = GEngineGetComponentTypeInfo(moveCompID);


    if (info != NULL) {
        printf("\n=== METADATI COMPONENTE: %s ===\n", info->name); // Assumo info->name esista
        // Assumo tu abbia info->size e info->fieldCount nella tua struct vera

        printf("-------------------------------------------------\n");
        // 4. Testiamo se gli offset recuperati dall'engine combaciano
        // NOTA: Sostituisci "info->fields[i]" con la struttura reale del tuo array interno
        for (uint32_t i = 0; i < info->fieldCount; i++) {
            printf("Campo [%d]: Nome: %-10s | Tipo ID: %-2d | Offset Engine: %2d byte \n",
                   i,
                   info->componentFieldsInfo[i].name,
                   info->componentFieldsInfo[i].type,
                   info->componentFieldsInfo[i].offset);
        }
        printf("-------------------------------------------------\n");

        // Verifica matematica per confronto diretto in console:
        printf("\nVerifica padding del compilatore (offsetof nativo):\n");
        printf("offsetof(id):       %zu byte\n", offsetof(MovementData, id));
        printf("offsetof(speed):    %zu byte\n", offsetof(MovementData, speed));
        printf("offsetof(position): %zu byte\n", offsetof(MovementData, position));
        printf("offsetof(isAlive):  %zu byte\n", offsetof(MovementData, isAlive));

        printf("\nSe 'Offset Engine' e 'offsetof nativo' sono identici, il sistema funziona.\n\n");
    } else {
        printf("ERRORE: Recupero ComponentTypeInfo fallito.\n");
    }

    GEngineTerminate();
    return 0;
}
