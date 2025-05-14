#include "diningph.h"

enum { THINKING, HUNGRY, EATING };

DiningPh::DiningPh() {
    for (int i = 0; i < 5; i++) {
        state[i] = THINKING;
        self[i] = new Semaphore("self", 0); // Semáforo privado para cada filósofo
    }
    mutex = new Semaphore("mutex", 1); // Para la sección crítica
}

DiningPh::~DiningPh() {
    for (int i = 0; i < 5; i++) {
        delete self[i];
    }
    delete mutex;
}

void DiningPh::pickup(long who) {
    mutex->P();

    state[who] = HUNGRY;
    printf("Philosopher %ld is Hungry\n", who + 1);

    test(who);

    mutex->V();

    // Esperar si no pudo comer
    self[who]->P();
}

void DiningPh::putdown(long who) {
    mutex->P();

    state[who] = THINKING;
    printf("Philosopher %ld puts down chopsticks and is Thinking\n", who + 1);

    // Revisar si vecinos pueden comer ahora
    test((who + 4) % 5); // LEFT
    test((who + 1) % 5); // RIGHT

    mutex->V();
}

void DiningPh::test(long who) {
    if (state[who] == HUNGRY &&
        state[(who + 4) % 5] != EATING &&
        state[(who + 1) % 5] != EATING) {
        
        state[who] = EATING;
        printf("Philosopher %ld takes forks and starts Eating\n", who + 1);

        self[who]->V(); // Despierta al filósofo
    }
}

void DiningPh::print() {
    printf("Dining Philosophers state:\n");
    for (int i = 0; i < 5; i++) {
        const char* st = (state[i] == THINKING) ? "Thinking" :
                         (state[i] == HUNGRY) ? "Hungry" : "Eating";
        printf("Philosopher %d: %s\n", i + 1, st);
    }
}
