/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2025-i
  *  Grupos: 1 y 3
  *
  ****** VSocket base class implementation
  *
  * (Fedora version)
  *
 **/

#include <sys/socket.h>
#include <arpa/inet.h>		// ntohs, htons
#include <stdexcept>            // runtime_error
#include <cstring>		// memset
#include <netdb.h>			// getaddrinfo, freeaddrinfo
#include <unistd.h>			// close
/*
#include <cstddef>
#include <cstdio>

//#include <sys/types.h>
*/
#include "VSocket.h"


/**
  *  Class creator (constructor)
  *     use Unix socket system call
  *
  *  @param     char t: socket type to define
  *     's' for stream
  *     'd' for datagram
  *  @param     bool ipv6: if we need a IPv6 socket
  *
 **/
void VSocket::BuildSocket( char t, bool IPv6 ) {
   if ( t == 'd' ) {
      idSocket = socket(IPv6 ? AF_INET6 : AF_INET, SOCK_DGRAM,0);
      if (idSocket < 0) {
         throw std::runtime_error( "VSocket::BuildSocket, (reason)" );
      } else {
         printf("Socket creado con éxito\n");
      }
   } else {
      throw std::runtime_error( "VSocket::BuildSocket, (reason)" );
   }
}


/**
  * Class destructor
  *
 **/
VSocket::~VSocket() {

   this->Close();

}


/**
  * Close method
  *    use Unix close system call (once opened a socket is managed like a file in Unix)
  *
 **/
void VSocket::Close(){
   if (close(idSocket) < 0) {
      throw std::runtime_error( "VSocket::Close() - Error al cerrar el Socket" );
   }
}


/**
  * EstablishConnection method
  *   use "connect" Unix system call
  *
  * @param      char * host: host address in dot notation, example "10.84.166.62"
  * @param      int port: process address, example 80
  *
 **/
int VSocket::EstablishConnection( const char * hostip, int port ) {

   int st = -1;

   if ( -1 == st ) {
      throw std::runtime_error( "VSocket::EstablishConnection" );
   }

   return st;

}


/**
  * EstablishConnection method
  *   use "connect" Unix system call
  *
  * @param      char * host: host address in dns notation, example "os.ecci.ucr.ac.cr"
  * @param      char * service: process address, example "http"
  *
 **/
int VSocket::EstablishConnection( const char *host, const char *service ) {
   int st = -1;

   throw std::runtime_error( "VSocket::EstablishConnection" );

   return st;

}


/**
  * Bind method
  *    use "bind" Unix system call (man 3 bind) (server mode)
  *
  * @param      int port: bind a unamed socket to a port defined in sockaddr structure
  *
  *  Links the calling process to a service at port
  *
 **/
int VSocket::Bind( int port ) {
   int opt = 1;
   if (setsockopt(idSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
      throw std::runtime_error("VSocket::Bind - Error al configurar SO_REUSEADDR");
  }
   struct sockaddr_in host4;
   memset(&host4, 0, sizeof(host4));
   host4.sin_family = AF_INET;
   host4.sin_addr.s_addr = htonl( INADDR_ANY );
   host4.sin_port = htons( port );

   if (bind(idSocket, (struct sockaddr*) &host4, sizeof(host4)) < 0) {
      throw std::runtime_error( "VSocket::Bind - Se cae en el bind" );
   } else {
      printf("VSocket::Bind - Bind exitoso\n");
   }
   return 0;
}


/**
  *  sendTo method
  *
  *  @param	const void * buffer: data to send
  *  @param	size_t size data size to send
  *  @param	void * addr address to send data
  *
  *  Send data to another network point (addr) without connection (Datagram)
  *
 **/
size_t VSocket::sendTo( const void * buffer, size_t size, void * addr ) {
   struct sockaddr_in* dest_address = (struct sockaddr_in*) addr;
   ssize_t bytes_sent = sendto(idSocket, buffer, size, 0, (struct sockaddr*)dest_address, sizeof(*dest_address));
   if (bytes_sent < 0) {
      throw std::runtime_error( "VSocket::sendTo - Error al enviar la info" );
   }
   return bytes_sent;
}


/**
  *  recvFrom method
  *
  *  @param	const void * buffer: data to send
  *  @param	size_t size data size to send
  *  @param	void * addr address to receive from data
  *
  *  @return	size_t bytes received
  *
  *  Receive data from another network point (addr) without connection (Datagram)
  *
 **/
size_t VSocket::recvFrom( void * buffer, size_t size, void * addr ) {
   struct sockaddr_in* source = (struct sockaddr_in*)addr;
   socklen_t addr_len = sizeof(*source);
   ssize_t bytes_recieved = recvfrom(idSocket, buffer, size, 0, (struct sockaddr*)source, &addr_len);
   if (bytes_recieved < 0) {
      throw std::runtime_error( "VSocket::recvFrom - Error al recibir la info" );
   }
   return bytes_recieved;

}

