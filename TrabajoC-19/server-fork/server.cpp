/**
 *  Universidad de Costa Rica
 *  ECCI
 *  CI0123 Proyecto integrador de redes y sistemas operativos
 *  2025-i
 *  Grupos: 1 y 3
 *
 *  Socket client/server example with threads
 *
 *  (Fedora version)
 *
 **/

 #include <sys/types.h>
 #include <sys/socket.h>
 #include <netinet/in.h>
 #include <arpa/inet.h>
 #include <unistd.h>      // close()
 #include <cstring>       // memset, strerror
 #include <iostream>
 #include <thread>
 #include <algorithm>     // std::min, std::max
 #include <chrono>        // std::chrono::seconds
 #include <mutex>

 
 #include "Socket.h"
 #include "file_system.hpp"
 
 #define PORT             5353
 #define BUFSIZE          512
 
 // Parámetros de multicast
 #define MULTICAST_GROUP  "239.0.0.1"
 #define MULTICAST_PORT   5353 // este puerto funciona tanto como para el broadcast en udp como para mandar los mensajes en tcp
 // se puede usar el mismo puerto para ambos, ya que las conexiones TCP y UDP son independientes
 
 // Para identificar el servidor al anunciar y al shutdown
#define SERVER_NAME "Isla5 127.0.0.1 gato,barco,sombrilla"
#define SERVER_NAME_FINAL "Isla5 172.16.123.95/28 gato,barco,sombrilla"
 // ... [includes y defines igual que antes] ...

int global_port = PORT;  // <--- puerto global, por defecto 9090

std::mutex server_mutex; // Mutex global para proteger el socket del server

void run_server(const std::string& broadcast_ip) ;        // <-- ya sin parámetros
void task(VSocket * client);
bool process_request(char* request, char* response);

FileSystem fs(true); // Inicializa y formatea el sistema de archivos


void task(VSocket * client) {
    char request[BUFSIZE];
    client->Read(request, BUFSIZE);

    std::cout << "Server received: " << request << " from id: " << client->idSocket << std::endl;

    char figure_name[BUFSIZE];
    if (process_request(request, figure_name)) {
        std::cout << "\n\nRequested figure: " << figure_name << "\n" << std::endl;
        std::cout << std::endl;

        std::string figure = fs.find_figura(figure_name);
        client->Write(figure.c_str(), figure.size());
        } else {
        std::string error_msg = "Invalid request format";
        client->Write(error_msg.c_str(), error_msg.size());
        }

        client->Close();
    }

/**
 *   Process the request
 *      Check if the request is valid
 *      If valid, extract the figure name and return it
 *      If invalid, return an error message
 *
 **/
bool process_request(char* request, char* response) {
    std::string req_str(request);
    std::string prefix = "GET /figure/";
  
    // La línea completa debe tener al menos prefix + 1 caracter de nombre
    if (req_str.rfind(prefix, 0) == 0 && req_str.size() > prefix.size()) {
        // Extraemos todo lo que viene tras prefix hasta el final de la línea
        size_t start = prefix.size();
        size_t end = req_str.find_first_of("\r\n ", start);
        if (end == std::string::npos) {
            end = req_str.size();
        }
        std::string figure_name = req_str.substr(start, end - start);
  
        // Limpiar caracteres no deseados (solo alfanuméricos, '_' o '-')
        figure_name.erase(std::remove_if(
            figure_name.begin(), figure_name.end(),
            [](char c) { return !std::isalnum(c) && c != '_' && c != '-'; }
        ), figure_name.end());
  
        if (!figure_name.empty()) {
            // Copiamos el nombre de la figura al buffer de respuesta
            strncpy(response, figure_name.c_str(), BUFSIZE - 1);
            response[BUFSIZE - 1] = '\0';
            return true;
        }
    }

   strncpy(response, "Invalid request format", BUFSIZE - 1);
   std::cout << "falló el process request" << std::endl;
   response[BUFSIZE - 1] = '\0';
   return false;
}
// -----------------------------------------------
// NUEVA FUNCIÓN SIN PARÁMETROS
// -----------------------------------------------


void run_server(const std::string& broadcast_ip, VSocket* s1)  {
    std::cout << "hola soy un hilo planeo escuhar de la ip: " << broadcast_ip << std::endl;
    // 1) Configuración Multicast UDP
    int udpSock = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSock < 0) {
        std::cerr << "Error creando udpSock: " << strerror(errno) << "\n";
        return;
    }

    // Usar la IP de broadcast recibida
    struct sockaddr_in bcastAddr;
    memset(&bcastAddr, 0, sizeof(bcastAddr));
    bcastAddr.sin_family      = AF_INET;
    bcastAddr.sin_addr.s_addr = inet_addr(broadcast_ip.c_str());
    bcastAddr.sin_port        = htons(MULTICAST_PORT);

    bool running = true;

    std::thread announcer([&]() {
        while (running) {
            std::string msg = std::string("ANUNCIO_SERVIDOR:") + SERVER_NAME + ":" + broadcast_ip + ":" + "gato,sombrilla,barco";
            sendto(udpSock, msg.c_str(), msg.size(), 0, (struct sockaddr*)&bcastAddr, sizeof(bcastAddr));
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    });

    // No listener UDP aquí, solo broadcast

    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        // Aquí podrías agregar lógica para terminar el hilo si es necesario
    }

    announcer.join();
    close(udpSock);
}

// --------------------------------------------------
// MAIN
// --------------------------------------------------
int main(int argc, char **argv) {
    if (argc > 1) {
        try {
            global_port = std::stoi(argv[1]);
        } catch (...) {
            std::cerr << "Invalid port argument. Using default port: " << PORT << std::endl;
        }
    }

    // Lista de IPs de broadcast por isla
    std::vector<std::string> broadcast_ips = {
        "172.16.123.31",
        "172.16.123.47",
        "172.16.123.63",
        "172.16.123.79",
        "127.0.0.1",
        "172.16.123.111"
    };

    // Inicializa el socket del server una sola vez
    VSocket *s1 = new Socket('s');
    s1->Bind(global_port);
    s1->MarkPassive(5);
    std::cout << "Server started on port: " << global_port << std::endl;

    // Lanza hilos de broadcast, cada uno con su IP
    std::vector<std::thread> threads;
    threads.reserve(broadcast_ips.size());
    for (size_t i = 0; i < broadcast_ips.size(); ++i) {

        threads.emplace_back([&, i]() { run_server(broadcast_ips[i], s1); });
    }

    // Loop principal: acepta conexiones y atiende clientes
    while (true) {
        server_mutex.lock();
        VSocket *client = s1->AcceptConnection();
        server_mutex.unlock();
        std::thread(task, client).detach();
    }

    // Cleanup (no se llega aquí normalmente)
    for (auto& t : threads) {
        t.join();
    }
    delete s1;

    return 0;
}
