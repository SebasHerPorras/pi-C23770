/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2025-i
  *  Grupos: 1 y 3
  *
  *  ******   Socket class implementation
  *
  * (Fedora version)
  *
 **/

#include <sys/socket.h>         // sockaddr_in
#include <arpa/inet.h>          // ntohs
#include <unistd.h>		// write, read
#include <cstring>
#include <stdexcept>
#include <stdio.h>		// printf

#include "Socket.h"		// Derived class

/**
  *  Class constructor
  *     use Unix socket system call
  *
  *  @param     char t: socket type to define
  *     's' for stream
  *     'd' for datagram
  *  @param     bool ipv6: if we need a IPv6 socket
  *
 **/
Socket::Socket( char t, bool IPv6 ){

   this->BuildSocket( t, IPv6 );      // Call base class constructor

}


/**
  *  Class destructor
  *
  *  @param     int id: socket descriptor
  *
 **/
Socket::~Socket() {

}


/**
  * MakeConnection method
  *   use "EstablishConnection" in base class
  *
  * @param      char * host: host address in dot notation, example "10.1.166.62"
  * @param      int port: process address, example 80
  *
 **/
int Socket::MakeConnection( const char * hostip, int port ) {

   return this->EstablishConnection( hostip, port );

}


/**
  * MakeConnection method
  *   use "EstablishConnection" in base class
  *
  * @param      char * host: host address in dns notation, example "os.ecci.ucr.ac.cr"
  * @param      char * service: process address, example "http"
  *
 **/
int Socket::MakeConnection( const char *host, const char *service ) {

   return this->EstablishConnection( host, service );

}


/**
  * Read method
  *   use "read" Unix system call (man 3 read)
  *
  * @param      void * buffer: buffer to store data read from socket
  * @param      int size: buffer capacity, read will stop if buffer is full
  *
 **/
size_t Socket::Read(void *buffer, size_t size) {
    if (idSocket == -1) {
        throw std::runtime_error("Socket::Read: Socket no inicializado");
    }

    
    ssize_t st = read(idSocket, buffer, size);
    if (st == -1) {
      // Usar perror para imprimir el error
      perror("Socket::Read: Error al leer del socket");
      // O usar strerror para obtener el mensaje del error con errno
      std::cerr << "Detalles del error: " << strerror(errno) << std::endl;
      throw std::runtime_error("Socket::Read: Error al leer del socket");
    }

    return static_cast<size_t>(st);
}

/**
  * Write method
  *   use "write" Unix system call (man 3 write)
  *
  * @param      void * buffer: buffer to store data write to socket
  * @param      size_t size: buffer capacity, number of bytes to write
  *
 **/
size_t Socket::Write(const void *buffer, size_t size) {
    if (idSocket == -1) {
        throw std::runtime_error("Socket::Write: Socket no inicializado");
    }

    ssize_t st = write(idSocket, buffer, size);
    if (st == -1) {
        throw std::runtime_error("Socket::Write: Error al escribir en el socket");
    }
    return static_cast<size_t>(st);
}



/**
  * Write method
  *   use "write" Unix system call (man 3 write)
  *
  * @param      char * text: text to write to socket
  *
 **/
size_t Socket::Write(const char* text) {
    if (idSocket == -1) {
        throw std::runtime_error("Socket::Write: Socket no inicializado");
    }

    ssize_t st = write(idSocket, text, strlen(text));
    if (st == -1) {
        throw std::runtime_error("Socket::Write: Error al escribir en el socket");
    }

    return static_cast<size_t>(st);
}



/**
  * AcceptiConnection method
  *    use base class to accept connections
  *
  *  @returns   a new class instance
  *
  *  Waits for a new connection to service (TCP mode: stream)
  *
 **/
VSocket * Socket::AcceptConnection(){
   int id;
   VSocket * peer;

   // std::cout << "Socket::AcceptConnection antes del wait" << std::endl;
   id = this->WaitForConnection();
   //std::cout << "Socket::AcceptConnection despues del wait" << std::endl;

   peer = new Socket( 's' );
   peer->idSocket = id;
   return peer;

}


