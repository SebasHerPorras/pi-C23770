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
 #include <csignal>       // Para manejo de señales
 #include <atomic>       // Para flags atómicos
 
 
 #include "Socket.h"
 #include "file_system.hpp"
 
 // Configuración de puertos
 #define DEFAULT_PORT     8087
 #define MULTICAST_PORT   5353
 #define BUFSIZE          512
 
 // Parámetros de multicast
 #define SERVER_NAME      "Isla5|127.0.0.1|gato,barco,sombrilla"
 #define SERVER_IP        "127.0.1" // IP de broadcast para pruebas locales 
 #define SERVER_NAME_FINAL "Isla5 172.16.123.95/28 gato,barco,sombrilla"
 
 // Variables globales
 std::atomic<bool> running(true);  // Flag de control para los hilos
 std::mutex server_mutex;          // Mutex global para proteger el socket del server
 int global_port = DEFAULT_PORT;   // Puerto global, configurable
 
 FileSystem fs(true); // Inicializa y formatea el sistema de archivos
 VSocket *s1 = nullptr; // Socket principal del servidor
 
 // Prototipos
 void broad_cast(const std::string& broadcast_ip, VSocket* s1);
 void task(VSocket * client);
 bool process_request(char* request, char* response);
 void signal_handler(int signal);
 
 // ==============================================
 // MANEJADOR DE SEÑALES (Ctrl+C)
 // ==============================================
 void signal_handler(int signal) {
     if (signal == SIGINT) {
         std::cout << "\n[Server] Recibida señal SIGINT (Ctrl+C), cerrando servidor..." << std::endl;
         running = false;
         sleep(2); // Esperar un segundo para permitir que los hilos terminen
         delete s1;
         exit(0); // Salir limpiamente
     }
 }
 
 // ==============================================
 // TASK - Manejo de conexiones de clientes
 // ==============================================
 void task(VSocket* client) {
    char request[BUFSIZE];
    client->Read(request, BUFSIZE);

    // std::cout << "Server received: " << request << " from id: " << client->idSocket << std::endl;

    // Detectar si es una solicitud HTTP (busca "GET" o "POST")
    if (strstr(request, "favicon.ico") != nullptr) {
        client->Close();
        return;
    }

    bool is_http = (strstr(request, "GET ") != nullptr) || 
                   (strstr(request, "POST ") != nullptr);

    char figure_name[BUFSIZE];
    if (process_request(request, figure_name)) {
        std::cout << "\nRequested figure: " << figure_name << "\n" << std::endl;

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
        std::cout << "Error: Invalid request format or figure not found." << std::endl;
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
 
 // ==============================================
 // SERVIDOR MULTICAST
 // ==============================================
 void broad_cast(const std::string& broadcast_ip, VSocket* s1) {
    std::cout << "Iniciando servidor UDP (envío/recepción) para IP: " << broadcast_ip << std::endl;

    // ==== Configuración del socket UDP ====
    int udpSock = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSock < 0) {
        std::cerr << "Error creando socket UDP: " << strerror(errno) << std::endl;
        return;
    }

    // Permitir reuso de dirección (para multicast/broadcast)
    int reuse = 1;
    if (setsockopt(udpSock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        std::cerr << "Error configurando SO_REUSEADDR: " << strerror(errno) << std::endl;
        close(udpSock);
        return;
    }

    // Configurar dirección local (para recibir)
    struct sockaddr_in localAddr;
    memset(&localAddr, 0, sizeof(localAddr));
    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = htonl(INADDR_ANY);  // Recibir en todas las interfaces
    localAddr.sin_port = htons(MULTICAST_PORT);      // Mismo puerto de anuncios

    if (bind(udpSock, (struct sockaddr*)&localAddr, sizeof(localAddr)) < 0) {
        std::cerr << "Error haciendo bind UDP: " << strerror(errno) << std::endl;
        close(udpSock);
        return;
    }

    // Configurar dirección de broadcast/multicast (para enviar)
    struct sockaddr_in bcastAddr;
    memset(&bcastAddr, 0, sizeof(bcastAddr));
    bcastAddr.sin_family = AF_INET;
    bcastAddr.sin_addr.s_addr = inet_addr(broadcast_ip.c_str());
    bcastAddr.sin_port = htons(MULTICAST_PORT);

    // ==== Lógica principal ====
    while (running) {
        // 1. Enviar anuncio periódico
        std::this_thread::sleep_for(std::chrono::seconds(2)); // Esperar 2 segundos entre anuncios
        std::string msg = std::string(SERVER_NAME);
        sendto(udpSock, msg.c_str(), msg.size(), 0, 
               (struct sockaddr*)&bcastAddr, sizeof(bcastAddr));
        //std::cout << "Anuncio enviado: " << msg << " a " << broadcast_ip << std::endl;
        // 2. Recibir mensajes (con timeout)
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(udpSock, &readfds);

        struct timeval timeout;
        timeout.tv_sec = 5;  // Esperar máximo 5 segundos
        timeout.tv_usec = 0;

        int ready = select(udpSock + 1, &readfds, NULL, NULL, &timeout);
        if (ready > 0 && FD_ISSET(udpSock, &readfds)) {
            char buffer[BUFSIZE];
            struct sockaddr_in srcAddr;
            socklen_t addrLen = sizeof(srcAddr);

            ssize_t bytes = recvfrom(udpSock, buffer, BUFSIZE, 0, 
                                    (struct sockaddr*)&srcAddr, &addrLen);
            if (bytes > 0) {
                buffer[bytes] = '\0';
                std::string receivedMsg(buffer);
                std::cout << "Mensaje UDP recibido: " << receivedMsg << std::endl;
                // Aquí procesarías el mensaje (ej: actualizar lista de servidores)
                break;
            }
        }

        if (!running) break;
    }

    close(udpSock);
    std::cout << "Servidor UDP finalizado para IP " << broadcast_ip << std::endl;
}
 
 // ==============================================
 // MAIN
 // ==============================================
 int main(int argc, char **argv) {
     // Configurar manejador de señales
     std::signal(SIGINT, signal_handler);
 
     // Configurar puerto si se especifica
     if (argc > 1) {
         try {
             global_port = std::stoi(argv[1]);
             std::cout << "Usando puerto configurado: " << global_port << std::endl;
         } catch (...) {
             std::cerr << "Puerto inválido. Usando puerto por defecto: " << DEFAULT_PORT << std::endl;
             global_port = DEFAULT_PORT;
         }
     }
 
     // Lista de IPs de broadcast por isla
     std::vector<std::string> broadcast_ips = {
         "172.16.123.31",
         "172.16.123.47",
         "172.16.123.63",
         "172.16.123.79",
         "172.16.123.111",
         "127.0.0.1"  // Localhost para pruebas
     };
 
     // Inicializar socket principal
     s1 = new Socket('s');
     if (s1->Bind(global_port) < 0) {
         std::cerr << "Error al bindear el puerto " << global_port << ": " << strerror(errno) << std::endl;
         delete s1;
         return 1;
     }
     s1->MarkPassive(5);
     std::cout << "Servidor iniciado en puerto: " << global_port << std::endl;
 
     // Lanzar hilos de broadcast
     std::vector<std::thread> threads;
     for (const auto& ip : broadcast_ips) {
         threads.emplace_back(broad_cast, ip, s1);
     }
 
     // Bucle principal de conexiones
     std::cout << "server se pondrá a escuchar solicitudes" << std::endl;
     while (running) {
         server_mutex.lock();
         VSocket *client = s1->AcceptConnection();
         server_mutex.unlock();
         
         if (client != nullptr) {
             std::thread(task, client).detach();
         } else if (!running) {
             break;  // Salir si se recibió señal de terminación
         }
     }
 
     // Limpieza antes de salir
     std::cout << "Cerrando servidor..." << std::endl;
     
     // Esperar a que terminen los hilos de broadcast
     for (auto& t : threads) {
         if (t.joinable()) t.join();
     }
     
     delete s1;
     std::cout << "Servidor detenido correctamente." << std::endl;
     return 0;
 }