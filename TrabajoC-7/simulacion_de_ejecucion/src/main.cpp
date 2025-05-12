#include <iostream>
#include <pthread.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>


std::string ballena = R"(
            __ \ / __
            /  \ | /  \
                \|/
            _,.---v---._
   /\__/\  /            \
   \_  _/ /              \
     \ \_|           @ __|
      \                \_
       \     ,__/       /
     ~~~`~~~~~~~~~~~~~~/~~~~
    )";

// Estructura que contiene los mutex y las variables de condición
struct SharedData {
    pthread_mutex_t mtx_cliente;  // Mutex para sincronizar al cliente
    pthread_mutex_t mtx_tenedor;  // Mutex para sincronizar al tenedor
    pthread_mutex_t mtx_server;   // Mutex para sincronizar al server

    pthread_cond_t cv_cliente;    // Condición para el cliente
    pthread_cond_t cv_tenedor;    // Condición para el tenedor
    pthread_cond_t cv_server;     // Condición para el server
};


void* cliente(void* arg) {
    SharedData* data = (SharedData*)arg;

    std::cout << "Cliente: ¡Hola!" << std::endl;

    // Simula esperar 2 segundos antes de pedir la ballena
    sleep(2);

    std::cout << "\nCliente: Pidiendo al tenedor la información de la ballena..." << std::endl;
    std::cout << "GET/figuras/ballena  HTTP/1.1\r\nHost: 163.178.104.62\r\n\r\n" << std::endl;

    // Desbloquea al tenedor para que pueda enviar la ballena
    pthread_cond_signal(&data->cv_tenedor);

    // Bloquea al cliente hasta que el tenedor le dé la información
    pthread_cond_wait(&data->cv_cliente, &data->mtx_cliente);
    sleep(2);

    std::cout << "Cliente: ¡Recibí la información de la ballena!, ya se cual server la tiene" << std::endl;
    sleep(1);
    std::cout << "Cliente: Pidiendo la información de la ballena al server...\n" << std::endl;

    pthread_cond_signal(&data->cv_server);

    pthread_cond_wait(&data->cv_cliente, &data->mtx_cliente);
    sleep(2);
    std::cout << "cliente: El server me envió la ballena" << std::endl;
    sleep(1);
    std::cout << "cliente: BALLENA PRINT" << std::endl;
    std::cout << ballena << std::endl;

    return nullptr;
}

void* server(void* arg) {
    SharedData* data = (SharedData*)arg;

    std::cout << "Server: ¡Hola!" << std::endl;

    // Desbloquea al server para que pueda enviar la ballena
    //pthread_cond_signal(&data->cv_server,);

    pthread_cond_wait(&data->cv_server, &data->mtx_server);
    sleep(2);
    
    std::cout << "Server: Request válido, procesando..." << std::endl;
    sleep(1);
    std::cout << "Server: Un cliente ha pedido información de la ballena." << std::endl;
    sleep(1);
    std::cout << "Server: Enviando la información de la ballena al cliente...\n" << std::endl;

    // Desbloquea al cliente para que pueda continuar con la operación
    pthread_cond_signal(&data->cv_cliente);


    return nullptr;
}

void* tenedor(void* arg) {
    SharedData* data = (SharedData*)arg;

    std::cout << "Tenedor: ¡Hola!" << std::endl;

    // Espera hasta que el cliente le pida la ballena
    pthread_cond_wait(&data->cv_tenedor, &data->mtx_tenedor);
    sleep(2);
    std::cout << "Tenedor: Procesando request del cliente" << std::endl;
    sleep(1);
    std::cout << "Tenedor: Recibí la solicitud del cliente para buscar \"ballena\"" << std::endl;
    sleep(1);
    std::cout << "Tenedor: Enviando la información del server que tiene la ballena...\n" << std::endl;

    // Desbloquea al cliente para que pueda continuar con la operación
    pthread_cond_signal(&data->cv_cliente);

    return nullptr;
}

int main() {
    std::cout << "Main: ¡Hola desde el hilo principal!" << std::endl;

    // Crear memoria compartida para los mutex y variables de condición
    SharedData* data = (SharedData*)mmap(nullptr, sizeof(SharedData), PROT_READ | PROT_WRITE,
                                          MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    // Inicializar los mutex y las variables de condición
    pthread_mutex_init(&data->mtx_cliente, nullptr);
    pthread_mutex_init(&data->mtx_tenedor, nullptr);
    pthread_mutex_init(&data->mtx_server, nullptr);

    pthread_cond_init(&data->cv_cliente, nullptr);
    pthread_cond_init(&data->cv_tenedor, nullptr);
    pthread_cond_init(&data->cv_server, nullptr);

    // Crear los hilos
    pthread_t t_cliente, t_server, t_tenedor;

    pthread_create(&t_cliente, nullptr, cliente, (void*)data);
    pthread_create(&t_server, nullptr, server, (void*)data);
    pthread_create(&t_tenedor, nullptr, tenedor, (void*)data);

    // Esperar a que los hilos terminen
    pthread_join(t_cliente, nullptr);
    pthread_join(t_server, nullptr);
    pthread_join(t_tenedor, nullptr);

    // Limpiar los mutex y variables de condición
    pthread_mutex_destroy(&data->mtx_cliente);
    pthread_mutex_destroy(&data->mtx_tenedor);
    pthread_mutex_destroy(&data->mtx_server);

    pthread_cond_destroy(&data->cv_cliente);
    pthread_cond_destroy(&data->cv_tenedor);

    // Liberar la memoria compartida
    munmap(data, sizeof(SharedData));

    return 0;
}
