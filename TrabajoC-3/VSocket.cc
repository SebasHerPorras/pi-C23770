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
#include <iostream>            // cout
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
void VSocket::BuildSocket( char t, bool IPv6 ){

    this->IPv6 = IPv6;
    this->type = t;
   
    int st;
    int domain = IPv6 ? AF_INET6 : AF_INET;  // IPv4 o IPv6
    int sockType;

    if (t == 's') {
        sockType = SOCK_STREAM;  // stream (TCP)
    } else if (t == 'd') {
        sockType = SOCK_DGRAM;  // DGram (UDP)
    } else {
        throw std::invalid_argument("VSocket::BuildSocket: Tipo de socket inválido. Use 's' para STREAM (TCP) o 'd' para DATAGRAM (UDP)");
    }

    int protocol = 0;  // 0 para protocolo por defecto

    st = socket(domain, sockType, protocol);
    if (st == -1) {
        throw std::runtime_error("VSocket::BuildSocket: error al crear el socket");
    }
    
    this->idSocket = st;
    std::cout << "\nidsocket " << st  << "  " << IPv6 << std::endl;
}

void VSocket::CreateVSocket(int id) {
    this->idSocket = id;
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
   int st = -1;

   if ( -1 == st ) {
      throw std::runtime_error( "VSocket::Close()" );
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
   int st = -1;

   return st;

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
   int st = -1;

   return st;

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
   int st = -1;

   return st;

}


int VSocket::DoConnect( const char * hostip, int port ) {
   int st = -1;
   struct sockaddr_in host4;
   memset( (char *) &host4, 0, sizeof( host4 ) );
   host4.sin_family = AF_INET;
   st = inet_pton( AF_INET, hostip, &host4.sin_addr );
   if ( -1 == st ) {
      throw( std::runtime_error( "VSocket::DoConnect, inet_pton" ));
   }

   host4.sin_port = htons( port );
   st = connect( idSocket, (sockaddr *) &host4, sizeof( host4 ) );

   if ( -1 == st ) {
      perror( "VSocket::connect" );
      throw std::runtime_error( "VSocket::DoConnect" );
   }

   return st;

}

int VSocket::DoConnect( const char * host, const char * service ) {
   
  struct addrinfo hints, *res, *p;
   int st = 0;
   memset(&hints,0,sizeof(hints));
   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = (this->type == 's') ? SOCK_STREAM : SOCK_DGRAM;
   if ((st = getaddrinfo(host,service,&hints,&res)) != 0) {
      throw std::runtime_error( "VSocket::EstablishConnection" );
   }
   for (p = res; p != NULL; p = p->ai_next) {
      if (connect(this->idSocket,p->ai_addr,p->ai_addrlen) == -1) {
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
