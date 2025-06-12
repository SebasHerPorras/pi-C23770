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
#include "Socket.h" // Asegúrate de que este archivo exista y sea correcto
#include <algorithm>
#include <cctype>
#include <csignal>  // para std::signal y signal handling

#define HTTP_PORT 8084
#define SERVER_PORT 8087
#define MULTICAST_PORT 5353
#define BUFSIZE 512

// Estructura para guardar info de servidores
struct ServerInfo {
    std::string name;
    std::vector<std::string> figures;
};

// --- Variables Globales ---
std::mutex servers_mutex;
std::map<std::string, ServerInfo> available_servers; // IP del servidor -> Info
std::atomic<bool> running(true);
void signal_handler(int signal) {
    if (signal == SIGINT) {
        std::cout << "\n[Server] Recibida señal SIGINT (Ctrl+C), cerrando servidor..." << std::endl;
        running = false;
        sleep(2); // Esperar un segundo para permitir que los hilos terminen
       
        exit(0); // Salir limpiamente
    }
}

// ==============================================
// LISTENER UDP - Descubre servidores disponibles
// MODIFICADO: Ahora recibe la IP en la que debe escuchar
// ==============================================
void udp_listener(const std::string& listen_ip) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        std::cerr << "\n[Listener UDP] Error creando socket para IP " << listen_ip << ": " << strerror(errno) << std::endl;
        return;
    }
    
    // Permitir reutilizar la dirección para que múltiples listeners puedan funcionar
    int reuse = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        std::cerr << "[Listener UDP] Error en setsockopt(SO_REUSEADDR) para " << listen_ip << ": " << strerror(errno) << std::endl;
        close(sockfd);
        return;
    }

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    
    // Convertir IP a binario usando inet_pton (más robusto que inet_addr)
    if (inet_pton(AF_INET, listen_ip.c_str(), &servaddr.sin_addr) <= 0) {
        std::cerr << "[Listener UDP] IP inválida o no soportada: " << listen_ip << std::endl;
        close(sockfd);
        return;
    }

    servaddr.sin_port = htons(MULTICAST_PORT);

    // Intentar bindear el socket a la IP específica y puerto
    if (bind(sockfd, (const struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        std::cerr << "[Listener UDP] Error en bind para " << listen_ip << ":" << MULTICAST_PORT << " : " << strerror(errno) << std::endl;
        close(sockfd);
        return;
    }

    std::cout << "[Listener UDP] Escuchando anuncios en " << listen_ip << ":" << MULTICAST_PORT << std::endl;

    while (running) {
        char buffer[BUFSIZE];
        struct sockaddr_in cliaddr;
        socklen_t len = sizeof(cliaddr);
        
        // Timeout para que el hilo pueda terminar si running es false
        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        int n = recvfrom(sockfd, buffer, BUFSIZE - 1, 0, (struct sockaddr*)&cliaddr, &len);
        if (n > 0) {
            buffer[n] = '\0';
            std::string msg(buffer);
            
            std::cout << "[Listener UDP] Recibido mensaje: " << msg << std::endl;

            // Enviar un saludo de vuelta
            std::string saludo = "Hola desde el fork!";
            sendto(sockfd, saludo.c_str(), saludo.size(), 0, (struct sockaddr*)&cliaddr, len);

            // Parseo del mensaje
            size_t pos1 = 0;
            size_t pos2 = msg.find('|');
            size_t pos3 = msg.find('|', pos2 + 1);

            if (pos2 != std::string::npos && pos3 != std::string::npos) {
                ServerInfo info;
                info.name = msg.substr(pos1, pos2 - pos1);
                std::string server_ip = msg.substr(pos2 + 1, pos3 - (pos2 + 1));

                std::string figures_str = msg.substr(pos3 + 1);
                size_t start = 0;
                size_t end = figures_str.find(',');

                while (end != std::string::npos) {
                    info.figures.push_back(figures_str.substr(start, end - start));
                    start = end + 1;
                    end = figures_str.find(',', start);
                }
                info.figures.push_back(figures_str.substr(start));

                {
                    std::lock_guard<std::mutex> lock(servers_mutex);
                    available_servers[server_ip] = info;

                    std::cout << "[Discovery] Lista de servidores conocidos:" << std::endl;
                    for (const auto& pair : available_servers) {
                        std::cout << "  IP: " << pair.first << ", Nombre: " << pair.second.name << ", Figuras: ";
                        for (const auto& fig : pair.second.figures) {
                            std::cout << fig << " ";
                        }
                        std::cout << std::endl;
                    }
                }
                break; // Salir si recibimos un mensaje válido
            }
        } else if (n < 0 && errno != EWOULDBLOCK && errno != EAGAIN) {
            std::cerr << "[Listener UDP] Error en recvfrom: " << strerror(errno) << std::endl;
            break;
        }
    }

    close(sockfd);
    std::cout << "[Listener UDP] Hilo para " << listen_ip << " terminado." << std::endl;
}


// ==============================================
// OBTENER FIGURA DE SERVIDOR REMOTO (Sin cambios)
// ==============================================
std::string get_figure_from_server(const std::string& ip, const std::string& figure_name) {
    // Crear socket TCP
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "Error creando socket TCP: " << strerror(errno) << std::endl;
        return "";
    }

    // Configurar dirección del fork
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);

    // Convertir IP con inet_addr y validar
    in_addr_t ip_bin = inet_addr(ip.c_str());
    if (ip_bin == INADDR_NONE) {
        std::cerr << "Dirección IP inválida: " << ip << std::endl;
        close(sockfd);
        return "";
    }
    serv_addr.sin_addr.s_addr = ip_bin;

    // Conectar al servidor
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Error conectando a " << ip << ": " << strerror(errno) << std::endl;
        close(sockfd);
        return "";
    }

    // Enviar solicitud
    std::string request = "GET /figure/" + figure_name + "\r\n";
    if (send(sockfd, request.c_str(), request.size(), 0) < 0) {
        std::cerr << "Error enviando solicitud: " << strerror(errno) << std::endl;
        close(sockfd);
        return "";
    }

    // Recibir respuesta completa
    std::string response;
    char buffer[BUFSIZE];
    ssize_t bytes_read;
    while ((bytes_read = recv(sockfd, buffer, BUFSIZE - 1, 0)) > 0) {
        buffer[bytes_read] = '\0';
        response.append(buffer, bytes_read);
    }

    close(sockfd);

    // Extraer el cuerpo de la respuesta HTTP si es necesario
    size_t header_end = response.find("\r\n\r\n");
    if (header_end != std::string::npos) {
        return response.substr(header_end + 4);
    }
    return response;
}

// ==============================================
// MANEJO DE SOLICITUDES HTTP (Sin cambios, pero usando VSocket de tu código original)
// ==============================================
void handle_http_request(VSocket* client) {
    char request[BUFSIZE];
    client->Read(request, BUFSIZE);

    if (strstr(request, "favicon.ico") != nullptr) {
        client->Close();
        delete client;
        return;
    }

    std::string req_str(request);
    size_t start = req_str.find("GET /figure/");
    if (start == std::string::npos) {
        client->Close();
        delete client;
        return;
    }
    
    start += strlen("GET /figure/");
    size_t end = req_str.find(' ', start);
    // 1. Extraer el nombre tal cual
    std::string figure_name = req_str.substr(start, end - start);

    // 2. Trim de espacios en ambos extremos
    auto is_not_space = [](char c){ return !std::isspace(static_cast<unsigned char>(c)); };
    figure_name.erase(figure_name.begin(),
        std::find_if(figure_name.begin(), figure_name.end(), is_not_space));
    figure_name.erase(
        std::find_if(figure_name.rbegin(), figure_name.rend(), is_not_space).base(),
        figure_name.end());

    // 3. Filtrar sólo caracteres válidos (letras, dígitos, guión bajo y guión)
    figure_name.erase(
        std::remove_if(
            figure_name.begin(),
            figure_name.end(),
            [](char c){
                return !(std::isalnum(static_cast<unsigned char>(c)) 
                        || c == '_' || c == '-');
            }
        ),
        figure_name.end()
    );

std::cout << "Buscando figura: " << figure_name << std::endl;
    
    std::string figure;
    std::string server_ip_found;

    {
        std::lock_guard<std::mutex> lock(servers_mutex);
        for (const auto& server : available_servers) {
            for (const auto& fig : server.second.figures) {
                if (fig == figure_name) {
                    server_ip_found = server.first;
                    break;
                }
            }
            if (!server_ip_found.empty()) break;
        }
    }

    if (!server_ip_found.empty()) {
        //std::cout << "Figura '" << figure_name << "' encontrada en el servidor " << server_ip_found << ". Solicitando..." << std::endl;
        figure = get_figure_from_server(server_ip_found, figure_name);
        std::cout << "Figura '" << figure_name << "' obtenida de " << server_ip_found << std::endl;
    }

    std::string response;
    if (!figure.empty()) {
        response = "HTTP/1.1 200 OK\r\n"
                   "Content-Type: text/html; charset=utf-8\r\n"
                   "Connection: close\r\n\r\n"
                   "<!DOCTYPE html><html><head><title>" + figure_name + 
                   "</title><style>pre{font-family:monospace;}</style></head>"
                   "<body><pre>" + figure + "</pre></body></html>";
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
    delete client; // Liberar memoria del objeto cliente
}

// ==============================================
// MAIN
// MODIFICADO: Lanza 6 hilos UDP
// ==============================================
int main() {
    std::signal(SIGINT, signal_handler);

    std::vector<std::string> broadcast_ips = {
        "172.16.123.31",
        "172.16.123.47",
        "172.16.123.63",
        "172.16.123.79",
        "172.16.123.111",
        "10.255.255.254"
    };

    // Hilos UDP para escuchar anuncios
    std::vector<std::thread> listener_threads;
    for (const auto& ip : broadcast_ips) {
        listener_threads.emplace_back(udp_listener, ip);
    }

    // Socket HTTP
    VSocket* http_sock = new Socket('s');
    if (http_sock->Bind(HTTP_PORT) < 0) {
        std::cerr << "Error al bindear puerto HTTP " << HTTP_PORT << std::endl;
        delete http_sock;
        running = false;
    } else {
        http_sock->MarkPassive(5);
        std::cout << "[HTTP Fork] Escuchando en puerto " << HTTP_PORT << std::endl;
    }

    // Escuchar conexiones entrantes HTTP
    while (running) {
        VSocket* client = http_sock->AcceptConnection();
        if (client) {
            std::thread(handle_http_request, client).detach();
        } else if (!running) {
            break;
        }
    }

    std::cout << "[HTTP Fork] Terminando..." << std::endl;
    running = false;

    for (auto& t : listener_threads) {
        if (t.joinable()) t.join();
    }

    delete http_sock;
    return 0;
}
