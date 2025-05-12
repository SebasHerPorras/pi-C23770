#include "diningph.h"

DiningPh::DiningPh() {
    // Inicializa los semáforos para los palillos
    for (int i = 0; i < 5; i++) {
        chopsticks[i] = new Semaphore("chopstick", 1); // Cada palillo comienza disponible
    }
}

DiningPh::~DiningPh() {
    // Libera la memoria de los semáforos
    for (int i = 0; i < 5; i++) {
        delete chopsticks[i];
    }
}

void DiningPh::pickup(long who) {
    // Adquiere los dos palillos necesarios
    chopsticks[who]->P();               // Palillo izquierdo
    chopsticks[(who + 1) % 5]->P();     // Palillo derecho

    printf("Philosopher %ld is Eating\n", who + 1);
}

void DiningPh::putdown(long who) {
    // Libera los dos palillos
    chopsticks[who]->V();               // Palillo izquierdo
    chopsticks[(who + 1) % 5]->V();     // Palillo derecho

    printf("Philosopher %ld is Thinking\n", who + 1);
}

void DiningPh::print() {
    // Opcional: Puedes implementar una función para imprimir el estado de los palillos si lo necesitas.
    printf("Dining Philosophers state:\n");
}