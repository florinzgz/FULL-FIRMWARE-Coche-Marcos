#pragma once

#ifdef __cplusplus
#include <stdint.h>
#include <stddef.h>

namespace Audio {

// Prioridad de reproducción en la cola
enum Priority : uint8_t { PRIO_LOW = 0, PRIO_NORMAL = 1, PRIO_HIGH = 2, PRIO_CRITICAL = 3 };

struct Item {
    uint16_t track;
    Priority prio;
};

// Clase estática de cola de audio
class AudioQueue {
public:
    static void init() noexcept;

    // Ahora devuelve bool → true si se encoló, false si la cola estaba llena
    static bool push(uint16_t track, Priority prio) noexcept;

    // Extrae un elemento de la cola, devuelve true si había algo
    static bool pop(Item &out) noexcept;

    // Devuelve true si la cola está vacía
    static bool empty() noexcept;

    // Actualización periódica (si la necesitas para procesar la cola)
    static void update() noexcept;

private:
    static constexpr size_t MAX_QUEUE = 8; // capacidad máxima de la cola
    static Item queue_[MAX_QUEUE];
    static size_t head_;
    static size_t tail_;
    static size_t count_;
};

} // namespace Audio

#endif // __cplusplus