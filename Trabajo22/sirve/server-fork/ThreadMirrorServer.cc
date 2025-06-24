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
#include <algorithm> // For std::min and std::max

#include "Socket.h"
#include "file_system.hpp"

#define PORT 1234
#define BUFSIZE 512

void task( VSocket * client );
bool process_request( char* request, char* response );


FileSystem fs(true); // Inicializa el sistema de archivos y lo formatea
/**
 *   Task each new thread will run
 *      Read string from socket
 *      Write it back to client
 *
 **/
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



/**
 *   Create server code
 *      Infinite for
 *         Wait for client conection
 *         Spawn a new thread to handle client request
 *
 **/
int main(int argc, char **argv) {
   std::thread *worker;
   VSocket *s1, *client;

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

   s1->Bind(port);        // Port to access this mirror server
   s1->MarkPassive(5);    // Set socket passive and backlog queue to 5 connections
   std::cout << "\nServer started on port: " << port << std::endl;
   for (;;) {
      client = s1->AcceptConnection();       // Wait for a client connection
      worker = new std::thread(task, client);
   }
   delete s1;        // Close socket in parent
   worker->join();   // Wait for thread to finish
   delete worker;    // Close thread
   std::cout << "Server finished" << std::endl;
   return 0;
}
