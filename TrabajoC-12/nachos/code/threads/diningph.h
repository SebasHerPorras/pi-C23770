#include "synch.h"

class DiningPh {

public:
    DiningPh();
    ~DiningPh();
    void pickup(long who);  // Filósofo intenta tomar los palillos
    void putdown(long who); // Filósofo deja los palillos
    void print();           // Imprime el estado de los filósofos

private:
    Semaphore *chopsticks[5]; // Semáforos para los 5 palillos
};