/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2025-i
  *  Grupos: 1 y 3
  *
  *   Socket client/server example with threads
  *
  * (Fedora version)
  *
 **/

#include <iostream>
#include <thread>
#include <algorithm>  // std::min
#include <sstream>    // std::ostringstream

#include "Socket.h"
#include "file_system.hpp"

#define PORT    1231
#define BUFSIZE 512

// ==== Parámetros del protocolo ====
const std::string SERVER_NAME = "ServidorA";
const std::string SERVER_IP   = "172.16.123.51";
const std::vector<std::string> FIGURE_LIST = {
    "arbol_navidad", "gato", "barco", "sombrilla"
};

std::string make_announcement() {
    std::ostringstream oss;
    oss << SERVER_NAME << " | " << SERVER_IP << " | ";
    for (size_t i = 0; i < FIGURE_LIST.size(); ++i) {
        oss << FIGURE_LIST[i];
        if (i + 1 < FIGURE_LIST.size()) oss << ", ";
    }
    return oss.str();
}

// Sistema de archivos global
FileSystem fs(true);

// Prototipos
void task(VSocket * client);

// ----------------------------------
int main(int argc, char **argv) {
    VSocket * s1;
    std::thread * worker;

    s1 = new Socket('s');
    s1->Bind(PORT);
    s1->MarkPassive(5);

    std::cout << "[Protocol] Server started on port " << PORT << std::endl;

    // Al arrancar, hacemos el "broadcast" de anuncio de servidor
    std::string announce = make_announcement();
    std::cout << "[Protocol] Broadcasting announcement: " 
              << announce << std::endl;
    // (En tu implementación real harías un sendTo multicast aquí)

    // Bucle principal de aceptación de clientes/tenedores
    for (;;) {
        VSocket * client = s1->AcceptConnection();
        std::cout << "[Protocol] Accepted new connection, socket id: "
                  << client->idSocket << std::endl;
        worker = new std::thread(task, client);
    }

    // (Nunca llega aquí en un server de ciclo infinito)
    worker->join();
    delete worker;
    delete s1;
    return 0;
}

// ==================================
// Task que maneja cada conexión entrante
// ==================================
void task(VSocket * client) {
    char buffer[BUFSIZE] = {0};
    client->Read(buffer, BUFSIZE);
    std::string req(buffer);

    std::cout << "[Protocol] Received request: \"" << req 
              << "\" from socket id " << client->idSocket << std::endl;

    // 1) Descubrimiento de servidores
    if (req == "GET /servers") {
        std::string announcement = make_announcement();
        std::cout << "[Protocol] RESPONDIENDO a GET /servers con: "
                  << announcement << std::endl;
        client->Write(announcement.c_str(), announcement.size());

    // 2) Petición de figura ASCII
    } else if (req.rfind("GET /figure/", 0) == 0) {
        std::string figName = req.substr(strlen("GET /figure/"));
        std::cout << "[Protocol] Petición ASCII-art para figura: '"
                  << figName << "'" << std::endl;

        std::string figure = fs.find_figura(figName);
        if (!figure.empty()) {
            std::cout << "[Protocol] Figura encontrada ("
                      << figure.size() << " bytes), enviando en chunks..."
                      << std::endl;
            size_t total = figure.size(), sent = 0;
            while (sent < total) {
                size_t chunk = std::min((size_t)BUFSIZE, total - sent);
                client->Write(figure.c_str() + sent, chunk);
                std::cout << "[Protocol]   → Enviado chunk de " 
                          << chunk << " bytes" << std::endl;
                sent += chunk;
            }
        } else {
            std::cout << "[Protocol] Figura '" << figName 
                      << "' no encontrada, enviando 404" << std::endl;
            std::string error = "404 Not Found";
            client->Write(error.c_str(), error.size());
        }

    // 3) Shutdown de servidor
    } else if (req.rfind("Shutdown ", 0) == 0) {
        std::string target = req.substr(strlen("Shutdown "));
        std::cout << "[Protocol] Shutdown request para: '" 
                  << target << "'" << std::endl;
        if (target == SERVER_NAME) {
            std::cout << "[Protocol] Coincide con este servidor → cerrando."
                      << std::endl;
            client->Write("Shutdown ACK", strlen("Shutdown ACK"));
            client->Close();
            exit(0);
        } else {
            std::cout << "[Protocol] No es para mí, ignorando." << std::endl;
        }

    // 4) Cualquier otro request inválido
    } else {
        std::cout << "[Protocol] Request inválido: " << req << std::endl;
        std::string invalid = "Invalid request format";
        client->Write(invalid.c_str(), invalid.size());
    }

    client->Close();
}
