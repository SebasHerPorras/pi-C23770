#include "system.h"
#include "thread.h"
#include <stdint.h>  // para intptr_t

void ForkTestThread(void* which) {
    // Convertir el puntero void* a entero (el id del thread)
    int threadId = (intptr_t)which;
    
    for (int i = 0; i < 5; i++) {
        printf("Soy el thread %d, iteración %d\n", threadId, i);
        currentThread->Yield();  // cede el CPU a otro thread
    }
}

void ForkTest() {
    Thread* t1 = new Thread("thread 1");
    Thread* t2 = new Thread("thread 2");
    
    // Lanzar threads con función y parámetro
    t1->Fork(ForkTestThread, (void*)(intptr_t)1);
    t2->Fork(ForkTestThread, (void*)(intptr_t)2);

    // El thread principal también ejecuta algo
    for (int i = 0; i < 5; i++) {
        printf("Soy el thread principal, iteración %d\n", i);
        currentThread->Yield();
    }
}
