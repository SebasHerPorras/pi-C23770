/**
 *
 **/

#include <cstddef>
#include <stdexcept>
#include <cstdio>
#include <cstring>			// memset

#include <sys/socket.h>
#include <arpa/inet.h>			// ntohs
#include <unistd.h>			// close
//#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>			// getaddrinfo, freeaddrinfo
#include <iostream>

#include "VSocket.h"


/**
  *  Class initializer
  *     use Unix socket system call
  *
  *  @param     char t: socket type to define
  *     's' for stream
  *     'd' for datagram
  *  @param     bool ipv6: if we need a IPv6 socket
  *
 **/
void VSocket::InitVSocket( char t, bool IPv6 ) {
   int IPVersion = 0;
   int SocketType = 0;
   this->IPv6 = IPv6;
   this->idSocket = socket(IPVersion = (IPv6 == false) ? AF_INET : AF_INET6,
      SocketType = (t == 'd') ? SOCK_DGRAM : SOCK_STREAM, 0);
   
   std::cout << "id socket: " << idSocket << std::endl;
}


/**
  *  Class initializer
  *
  *  @param     int descriptor: socket descriptor for an already opened socket
  *
 **/
void VSocket::InitVSocket( int descriptor ) {
   std::cout << "VSocket::InitVSocket( int descriptor )" << std::endl;
   this->idSocket = descriptor;

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
   int st = close(idSocket);

   if ( -1 == st ) {
      throw std::runtime_error( "Socket::Close()" );
   }

}

/**
  * Listen method
  *
  * @param      int queue: max pending connections to enqueue (server mode)
  *
  *  This method define how many elements can wait in queue
  *
 **/
int VSocket::Listen( int queue ) {

   int st = listen(this->idSocket, queue);

   if ( -1 == st ) {
      throw std::runtime_error( "VSocket::Listen( int )" );
   }

   return st;
}
/**
  * DoConnect method
  *   use "connect" Unix system call
  *
  * @param      char * host: host address in dot notation, example "10.1.104.187"
  * @param      int port: process address, example 80
  *
 **/
int VSocket::DoConnect( const char * hostip, int port ) {
   int st = -1;
   if(this->IPv6) {
      // Para IPv6 
      std::cout << "ipv6" << std::endl;
      struct sockaddr_in6 host6;
      struct sockaddr* ha;
      memset( &host6, 0, sizeof( host6 ) );
      host6.sin6_family = AF_INET6;

      st = inet_pton( AF_INET6, hostip, &host6.sin6_addr );
      std::cout << "st: " << st << std::endl;

      if ( 1 == st ) {	// 0 means invalid address, -1 means address error
         // throw std::runtime_error( "Socket::Connect( const char *, int ) [inet_pton]" );
         std::cout << "Success" << std::endl;
      }
      host6.sin6_port = htons( port );
      ha = (struct sockaddr *) &host6;
      uint64_t len = sizeof( host6 );
      st = connect( this->idSocket, ha, len );
      if ( -1 == st ) {
         throw std::runtime_error( "Socket::Connect( const char *, int ) [connect]" );
      }
   } else {
      // Para IPv4
      std::cout << "ipv4" << std::endl;
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
         throw( std::runtime_error( "VSocket::DoConnect, connect" ));
      }
   }

   return st;
}

/**
  * DoAccept method
  *    use "accept" Unix system call (man 3 accept) (server mode)
  *
  *  @returns   a new class instance
  *
  *  Waits for a new connection to service (TCP mode: stream)
  *
 **/
int VSocket::DoAccept(){
   int st = -1;
   struct sockaddr_in hostAccept;
   socklen_t hostAcceptSize = sizeof(struct sockaddr_in);
   /*On success, these system calls return a file descriptor for the
       accepted socket (a nonnegative integer).*/
   st = accept(this->idSocket, (struct sockaddr *) &hostAccept, &hostAcceptSize);

   if ( -1 == st ) {
      throw std::runtime_error( "VSocket::DoAccept()" );
   }

   return st;

}

/**
  * DoConnect method
  *   use "connect" Unix system call
  *
  * @param      char * host: host address in dns notation, example "os.ecci.ucr.ac.cr"
  * @param      char * service: process address, example "http"
  *
 **/
int VSocket::DoConnect( const char *host, const char *service ) {
   int st = -1;

   if ( 0 == st ) {
   } else {
      throw std::runtime_error( gai_strerror( st ) );
   }

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
   if (this->IPv6)
   {
      std::cout << "bind ipv6" << std::endl;
      // Create a struct in the af inet domain (ipv4)
      struct sockaddr_in6 addrIpv6;
      // Clean all fields inside struct
      memset((char*) &addrIpv6, 0, sizeof(addrIpv6));
      // Fill the fields
      addrIpv6.sin6_family = AF_INET6;
      addrIpv6.sin6_port = htons (port);

      addrIpv6.sin6_addr = in6addr_any;
      st = bind(idSocket, (sockaddr*) &addrIpv6, sizeof(addrIpv6));
   } else {
      // Create a struct in the af inet domain (ipv4)
      struct sockaddr_in addrIpv4;
      // Clean all fields inside struct
      memset((char*) &addrIpv4, 0, sizeof(addrIpv4));
      // Fill the fields
      addrIpv4.sin_family = AF_INET;
      
      addrIpv4.sin_port = htons (port);
    
      addrIpv4.sin_addr.s_addr = INADDR_ANY;
      st = bind(idSocket, (sockaddr*) &addrIpv4, sizeof(addrIpv4));
   }

   if (st == -1)
   {
      perror("Bind: ");
   }
   
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
   
   socklen_t len = sizeof(addr);
   const struct sockaddr* address = (const struct sockaddr*) addr;
   
   int st = sendto(idSocket, buffer, size, 0, address, (socklen_t) sizeof(*address));
   if(st == -1) {
      perror("Cant send message");
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
   struct sockaddr* address = (struct sockaddr*) addr; 
   socklen_t addrlen = sizeof(*address);
   int st = recvfrom(idSocket, buffer, size, 0, address, &addrlen);
   if (st == -1)
   {
      perror("Cant recv message");
   }
   
   return st;

}

int VSocket::MarkPassive(int backlog) {
    int st = listen(idSocket, backlog);

    if (st == -1) {
        perror("VSocket::listen");
        std::cout << "idSocket: " << idSocket << "  backlog: " << backlog << std::endl;
        throw std::runtime_error("VSocket::MARKPASSIVE failed");
    }

    return st;
}

