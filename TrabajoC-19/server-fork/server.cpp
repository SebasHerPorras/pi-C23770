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
 
 #include "Socket.h"
 #include "file_system.hpp"
 
 #define PORT             1234
 #define BUFSIZE          512
 
 // Parámetros de multicast
 #define MULTICAST_GROUP  "239.0.0.1"
 #define MULTICAST_PORT   5353
 
 // Para identificar el servidor al anunciar y al shutdown
 #define SERVER_NAME      "ServidorA"
 
 void task(VSocket * client);
 bool process_request(char* request, char* response);
 
 FileSystem fs(true); // Inicializa y formatea el sistema de archivos
 
 int main(int argc, char **argv) {
     // -----------------------------
     // 1) Configuración Multicast UDP
     // -----------------------------
     int udpSock = socket(AF_INET, SOCK_DGRAM, 0);
     if (udpSock < 0) {
         std::cerr << "Error creando udpSock: " << strerror(errno) << "\n";
         return 1;
     }
 
     struct sockaddr_in mcastAddr;
     memset(&mcastAddr, 0, sizeof(mcastAddr));
     mcastAddr.sin_family      = AF_INET;
     mcastAddr.sin_addr.s_addr = inet_addr(MULTICAST_GROUP);
     mcastAddr.sin_port        = htons(MULTICAST_PORT);
 
     // Socket para escuchar Shutdown
     int udpListen = socket(AF_INET, SOCK_DGRAM, 0);
     if (udpListen < 0) {
         std::cerr << "Error creando udpListen: " << strerror(errno) << "\n";
         close(udpSock);
         return 1;
     }
     // Reusar dirección
     int reuse = 1;
     setsockopt(udpListen, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
 
     struct sockaddr_in localAddr;
     memset(&localAddr, 0, sizeof(localAddr));
     localAddr.sin_family      = AF_INET;
     localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
     localAddr.sin_port        = htons(MULTICAST_PORT);
 
     if (bind(udpListen, (struct sockaddr*)&localAddr, sizeof(localAddr)) < 0) {
         std::cerr << "Error bind udpListen: " << strerror(errno) << "\n";
         close(udpSock); close(udpListen);
         return 1;
     }
 
     struct ip_mreq mreq;
     mreq.imr_multiaddr.s_addr = inet_addr(MULTICAST_GROUP);
     mreq.imr_interface.s_addr = htonl(INADDR_ANY);
     if (setsockopt(udpListen, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
         std::cerr << "Error unirse a multicast: " << strerror(errno) << "\n";
         close(udpSock); close(udpListen);
         return 1;
     }
 
     // -----------------------------
     // 2) Iniciar servidor TCP original
     // -----------------------------
     std::thread *worker;
     VSocket *s1, *client;
 
     int port = PORT;
     if (argc > 1) {
         try {
             port = std::stoi(argv[1]);
         } catch (...) {
             std::cerr << "Invalid port argument. Using default port: " << PORT << std::endl;
             port = PORT;
         }
     }
 
     s1 = new Socket('s');
     s1->Bind(port);
     s1->MarkPassive(5);
     std::cout << "Server started on port: " << port << std::endl;
 
     // -----------------------------
     // 3) Hilo que envía anuncios periódicos
     // -----------------------------
     bool running = true;
     std::thread announcer([&]() {
         while (running) {
             // Ejemplo: "ANUNCIO_SERVIDOR:ServidorA:127.0.0.1:1234"
             std::string msg = std::string("ANUNCIO_SERVIDOR:") + SERVER_NAME
                             + ":" + "127.0.0.1" + ":" + std::to_string(port);
             sendto(udpSock, msg.c_str(), msg.size(), 0,
                    (struct sockaddr*)&mcastAddr, sizeof(mcastAddr));
             std::this_thread::sleep_for(std::chrono::seconds(5));
         }
     });
 
     // -----------------------------
     // 4) Hilo que escucha Shutdown
     // -----------------------------
     std::thread listener([&]() {
         char buf[BUFSIZE];
         while (running) {
             memset(buf, 0, BUFSIZE);
             int n = recvfrom(udpListen, buf, BUFSIZE, 0, nullptr, nullptr);
             if (n > 0) {
                 std::string recibido(buf);
                 if (recibido.rfind("Shutdown " SERVER_NAME, 0) == 0) {
                     std::cout << "Recibido pedido de Shutdown. Muerte inminente...\n";
                     // Notificar a tenedores
                     std::string morirMsg = std::string("MORIRE:") + SERVER_NAME;
                     sendto(udpSock, morirMsg.c_str(), morirMsg.size(), 0,
                            (struct sockaddr*)&mcastAddr, sizeof(mcastAddr));
                     running = false;
                     break;
                 }
             }
         }
     });
 
     // -----------------------------
     // 5) Bucle principal TCP (sin cambios)
     // -----------------------------
     for (;;) {
         if (!running) break;
         client = s1->AcceptConnection();
         worker = new std::thread(task, client);
         // No hacemos join aquí para seguir aceptando
     }
 
     // -----------------------------
     // 6) Limpieza
     // -----------------------------
     announcer.join();
     listener.join();
     close(udpSock);
     close(udpListen);
 
     delete s1;
     std::cout << "Server finished" << std::endl;
     return 0;
 }
 
 void task(VSocket * client) {
  char request[BUFSIZE];
  client->Read(request, BUFSIZE);

  std::cout << "Server received: " << request << " from id: " << client->idSocket << std::endl;

  char figure_name[BUFSIZE];
  if (process_request(request, figure_name)) {
      std::cout << "\n\nRequested figure: " << figure_name << "\n" << std::endl;
      std::cout <<std::endl;

      std::string figure = fs.find_figura(figure_name);
     std::string http_response = 
       "HTTP/1.1 200 OK\r\n"
       "Content-Type: text/html; charset=UTF-8\r\n"
       "Content-Length: " + std::to_string(figure.size()) + "\r\n"
       "\r\n" +
       "<html><body>" +
       "<pre>" + "\n" + figure + "</pre>" +  // Aseguramos que los saltos de línea y formato se respeten
       "</body></html>";

      client->Write(http_response.c_str(), http_response.size());
  } else {
      std::string error_msg = "Invalid request format";
      std::string http_response = 
          "HTTP/1.1 400 Bad Request\r\n"
         "Content-Type: text/html; charset=UTF-8\r\n"
          "Content-Length: " + std::to_string(error_msg.size()) + "\r\n"
          "\r\n" +
          error_msg;

      client->Write(http_response.c_str(), http_response.size());
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
 std::string prefix = "GET /figure?name=";

 size_t pos_start = req_str.find(prefix);
 if (pos_start != std::string::npos) {
    // Buscamos el final de la línea (antes de espacio o \r o \n)
    size_t pos_name_start = pos_start + prefix.length();
    size_t pos_end_space = req_str.find(' ', pos_name_start);
    size_t pos_end_r = req_str.find('\r', pos_name_start);
    size_t pos_end_n = req_str.find('\n', pos_name_start);

    // Tomamos el mínimo de las posiciones válidas
    size_t pos_end = std::min({pos_end_space, pos_end_r, pos_end_n, req_str.size()});

    std::string figure_name = req_str.substr(pos_name_start, pos_end - pos_name_start);

    // Eliminamos posibles caracteres indeseados (solo letras y números permitidos)
    figure_name.erase(std::remove_if(figure_name.begin(), figure_name.end(),
                                     [](char c) { return !std::isalnum(c) && c != '_' && c != '-'; }),
                      figure_name.end());

    if (!figure_name.empty()) {
       strncpy(response, figure_name.c_str(), BUFSIZE - 1);
       response[BUFSIZE - 1] = '\0';
       return true;
    }
 }

 strncpy(response, "Invalid request format", BUFSIZE - 1);
 response[BUFSIZE - 1] = '\0';
 return false;
} 