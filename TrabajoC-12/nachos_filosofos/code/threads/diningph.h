#include "synch.h"

class DiningPh {

public:
    DiningPh();
    ~DiningPh();
    void pickup(long who);   // Filósofo intenta tomar los palillos
    void putdown(long who);  // Filósofo deja los palillos
    void print();            // Imprime el estado de los filósofos

private:
    int state[5];            // Estado de cada filósofo: THINKING, HUNGRY, EATING
    Semaphore* self[5];      // Semáforos individuales por filósofo
    Semaphore* mutex;        // Mutex para exclusión mutua
    void test(long who);     // Verifica si un filósofo puede comer

    enum { THINKING, HUNGRY, EATING };
};
