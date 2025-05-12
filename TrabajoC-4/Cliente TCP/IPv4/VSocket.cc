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
#include <ostream>
#include <iostream>
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
   this->IPv6 = IPv6;
   if (IPv6)
   {
      printf(("Se está utilizando IPv6\n"));
   }
   
   this->type = t;
   if (this->type == 's') {
      idSocket = socket( IPv6 ? AF_INET6 : AF_INET, SOCK_STREAM,0);
   } else if (this->type == 'd') {
      idSocket = socket(IPv6 ? AF_INET6 : AF_INET, SOCK_DGRAM,0);
   } else{
      throw std::runtime_error( "Se está ingresando un parámetro incorrecto" );
   }
   if (idSocket == -1) {
      throw std::runtime_error( "VSocket::BuildSocket, (reason)" );
   }
   printf("Socket creado con éxito (ID: %d)\n", idSocket);
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
   if(idSocket != -1) {
      if(close(idSocket) == -1) {
         throw std::runtime_error( "VSocket::Close()" );
      }
      idSocket = -1;
      printf("Socket cerrado con éxito.\n");
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
   struct sockaddr_in addr_serv;
   memset(&addr_serv,0,sizeof(addr_serv));
   addr_serv.sin_family = AF_INET;
   addr_serv.sin_port = htons(port);
   // Verifica la dirección IP
   if (inet_pton(AF_INET,hostip,&addr_serv.sin_addr) <= 0) {
      throw std::runtime_error( "VSocket::EstablishConnection - Dirección IP inválida" );
   }
   // Realiza la conexión y verifica el error
   if (connect(idSocket,(struct sockaddr *)&addr_serv,sizeof(addr_serv)) < 0) {
      throw std::runtime_error( "VSocket::EstablishConnection" );
   }
   // Para verificar la dirección y el puerto al que se conectó
   printf("Conexión establecida con éxito a %s y %d\n", hostip,port);

   return 0;

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
   struct addrinfo hints, *res, *p;
   int status = 0;
   memset(&hints,0,sizeof(hints));
   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = (this->type == 's') ? SOCK_STREAM : SOCK_DGRAM;
   if ((status = getaddrinfo(host,service,&hints,&res)) != 0) {
      throw std::runtime_error( "VSocket::EstablishConnection" );
   }
   for (p = res; p != NULL; p = p->ai_next) {
      if (connect(idSocket,p->ai_addr,p->ai_addrlen) == -1) {
         continue;
      }
      break;
   }
   if (p == NULL) {
      freeaddrinfo(res);
      throw std::runtime_error( "VSocket::EstablishConnection - Connection Failed" );
   }
   freeaddrinfo(res);
   printf("Conexión establecida con éxito a %s:%s\n", host, service);
   return 0;

}

