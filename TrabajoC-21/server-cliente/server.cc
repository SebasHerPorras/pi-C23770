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
#include <algorithm>     // std::min, std::max
#include <chrono>        // std::chrono::seconds
#include <mutex>
#include <csignal>       // Para manejo de señales
#include <atomic>       // Para flags atómicos
#include <cstring>       // Para manejo de cadenas
#include <vector>
#include <string>
#include <arpa/inet.h>  // inet_pton
#include <unistd.h>      // close
#include <stdexcept>     // std::runtime_error
#include <cctype>        // std::isalnum




#include "Socket.h"
#include "file_system.hpp"

#define PORT 8082
#define BROADCAST_PORT 5353
#define BUFSIZE 512
#define SERVER_NAME      "Isla5|127.0.0.1|gato,barco,sombrilla"
#define SERVER_NAME_FINAL "Isla5 172.16.123.95/28 gato,barco,sombrilla"

void task( VSocket * client );
bool process_request( char* request, char* response );
void send_UDP_broadcast( VSocket* sockUDP );
void atomic_cout(const std::string& message);
void udp_listener(const std::string& listen_ip, VSocket* s2);
void signal_handler(int signal);

std::mutex cout_mutex; // Mutex para proteger el acceso a std::cout
std::mutex broadcast_mutex; // Mutex para proteger el acceso a la transmisión UDP
bool running = true; // Variable para controlar el estado del servidor
VSocket* s1 = nullptr; // Socket para conexiones TCP
VSocket* s2 = nullptr; // Socket para conexiones UDP
std::vector<std::thread> broadcast_threads; // Hilos para manejar las transmisiones UDP

void atomic_cout(const std::string& message) {
   std::lock_guard<std::mutex> guard(cout_mutex);
   std::cout << message << std::flush;
}

void signal_handler(int signal) {
    if (signal == SIGINT) {
        atomic_cout("\n[Server] Recibida señal SIGINT (Ctrl+C), cerrando servidor...\n");
        running = false;
        sleep(2); // Esperar un segundo para permitir que los hilos terminen
        atomic_cout("[Server] Cerrando sockets y finalizando hilos...\n");
       
        exit(0); // Terminar el programa
       
    }
}

FileSystem fs(true); // Inicializa el sistema de archivos y lo formatea
/**
 *   Task each new thread will run
 *      Read string from socket
 *      Write it back to client
 *
 **/
 void task(VSocket* client) {
    char request[BUFSIZE];
    client->Read(request, BUFSIZE);

    // Detectar si es una solicitud HTTP (busca "GET" o "POST")
    if (strstr(request, "favicon.ico") != nullptr) {
        client->Close();
        return;
    }

    bool is_http = (strstr(request, "GET ") != nullptr) || 
                   (strstr(request, "POST ") != nullptr);

    char figure_name[BUFSIZE];
    if (process_request(request, figure_name)) {
        atomic_cout("\nRequested figure: " + std::string(figure_name) + "\n\n");

        std::string figure = fs.find_figura(figure_name);
        
        if (is_http) {
            // Construir respuesta HTTP con formato pre y codificación UTF-8
            std::string http_response = 
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html; charset=utf-8\r\n"
                "Connection: close\r\n"
                "\r\n"
                "<!DOCTYPE html>"
                "<html>"
                "<head>"
                "<meta charset='utf-8'>"
                "<title>Figura ASCII</title>"
                "<style>"
                "pre {"
                "  font-family: monospace;"
                "  white-space: pre-wrap;"
                "  margin: 10px;"
                "  padding: 10px;"
                "  background: #f8f8f8;"
                "  border: 1px solid #ddd;"
                "}"
                "</style>"
                "</head>"
                "<body>"
                "<pre>" + figure + "</pre>"
                "</body>"
                "</html>";
            
            client->Write(http_response.c_str(), http_response.size());
        } else {
            // Respuesta directa (sin formato HTTP)
            client->Write(figure.c_str(), figure.size());
        }
    } else {
        std::string error_msg;
        atomic_cout("Error: Invalid request format or figure not found.\n");
        if (is_http) {
            error_msg = "HTTP/1.1 400 Bad Request\r\n";
            error_msg += "Content-Type: text/plain\r\n";
            error_msg += "Connection: close\r\n";
            error_msg += "\r\n";
            error_msg += "Invalid request format";
        } else {
            error_msg = "Invalid request format";
        }
        client->Write(error_msg.c_str(), error_msg.size());
    }

    client->Close();
}

// ==============================================
// PROCESAMIENTO DE SOLICITUDES
// ==============================================
bool process_request(char* request, char* response) {
    std::string req_str(request);
    
    // Para peticiones HTTP, tomar solo la primera línea
    size_t end_of_first_line = req_str.find("\r\n");
    if (end_of_first_line != std::string::npos) {
        req_str = req_str.substr(0, end_of_first_line);
    }

    // Buscar solicitud de figura
    size_t figure_pos = req_str.find("GET /figure/");
    if (figure_pos != std::string::npos) {
        size_t start = figure_pos + strlen("GET /figure/");
        size_t end = req_str.find_first_of(" ?", start); // Termina en espacio o ?
        if (end == std::string::npos) end = req_str.size();
        
        std::string figure_name = req_str.substr(start, end - start);
        strncpy(response, figure_name.c_str(), BUFSIZE - 1);
        response[BUFSIZE - 1] = '\0';
        return true;
    }
    
    return false;
}

void udp_listener(const std::string& listen_ip, VSocket* s2) {
    while (running) {
        char buffer[BUFSIZE];
        struct sockaddr_in srcAddr;
        socklen_t addrLen = sizeof(srcAddr);

        // Preparar estructura para IPv4
        memset(&srcAddr, 0, sizeof(srcAddr));
        srcAddr.sin_family = AF_INET; // IPv4
        inet_pton(AF_INET, listen_ip.c_str(), &srcAddr.sin_addr);
        srcAddr.sin_port = htons(BROADCAST_PORT); // Puerto de broadcast

        size_t bytes;

        {
            
            try {
                bytes = s2->recvFrom(buffer, BUFSIZE, &srcAddr);
            } catch (const std::exception& e) {
                std::cerr << "Error en recvFrom: " << e.what() << std::endl;
                
                continue;
            }
            
        }

        if (bytes > 0) {
            buffer[bytes] = '\0';
            std::string receivedMsg(buffer);

            std::string response;

            if (receivedMsg.find("GET /servers") == 0 || receivedMsg == "get servers") {
                response = SERVER_NAME_FINAL;
                atomic_cout("Solicitud de servidores recibida: " + receivedMsg + "\n");
                send_UDP_broadcast(s2); // Responder con el nombre del servidor

                {
                    
                    try {
                        s2->sendTo(response.c_str(), response.size(), &srcAddr);
                    } catch (const std::exception& e) {
                        std::cerr << "Error en sendTo: " << e.what() << std::endl;
                        
                        continue;
                    }
                    
                }

                // Convertir dirección a string legible
                char ipStr[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &(srcAddr.sin_addr), ipStr, INET_ADDRSTRLEN);
            }
        }
    }
   // No cerramos s2 aquí, porque puede estar compartido por otros hilos
}

void send_UDP_broadcast(VSocket* sockUDP) {
    

    std::vector<std::string> broadcast_ips = {
        "172.16.123.31",
        "172.16.123.47",
        "172.16.123.63",
        "172.16.123.79",
        "172.16.123.111",
        "10.255.255.254"  // Broadcast local
    };

    for (const auto& ip : broadcast_ips) {
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(BROADCAST_PORT);
        inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

        std::string response = SERVER_NAME;
        try {
            sockUDP->sendTo(response.c_str(), response.size(), &addr);
            atomic_cout("[ManualDiscovery] Respuesta enviada a " + ip + "\n");
        } catch (const std::exception& e) {
            atomic_cout("[ManualDiscovery] Error enviando a " + ip + ": " + e.what() + "\n");
        }
    }
}






/**
 *   Create server code
 *      Infinite for
 *         Wait for client conection
 *         Spawn a new thread to handle client request
 *
 **/
int main(int argc, char **argv) {
    // Manejo de señales para cerrar el servidor limpiamente
    std::signal(SIGINT, signal_handler); // Captura Ctrl+C para cerrar el servidor
   std::thread *worker;
   VSocket  *client;

   int port = PORT; // Default port
   if (argc > 1) {
      try {
         port = std::stoi(argv[1]); // Convert argument to integer
      } catch (const std::exception &e) {
         std::cerr << "Invalid port argument. Using default port: " << PORT << std::endl;
         port = PORT;
      }
   }

   s1 = new Socket('s');
   s2 = new Socket('d'); // Create a datagram socket for UDP broadcast

   s1->Bind(port);        // Port to access this mirror server
   s2->Bind(BROADCAST_PORT); // Bind the UDP socket to the broadcast port
   
   s1->MarkPassive(5);    // Set socket passive and backlog queue to 5 connections
   std::cout << "\nServer started on port: " << port << std::endl;
   std::cout << "UDP broadcast port: " << BROADCAST_PORT << std::endl;
   send_UDP_broadcast(s2); // Send UDP broadcast to announce server availability
   // std::vector<std::thread> broadcast_threads;
   std::vector<std::string> broadcast_ips = {
      "172.16.123.31",
      "172.16.123.47",
      "172.16.123.63",
      "172.16.123.79",
      "172.16.123.111",
      "10.255.255.254"  // Localhost para pruebas broadcast
  };
   for (const auto& ip : broadcast_ips) {
      broadcast_threads.emplace_back(udp_listener, ip, s2);
   }
while (running) {
    client = s1->AcceptConnection();       // Wait for a client connection
    worker = new std::thread(task, client);
}
   delete s1;        // Close socket in parent
   worker->join();   // Wait for thread to finish
   delete worker;    // Close thread
   std::cout << "Server finished" << std::endl;
   return 0;
}
