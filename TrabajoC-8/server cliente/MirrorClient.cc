/**
 *   UCR-ECCI
 *   CI-0123 Proyecto integrador de redes y sistemas operativos
 *
 *   Socket client/server example
 *
 *   Deben determinar la dirección IP del equipo donde van a correr el servidor
 *   para hacer la conexión en ese punto (ip addr)
 *
 **/

#include <stdio.h>
#include <cstring>
#include "Socket.h"

#define PORT 1231
#define BUFSIZE 512

int main( int argc, char ** argv ) {
   VSocket * s;
   char buffer[ BUFSIZE ];
   std::string figure_name = "gato"; // Default figure name
   

   s = new Socket( 's' );     // Creaite a new stream IPv4 socket
   memset( buffer, 0, BUFSIZE );	// Zero fill buffer

   s->MakeConnection( "127.0.0.1", PORT ); // Same port as server
   if ( argc > 1 ) {
      figure_name = argv[1];	// If provided, use first program argument as figure name
   } else {
      std::cout << "No figure name provided, using default: gato" << std::endl;
   }
   std::string request = "GET /figure?name=" + figure_name;
   s->Write(request.c_str(), request.size());	// Send string to server
 
   int bytesRead = 0;
   do {
      bytesRead = s->Read(buffer, BUFSIZE - 1);
      if (bytesRead > 0) {
         buffer[bytesRead] = '\0'; // Null-terminate the buffer
         printf("%s", buffer);    // Print the received chunk
      }
   } while (bytesRead > 0);
   std::cout<<std::endl;

}

