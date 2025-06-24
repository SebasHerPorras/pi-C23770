#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <mutex>
#include <map>
#include <atomic>
#include "Socket.h"
#include <algorithm>
#include <cctype>
#include <csignal>
#include <chrono>

#define HTTP_PORT 8080
#define SERVER_PORT 8081
#define MULTICAST_PORT 5353
#define BUFSIZE 512
#define DISCOVERY_INTERVAL 120 // segundos

struct ServerInfo {
    std::string name;
    std::vector<std::string> figures;
};

// Variables Globales
std::mutex servers_mutex;
std::mutex udp_mutex; // Mutex para proteger el socket UDP
VSocket* s2 = nullptr; // Socket principal del servidor HTTP
std::map<std::string, ServerInfo> available_servers;
std::atomic<bool> running(true);
std::mutex cout_mutex; // Mutex para cout atómico
void manual_discover_servers();

// ==============================================
// FUNCIÓN PARA COUT ATÓMICO
// ==============================================
void atomic_cout(const std::string& message) {
    std::lock_guard<std::mutex> guard(cout_mutex);
    std::cout << message << std::flush;
}

std::string get_figure_from_server(const std::string& figure_name);

void signal_handler(int signal) {
    if (signal == SIGINT) {
        atomic_cout("\n[Fork] Cerrando...\n");
        running = false;
        sleep(2);
        exit(0);
    }
}

// ==============================================
// LISTENER UDP - Escucha respuestas de servidores
// ==============================================
void udp_listener(const std::string& listen_ip, VSocket* s2) {
    atomic_cout("[Listener] Escuchando en " + listen_ip + ":" + std::to_string(MULTICAST_PORT) + "\n");

    while (running) {
        char buffer[BUFSIZE];
        struct sockaddr_in cliaddr;
        socklen_t len = sizeof(cliaddr);
        int n = 0;

        try {
            n = s2->recvFrom(buffer, BUFSIZE - 1, &cliaddr);
        } catch (const std::exception& e) {
            std::cerr << "[Listener] Error en recvFrom: " << e.what() << std::endl;
            continue;
        }

        if (n > 0) {
            buffer[n] = '\0';  // Asegura que el mensaje sea null-terminated
            std::string msg(buffer);

            // Obtener IP del cliente
            char ip_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(cliaddr.sin_addr), ip_str, INET_ADDRSTRLEN);

            atomic_cout("[Listener] Mensaje recibido de " + std::string(ip_str) + ": " + msg + "\n");

            // Parsear mensaje con formato: "Nombre|IP|fig1,fig2"
            size_t p1 = msg.find('|');
            size_t p2 = msg.find('|', p1 + 1);

            if (p1 != std::string::npos && p2 != std::string::npos) {
                ServerInfo info;
                info.name = msg.substr(0, p1);
                std::string ip = msg.substr(p1 + 1, p2 - (p1 + 1));
                std::string figs = msg.substr(p2 + 1);

                // Extraer figuras separadas por coma
                size_t start = 0;
                size_t end = figs.find(',');
                while (end != std::string::npos) {
                    info.figures.push_back(figs.substr(start, end - start));
                    start = end + 1;
                    end = figs.find(',', start);
                }
                info.figures.push_back(figs.substr(start));

                // Guardar en el mapa protegido por mutex
                {
                    std::lock_guard<std::mutex> lock(servers_mutex);
                    available_servers[ip] = info;

                    std::string server_info = "[Discovery] Servidor encontrado: " + info.name + " (" + ip + ") Figuras: ";
                    for (const auto& fig : info.figures) {
                        server_info += fig + " ";
                    }
                    server_info += "\n";
                    atomic_cout(server_info);
                }
            }
        }
    }
}



// ==============================================
// descubrimiento de servers de manera manual, envia el menaje a los servidores y se sale
// ==============================================
void manual_discover_servers() {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        atomic_cout("[ManualDiscovery] Error creando socket: " + std::string(strerror(errno)) + "\n");
        return;
    }

    // Habilitar broadcast
    int broadcast = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));

    struct sockaddr_in bcast_addr;
    memset(&bcast_addr, 0, sizeof(bcast_addr));
    bcast_addr.sin_family = AF_INET;
    bcast_addr.sin_port = htons(MULTICAST_PORT);

    const char* discovery_msg = "GET /servers";
    std::vector<std::string> bcast_ips = {
        "172.16.123.31",
        "172.16.123.47",
        "172.16.123.63",
        "172.16.123.79",
        "172.16.123.111",
        "10.255.255.254"
    };

    atomic_cout("[ManualDiscovery] Enviando solicitud de descubrimiento manual...\n");
    atomic_cout("[ManualDiscovery] mensaje enviado: '" + std::string(discovery_msg) + "'"+ "\n");
    for (const auto& ip : bcast_ips) {
        inet_pton(AF_INET, ip.c_str(), &bcast_addr.sin_addr);
        sendto(sockfd, discovery_msg, strlen(discovery_msg), 0, 
              (struct sockaddr*)&bcast_addr, sizeof(bcast_addr));
    }

    close(sockfd);
}

// ==============================================
// DESCUBRIMIENTO DE SERVIDORES (Broadcast)
// ==============================================
void discover_servers() {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        atomic_cout("[Discovery] Error creando socket: " + std::string(strerror(errno)) + "\n");
        return;
    }

    // Habilitar broadcast
    int broadcast = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));

    struct sockaddr_in bcast_addr;
    memset(&bcast_addr, 0, sizeof(bcast_addr));
    bcast_addr.sin_family = AF_INET;
    bcast_addr.sin_port = htons(MULTICAST_PORT);

    const char* discovery_msg = "GET /servers";
    std::vector<std::string> bcast_ips = {
        "172.16.123.31",
        "172.16.123.47",
        "172.16.123.63",
        "172.16.123.79",
        "172.16.123.111",
        "10.255.255.254"
    };

    while (running) {
        atomic_cout("[Discovery] Enviando solicitud de descubrimiento...\n");
        
        for (const auto& ip : bcast_ips) {
            inet_pton(AF_INET, ip.c_str(), &bcast_addr.sin_addr);
            sendto(sockfd, discovery_msg, strlen(discovery_msg), 0, 
                  (struct sockaddr*)&bcast_addr, sizeof(bcast_addr));
        }

        // Esperar 120 segundos
        for (int i = 0; i < DISCOVERY_INTERVAL && running; ++i) {
            sleep(1);
        }
    }
    close(sockfd);
}

// ==============================================
// MANEJO DE HTTP
// ==============================================
void handle_http_request(VSocket* client) {
    //atomic_cout("tengo un request\n");
    char request[BUFSIZE];
    client->Read(request, BUFSIZE);

    if (strstr(request, "favicon.ico") != nullptr) {
        client->Close();
        delete client;
        return;
    }

    std::string req_str(request);

    // recibir una solicitud de GET /servers para llamar a discover_servers e imprimir la lista de servidores
    if (req_str.find("GET /servers") != std::string::npos) {
        // hacer discover_servers "manualmente"
        manual_discover_servers();
        // slee[ de 3 segundos para esperar a que se reciban las respuestas]
        sleep(3);
        // responder con la lista de servidores disponibles
        atomic_cout("Solicitud GET /servers recibida\n");
        std::string response = "HTTP/1.1 200 OK\r\n"
                              "Content-Type: text/plain; charset=utf-8\r\n"
                              "Connection: close\r\n\r\n";
        response += "Servidores disponibles:\n";
        for (const auto& server : available_servers) {
            response += server.second.name + " (" + server.first + ") - Figuras: ";
            for (const auto& fig : server.second.figures) {
                response += fig + " ";
            }
            response += "\n";
        }
        client->Write(response.c_str(), response.size());
        client->Close();
        delete client;
        return;
    }
    size_t start = req_str.find("GET /figure/");
    if (start == std::string::npos) {
        client->Close();
        delete client;
        return;
    }
    
    start += strlen("GET /figure/");
    size_t end = req_str.find(' ', start);
    std::string figure_name = req_str.substr(start, end-start);
    atomic_cout("me pidieron figura: " + figure_name + "\n");
    std::string figure_data = get_figure_from_server(figure_name);
    
    // Construir respuesta HTTP
    std::string response;
    if (!figure_data.empty()) {
        atomic_cout("Figura '" + figure_name + "' encontrada, enviando datos...\n");
        response = "HTTP/1.1 200 OK\r\n"
                  "Content-Type: text/html; charset=utf-8\r\n"
                  "Connection: close\r\n\r\n"
                  "<!DOCTYPE html><html><head><title>" + figure_name + 
                  "</title><style>pre{font-family:monospace;}</style></head>"
                  "<body><pre>" + figure_data + "</pre></body></html>";
    } else {
        response = "HTTP/1.1 404 Not Found\r\n"
                  "Content-Type: text/html\r\n"
                  "Connection: close\r\n\r\n"
                  "<!DOCTYPE html><html><body>"
                  "<h1>404 - Figura no encontrada</h1>"
                  "</body></html>";
    }

    client->Write(response.c_str(), response.size());
    client->Close();
    delete client;
}

std::string get_figure_from_server(const std::string& figure_name) {
    std::string ip;
    
    // Buscar en qué servidor está la figura
    {
        std::lock_guard<std::mutex> lock(servers_mutex);
        for (const auto& server : available_servers) {
            // Buscar la figura en la lista del servidor
            if (std::find(server.second.figures.begin(), 
                          server.second.figures.end(), 
                          figure_name) != server.second.figures.end()) {
                ip = server.first;
                break;
            }
        }
    }

    if (ip.empty()) {
        atomic_cout("Figura '" + figure_name + "' no encontrada en ningún servidor conocido\n");
        return "";
    }

    atomic_cout("Obteniendo figura '" + figure_name + "' del servidor " + ip + "\n");

    // Crear socket TCP
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        atomic_cout("Error creando socket TCP: " + std::string(strerror(errno)) + "\n");
        return "";
    }

    // Configurar dirección del servidor
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);
    serv_addr.sin_addr.s_addr = inet_addr(ip.c_str());

    // Conectar al servidor
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        atomic_cout("Error conectando a " + ip + ": " + std::string(strerror(errno)) + "\n");
        close(sockfd);
        return "";
    }

    // Enviar solicitud sin HTTP
    std::string request = "GET /figure/" + figure_name + "\r\n\r\n";

    if (send(sockfd, request.c_str(), request.size(), 0) < 0) {
        atomic_cout("Error enviando solicitud: " + std::string(strerror(errno)) + "\n");
        close(sockfd);
        return "";
    }

    // Recibir respuesta
    std::string response;
    char buffer[BUFSIZE];
    ssize_t bytes_read;
    
    while ((bytes_read = recv(sockfd, buffer, BUFSIZE - 1, 0)) > 0) {
        buffer[bytes_read] = '\0';
        response.append(buffer, bytes_read);
    }

    close(sockfd);

    // Extraer cuerpo de la respuesta HTTP
    size_t header_end = response.find("\r\n\r\n");
    if (header_end != std::string::npos) {
        return response.substr(header_end + 4);
    }

    return response;
}

// ==============================================
// MAIN
// ==============================================
int main() {
    std::signal(SIGINT, signal_handler);

    // 1. Iniciar listeners UDP para cada IP
    std::vector<std::string> listen_ips = {
        "172.16.123.31",
        "172.16.123.47",
        "172.16.123.63",
        "172.16.123.79",
        "172.16.123.111",
        "10.255.255.254"
    };
    s2 = new Socket('d'); // Crear socket UDP para escuchar
    if (s2->Bind(MULTICAST_PORT) < 0) {
        atomic_cout("Error al bindear el socket UDP: " + std::string(strerror(errno)) + "\n");
        delete s2;
        return 1;
    }
    std::vector<std::thread> listener_threads;
    for (const auto& ip : listen_ips) {
        listener_threads.emplace_back(udp_listener, ip, s2);
    }

    // 2. Hilo de descubrimiento periódico
    std::thread discovery_thread(discover_servers);

    // 3. Servidor HTTP
    VSocket* http_sock = new Socket('s');
    if (http_sock->Bind(HTTP_PORT) < 0) {
        atomic_cout("Error al bindear puerto HTTP " + std::to_string(HTTP_PORT) + "\n");
        running = false;
    } else {
        http_sock->MarkPassive(5);
        atomic_cout("[HTTP] Escuchando en puerto " + std::to_string(HTTP_PORT) + "\n");
        
        while (running) {
            VSocket* client = http_sock->AcceptConnection();
            if (client) {
                std::thread(handle_http_request, client).detach();
            }
        }
    }

    // Limpieza
    running = false;
    discovery_thread.join();
    for (auto& t : listener_threads) t.join();
    delete http_sock;
    return 0;
}