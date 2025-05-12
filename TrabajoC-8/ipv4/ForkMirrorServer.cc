/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2025-i
  *  Grupos: 1 y 3
  *
  *   Socket client/server example
  *
  * (Fedora version)
  *
 **/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>	// memset
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>  // para waitpid en handler de SIGCHLD


#include "Socket.h"

#define PORT 1234
#define BUFSIZE 512


VSocket * s1 = nullptr;

void sigint_handler(int signum) {
    std::cout << "\n[INFO] Señal SIGINT capturada. Cerrando socket y saliendo...\n";
    if (s1) {
        s1->Close();  // cerrar el socket
        delete s1;
    }
    exit(0);
}

void sigchld_handler(int signum) {
    // Evita procesos zombis
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main( int argc, char ** argv ) {
   VSocket  * s2;
   int childpid;
   char a[ BUFSIZE ];

   signal(SIGINT, sigint_handler);     // <-- ESTO FALTABA
   signal(SIGCHLD, sigchld_handler);   // <-- ESTO TAMBIÉN

   s1 = new Socket( 's' );		// Create a stream IPv4 socket
   s1->Bind( PORT );			// Port to access this mirror server
   s1->MarkPassive( 5 );		// Set passive socket and backlog queue to 5 connections

   std::cout << "\nFork started port: " << PORT << std::endl;
   for( ; ; ) {
      s2 = s1->AcceptConnection();	// Wait for a new connection, connection info is in s2 variable
      childpid = fork();		// Create a child to serve the request
      if ( childpid < 0 ) {
         perror( "server: fork error" );
      } else {
         if (0 == childpid) {		// child code
            s1->Close();			// Close original socket "s1" in child
            memset( a, 0, BUFSIZE );
            s2->Read( a, BUFSIZE );	// Read a string from client using new conection info
            std::cout << "Server received: " << a << " from id: " << s2->idSocket << std::endl;
            s2->Write( a );		// Write it back to client, this is the mirror function
            exit( 0 );			// Exit, finish child work
         }
      }

      s2->Close();			// Close socket s2 in parent, then go wait for a new conection
   }

   delete s1;
   delete s2;
   return 0;
}


