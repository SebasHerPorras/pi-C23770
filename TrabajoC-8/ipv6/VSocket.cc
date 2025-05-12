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
#include <netdb.h>		// getaddrinfo, freeaddrinfo
#include <unistd.h>		// close
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
       std::cout << "estás usando: '" << t <<"'" << std::endl;
       throw std::invalid_argument("VSocket::BuildSocket: Tipo de socket inválido. Use 's' para STREAM (TCP) o 'd' para DATAGRAM (UDP)");
   }

   int protocol = 0;  // 0 para protocolo por defecto

   st = socket(domain, sockType, protocol);
   if (st == -1) {
       throw std::runtime_error("VSocket::BuildSocket: error al crear el socket");
   }
   
   this->idSocket = st;
   // std::cout << "\nidsocket " << st  << "  " << IPv6 << std::endl;
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
   if (idSocket != -1) {
      close(idSocket);  // Cierra el socket si está abierto
      idSocket = -1;    // Asegura que no se intente cerrar nuevamente el socket
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
int VSocket::EstablishConnection(const char * hostip, int port) {
    int st = -1;

    if (! this->IPv6) {
        // ——— IPv4 ———
        struct sockaddr_in host4;
        memset(&host4, 0, sizeof(host4));
        host4.sin_family = AF_INET;
        host4.sin_port   = htons(port);

        st = inet_pton(AF_INET, hostip, &host4.sin_addr);
        if (st <= 0) {
            throw std::runtime_error("VSocket::ESTABLISHCONNECTION IPv4, inet_pton failed");
        }

        st = connect(idSocket, (struct sockaddr*)&host4, sizeof(host4));
        if (st == -1) {
            perror("VSocket::connect IPv4");
            std::cout << "port: " << port << "  idSocket: " << idSocket << std::endl;
            throw std::runtime_error("VSocket::CONNECT IPv4 failed");
        }
    }
    else {
        // ——— IPv6 ———
        struct sockaddr_in6 host6;
        memset(&host6, 0, sizeof(host6));
        host6.sin6_family = AF_INET6;
        host6.sin6_port   = htons(port);

        st = inet_pton(AF_INET6, hostip, &host6.sin6_addr);
        if (st <= 0) {
            throw std::runtime_error("VSocket::ESTABLISHCONNECTION IPv6, inet_pton failed");
        }

        st = connect(idSocket, (struct sockaddr*)&host6, sizeof(host6));
        if (st == -1) {
            perror("VSocket::connect IPv6");
            std::cout << "port: " << port << "  idSocket: " << idSocket << std::endl;
            throw std::runtime_error("VSocket::CONNECT IPv6 failed");
        }
    }

    return st;
}


int VSocket::EstablishConnection( const char * host, const char * service ) {
   
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


/**
  * Bind method
  *    use "bind" Unix system call (man 3 bind) (server mode)
  *
  * @param      int port: bind a unamed socket to a port defined in sockaddr structure
  *
  *  Links the calling process to a service at port
  *
 **/
int VSocket::Bind(int port) {
    int st = -1;
    if(!IPv6) {
        struct sockaddr_in addr4{};
        addr4.sin_family = AF_INET;
        addr4.sin_addr.s_addr = INADDR_ANY;
        addr4.sin_port = htons(port);
        st = ::bind(idSocket, (sockaddr*)&addr4, sizeof(addr4));
    } else {
        struct sockaddr_in6 addr6{};
        addr6.sin6_family = AF_INET6;
        addr6.sin6_addr = in6addr_any;            // acepta en todas las interfaces
        addr6.sin6_port = htons(port);
        st = ::bind(idSocket, (sockaddr*)&addr6, sizeof(addr6));
    }
    return st;
}


/**
  * MarkPassive method
  *    use "listen" Unix system call (man listen) (server mode)
  *
  * @param      int backlog: defines the maximum length to which the queue of pending connections for this socket may grow
  *
  *  Establish socket queue length
  *
 **/
int VSocket::MarkPassive(int backlog) {
    int st = listen(idSocket, backlog);

    if (st == -1) {
        perror("VSocket::listen");
        std::cout << "idSocket: " << idSocket << "  backlog: " << backlog << std::endl;
        throw std::runtime_error("VSocket::MARKPASSIVE failed");
    }

    return st;
}



/**
  * WaitForConnection method
  *    use "accept" Unix system call (man 3 accept) (server mode)
  *
  *
  *  Waits for a peer connections, return a sockfd of the connecting peer
  *
 **/
int VSocket::WaitForConnection( void ) {
    int clientSocket = -1;
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);

    clientSocket = accept(idSocket, (struct sockaddr*)&clientAddr, &clientLen);

    if (clientSocket == -1) {
        perror("VSocket::accept");
        throw std::runtime_error("VSocket::WAITFORCONNECTION failed");
    }
    std::cout << "VSocket::WaitForConnection: client socket: " << clientSocket << std::endl;
    return clientSocket;
}



/**
  * Shutdown method
  *    use "shutdown" Unix system call (man 3 shutdown) (server mode)
  *
  *
  *  cause all or part of a full-duplex connection on the socket associated with the file descriptor socket to be shut down
  *
 **/
int VSocket::Shutdown( int mode ) {
   int st = -1;

   throw std::runtime_error( "VSocket::Shutdown" );

   return st;

}


// UDP methods 2025

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
   struct sockaddr_in6* address = ( struct sockaddr_in6*) addr;
   socklen_t len = sizeof(addr);
   
   int st = sendto(idSocket, buffer, size, 0, (struct sockaddr*)address, (socklen_t) sizeof(*address));
   if(st == -1) {
      std::cerr << "Error en sendto: " << strerror(errno) << " (" << errno << ")" << std::endl;
      throw std::runtime_error("Cant send message");
   }
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
   
    struct sockaddr_in6 *srcAddr = ( struct sockaddr_in6*) addr;
    socklen_t addrLen = sizeof(struct sockaddr_in);

    // Llamada al sistema para recibir datos
    ssize_t bytesReceived = recvfrom(idSocket, buffer, size, 0, (struct sockaddr*)srcAddr, &addrLen);

    if (bytesReceived == -1) {
        std::cerr << "Error en recvfrom: " << strerror(errno) << " (" << errno << ")" << std::endl;
        throw std::runtime_error("VSocket::recvFrom: Error al recibir datos");
    }

    return static_cast<size_t>(bytesReceived);
}


/**
  *  AcceptConnection method
  *
  *  @return	VSocket* pointer to a new VSocket object
  *
  *  Accept a connection from a peer
  *
 **/
VSocket * VSocket::AcceptConnection() {
   throw std::runtime_error( "VSocket::AcceptConnection hola" );
   return NULL;
}
