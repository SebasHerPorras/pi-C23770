
/**
 * Universidad de Costa Rica
 * ECCI
 * CI0123 Proyecto integrador de redes y sistemas operativos
 * 2025-i
 * Grupos: 1 y 3
 *
 * Socket client/server example with threads
 *
 * (Fedora version)
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
 #include <atomic>        // Para flags atómicos
 
 
 #include "Socket.h"
 #include "file_system.hpp"
 
 // Configuración de puertos
 #define DEFAULT_PORT      8081
 #define MULTICAST_PORT    5353
 #define BUFSIZE           512
 
 // Parámetros de multicast
 // MODIFICADO: Se elimina SERVER_NAME_FINAL para evitar inconsistencias.
 #define SERVER_NAME       "Isla5|127.0.0.1|gato,barco,sombrilla"
 #define SERVER_IP         "127.0.1" // IP de broadcast para pruebas locales 
 
 // Variables globales
 std::atomic<bool> running(true);  // Flag de control para los hilos
 std::mutex server_mutex;          // Mutex global para proteger el socket del server
 std::mutex udp_mutex;             // Mutex para proteger el socket UDP
 std::mutex cout_mutex;            // Mutex para proteger std::cout
 int global_port = DEFAULT_PORT;  // Puerto global, configurable
 
 FileSystem fs(true); // Inicializa y formatea el sistema de archivos
 VSocket *s1 = nullptr; // Socket principal del servidor
 VSocket *s2 = nullptr; // Socket de UDP para recibir solicitudes
 
 // Prototipos
 void send_initial_announcement(VSocket* s2);
 void udp_listener(const std::string& listen_ip, VSocket* s2);
 void task(VSocket * client);
 bool process_request(char* request, char* response);
 void signal_handler(int signal);
 void atomic_cout(const std::string& message);
 
 // ==============================================
 // SALIDA ATÓMICA
 // ==============================================
 void atomic_cout(const std::string& message) {
      std::lock_guard<std::mutex> guard(cout_mutex);
      std::cout << message << std::flush;
 }
 
 // ==============================================
 // MANEJADOR DE SEÑALES (Ctrl+C)
 // ==============================================
 void signal_handler(int signal) {
      if (signal == SIGINT) {
          atomic_cout("\n[Server] Recibida señal SIGINT (Ctrl+C), cerrando servidor...\n");
          running = false;
          sleep(2); // Esperar un segundo para permitir que los hilos terminen
          delete s1;
          exit(0); // Salir limpiamente
      }
}
 
 // ==============================================
 // ENVÍO INICIAL DE ANUNCIOS
 // ==============================================
 void send_initial_announcement(VSocket* s2) {
    atomic_cout("[Server] Enviando anuncio inicial...\n");

    std::vector<std::string> broadcast_ips = {
        "172.16.123.31",
        "172.16.123.47",
        "172.16.123.63",
        "172.16.123.79",
        "172.16.123.111",
        "10.255.255.254"  // Localhost para pruebas broadcast
    };

    struct sockaddr_in bcastAddr;
    memset(&bcastAddr, 0, sizeof(bcastAddr));
    bcastAddr.sin_family = AF_INET;
    bcastAddr.sin_port = htons(MULTICAST_PORT);

    std::string msg = std::string(SERVER_NAME);

    for (const auto& ip : broadcast_ips) {
        bcastAddr.sin_addr.s_addr = inet_addr(ip.c_str());

        try {
            s2->sendTo(msg.c_str(), msg.size(), &bcastAddr);
            atomic_cout("Anuncio enviado a: " + ip + "\n");
        } catch (const std::exception& e) {
            atomic_cout("Error enviando anuncio a " + ip + ": " + e.what() + "\n");
        }
    }
}

 
 // ==============================================
 // LISTENER UDP - Espera solicitudes "get servers"
 // ==============================================
 extern std::mutex udp_mutex;  // mutex global definido afuera

 void udp_listener(const std::string& listen_ip, VSocket* s2) {
    while (running) {
        char buffer[BUFSIZE];
        struct sockaddr_in srcAddr;
        socklen_t addrLen = sizeof(srcAddr);
        memset(&srcAddr, 0, sizeof(srcAddr));

        size_t bytes;
        {
            std::lock_guard<std::mutex> lock(udp_mutex);
            try {
                bytes = s2->recvFrom(buffer, BUFSIZE - 1, &srcAddr);
            } catch (const std::exception& e) {
                atomic_cout("Error en recvFrom: " + std::string(e.what()) + "\n");
                continue;
            }
        }

        if (bytes > 0) {
            buffer[bytes] = '\0';
            std::string receivedMsg(buffer);
            
            char ipStr[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(srcAddr.sin_addr), ipStr, INET_ADDRSTRLEN);
            atomic_cout("Mensaje UDP recibido de " + std::string(ipStr) + ": " + receivedMsg + "\n");

            if (receivedMsg.find("GET /servers") == 0) {
                atomic_cout("Solicitud de servidores recibida de " + std::string(ipStr) + "\n");
                
                std::string response = SERVER_NAME;
                
                {
                    std::lock_guard<std::mutex> lock(udp_mutex);
                    try {
                        // Send response back to the requester's address and port
                        s2->sendTo(response.c_str(), response.size(), &srcAddr);
                        atomic_cout("Respuesta enviada a " + std::string(ipStr) + ": " + response + "\n");
                    } catch (const std::exception& e) {
                        atomic_cout("Error en sendTo: " + std::string(e.what()) + "\n");
                    }
                }
            }
        }
    }
}
 
 
 // ==============================================
 // TASK - Manejo de conexiones de clientes (TCP)
 // ==============================================
 void task(VSocket* client) {
     char request[BUFSIZE];
     client->Read(request, BUFSIZE);
 
     if (strstr(request, "favicon.ico") != nullptr) {
         client->Close();
         return;
     }
 
     char figure_name[BUFSIZE];
     if (process_request(request, figure_name)) {
         atomic_cout("\nFigura solicitada: " + std::string(figure_name) + "\n");
 
         std::string figure = fs.find_figura(figure_name);
         
         // MODIFICADO: Se elimina la lógica de respuesta HTTP compleja.
         // El servidor ahora SIEMPRE responde con los datos crudos de la figura.
         // El 'fork' (intermediario) es el responsable de crear la respuesta HTML para el navegador.
         // Esto soluciona el problema del HTML anidado y define responsabilidades claras.
         
         if (!figure.empty()) {
             client->Write(figure.c_str(), figure.size());
             atomic_cout("Figura '" + std::string(figure_name) + "' enviada.\n\n");
         } else {
             // Si la figura no se encuentra, se puede enviar un mensaje de error simple.
             std::string error_msg = "Error: Figura no encontrada.\n";
             client->Write(error_msg.c_str(), error_msg.size());
             atomic_cout("Error: Figura '" + std::string(figure_name) + "' no encontrada.\n\n");
         }

     } else {
         atomic_cout("Error: Solicitud inválida.\n");
         std::string error_msg = "Error: Solicitud con formato invalido.\n";
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
 // MAIN
 // ==============================================
 int main(int argc, char **argv) {
     // Configurar manejador de señales
     std::signal(SIGINT, signal_handler);
 
     // Configurar puerto si se especifica
     if (argc > 1) {
         try {
             global_port = std::stoi(argv[1]);
             atomic_cout("Usando puerto configurado: " + std::to_string(global_port) + "\n");
         } catch (...) {
             atomic_cout("Puerto inválido. Usando puerto por defecto: " + std::to_string(DEFAULT_PORT) + "\n");
             global_port = DEFAULT_PORT;
         }
     }
 
     // Enviar anuncios iniciales
     s2 = new Socket('d'); // socket datagram para UDP
     int st = s2->Bind(MULTICAST_PORT);
     if (st < 0) {
         atomic_cout("Error al bindear el socket UDP: " + std::string(strerror(errno)) + "\n");
         delete s2;
         return 1;
     }
     atomic_cout("Socket UDP creado y bindado al puerto: " + std::to_string(MULTICAST_PORT) + "\n");
     send_initial_announcement(s2);
 
     // Iniciar listeners UDP para cada IP
     std::vector<std::string> listen_ips = {
         "172.16.123.31",
         "172.16.123.47",
         "172.16.123.63",
         "172.16.123.79",
         "172.16.123.111",
         "10.255.255.254"
     };
     std::vector<std::thread> udp_threads;
     for (const auto& ip : listen_ips) {
         udp_threads.emplace_back(udp_listener, ip, s2);
     }
 
     // Inicializar socket principal TCP
     s1 = new Socket('s');
     if (s1->Bind(global_port) < 0) {
         atomic_cout("Error al bindear el puerto " + std::to_string(global_port) + ": " + strerror(errno) + "\n");
         delete s1;
         return 1;
     }
     s1->MarkPassive(5);
     atomic_cout("Servidor TCP iniciado en puerto: " + std::to_string(global_port) + "\n");
 
     // Bucle principal de conexiones TCP
     atomic_cout("Servidor listo para aceptar conexiones TCP...\n");
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
     atomic_cout("Cerrando servidor...\n");
     
     // Esperar a que terminen los hilos UDP
     for (auto& t : udp_threads) {
         if (t.joinable()) t.join();
     }
     
     delete s1;
     delete s2; // Asegurarse de limpiar el socket UDP también
     atomic_cout("Servidor detenido correctamente.\n");
     return 0;
 }