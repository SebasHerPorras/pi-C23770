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

#define PORT 1231
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
void task( VSocket * client ) {
   char request[BUFSIZE];

   client->Read(request, BUFSIZE); // Read a string from client, data will be limited by BUFSIZE bytes
   std::cout << "Server received: " << request << " from id: " << client->idSocket << std::endl;

   char figure_name[BUFSIZE];
   if (process_request(request, figure_name)) {
      std::string figure = fs.find_figura(figure_name);
      // Send the figure in chunks if it is larger than BUFSIZE
      size_t total_size = figure.size();
      size_t sent_size = 0;
      while (sent_size < total_size) {
         size_t chunk_size = std::min(static_cast<size_t>(BUFSIZE), total_size - sent_size);
         client->Write(figure.c_str() + sent_size, chunk_size);
         sent_size += chunk_size;
      }
   } else {
      std::cout << "Invalid request format" << std::endl;
      client->Write("Invalid request format", strlen("Invalid request format"));
   }
   client->Close(); // Close socket in parent
}

/**
 *   Process the request
 *      Check if the request is valid
 *      If valid, extract the figure name and return it
 *      If invalid, return an error message
 *
 **/
bool process_request( char* request, char* response ) {
   std::string req_str(request);
   std::string prefix = "GET /figure?name=";

   // Validate the request format
   if (req_str.rfind(prefix, 0) == 0) {
      std::string figure_name = req_str.substr(prefix.length());
      if (!figure_name.empty()) {
         // Copy the figure name to the response
         strncpy(response, figure_name.c_str(), BUFSIZE - 1);
         response[BUFSIZE - 1] = '\0'; // Ensure null termination
         return true;
      }
   }

   // If the format is invalid, return false
   strncpy(response, "Invalid request format", BUFSIZE - 1);
   response[BUFSIZE - 1] = '\0'; // Ensure null termination
   return false;
   
   return request;
}

/**
 *   Create server code
 *      Infinite for
 *         Wait for client conection
 *         Spawn a new thread to handle client request
 *
 **/
int main( int argc, char ** argv ) {
   std::thread * worker;
   VSocket * s1, * client;

   s1 = new Socket( 's' );

   s1->Bind( PORT );		// Port to access this mirror server
   s1->MarkPassive( 5 );	// Set socket passive and backlog queue to 5 connections
   std::cout << "\nServer started in port: " << PORT << std::endl;
   for( ; ; ) {
      client = s1->AcceptConnection();	 	// Wait for a client connection
      worker = new std::thread( task, client );
   }
   delete s1;		// Close socket in parent
   worker->join();	// Wait for thread to finish
   delete worker;	// Close thread
   std::cout << "Server finished" << std::endl;
   return 0;


}
