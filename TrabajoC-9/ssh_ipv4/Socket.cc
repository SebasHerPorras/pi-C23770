/**
 *   CI0123 PIRO
 *
 **/

#include <stdio.h>	// for perror
#include <stdlib.h>	// for exit
#include <string.h>	// for memset
#include <arpa/inet.h>	// for inet_pton
#include <sys/types.h>	// for connect 
#include <sys/socket.h>
#include <iostream>
#include <unistd.h>
#include "Socket.h"

/**
  *  Class constructor
  *     use Unix socket system call
  *
  *  @param	char type: socket type to define
  *     's' for stream
  *     'd' for datagram
  *  @param	bool ipv6: if we need a IPv6 socket
  *
 **/
Socket::Socket( char type, bool IPv6 ){
   
   this->InitVSocket( type, IPv6 );

}

Socket::Socket( int fd ){
   this->InitVSocket( fd );
}

/**
  * Class destructor
  *
 **/
Socket::~Socket(){
   Close();
}


/**
  * Close method
  *    use Unix close system call (once opened a socket is managed like a file in Unix)
  *
 **/
void Socket::Close(){

}

//! METODOS NUEVOS
Socket * Socket::SSLInit() {
   this->SSLInitContext();

   SSL * ssl = SSL_new( (SSL_CTX *) this->SSLContext );
   if (ssl == nullptr)
   {
      std::cout << "ssl null pointer: Socket::SSLInit() " << std::endl;
   }
   
   this->SSLStruct = (void *) ssl;
   // std::cout << "this->SSLStruct: " << this->SSLStruct << std::endl;
   return NULL;
}

void Socket::SSLInitServer( const char * certFileName, const char * keyFileName) {
   // std::cout << "SSLInitServer( const char * certFileName, const char * keyFileName)" << certFileName << keyFileName << std::endl;
   this->SSLInitServerContext();
   SSL * ssl = SSL_new( (SSL_CTX *) this->SSLContext );

   if (ssl == nullptr)
   {
      std::cout << "ssl null pointer: Socket::SSLInitServer( const char * certFileName, const char * keyFileName) " << std::endl;
   }
   this->SSLStruct = (void *) ssl;

   SSLLoadCertificates(certFileName, keyFileName);
}

void Socket::SSLCreate( Socket * original ) {
   SSL * ssl;
   int st;

   // Creando una estructura a partir del contexto del socket original
   ssl = SSL_new((SSL_CTX *) original->SSLContext);
   if (ssl == nullptr)
   {
      std::cout << "ssl null pointer: Socket::SSLCreate() " << std::endl;
   }
   this->SSLStruct = (void*) ssl;
   // Adjuntando a la estructura ssl el socket fd del socket creado por accept
   int error = SSL_set_fd((SSL *) this->SSLStruct, this->idSocket);

   if (error == 0)
   {
      std::cout << "Socket::SSLCreate SSL_set_fd" << std::endl;
   }
   

}

void Socket::SSLAccept() {
   int error = SSL_accept((SSL*) this->SSLStruct);

   if (error < 0)
   {
      perror("Socket::SSLAccept() error");
      throw std::runtime_error( "Socket::SSLAccept()" );
   }
   
}

const char * Socket::SSLGetCipher() {
   return SSL_get_cipher( reinterpret_cast<SSL *>( this->SSLStruct ) );
}

void Socket::SSLInitContext() {
 
   const SSL_METHOD * method = TLS_client_method();
   if( method == nullptr) {
      std::cout << "Error: SSL_METHOD * method = TLS_client_method()" << std::endl;
   }

   SSL_CTX * context = SSL_CTX_new( method );
   if (context == nullptr) {
      perror("Cant intialize context");
   }
   
   this->SSLContext = (void *) context;

   if ( nullptr == method ) {
      throw std::runtime_error( "SSLSocket::InitContext( bool )" );

   }
}
size_t Socket::SSLRead( void * buffer, size_t size ) {

   int st = SSL_read((SSL*) this->SSLStruct, buffer, size);

   if ( -1 == st ) {
      perror("Socket::SSLRead( void * buffer, size_t size )");
      throw std::runtime_error( "SSLSocket::Read( void *, size_t )" );
   }

   return st;

}

size_t Socket::SSLWrite( const void * buffer, size_t size ) {

   int st = SSL_write( (SSL*) this->SSLStruct, buffer, size);
   std::cout << "Write 1: " << st << std::endl;
    if (st <= 0) {
        int error_code = SSL_get_error((SSL *)this->SSLStruct, st);
        const char *error_str = "Error desconocido";
        
        switch (error_code) {
            case SSL_ERROR_NONE:
                error_str = "SSL_ERROR_NONE";
                break;
            case SSL_ERROR_ZERO_RETURN:
                error_str = "SSL_ERROR_ZERO_RETURN";
                break;
            case SSL_ERROR_WANT_READ:
                error_str = "SSL_ERROR_WANT_READ";
                break;
            case SSL_ERROR_WANT_WRITE:
                error_str = "SSL_ERROR_WANT_WRITE";
                break;
            case SSL_ERROR_WANT_CONNECT:
                error_str = "SSL_ERROR_WANT_CONNECT";
                break;
            case SSL_ERROR_WANT_ACCEPT:
                error_str = "SSL_ERROR_WANT_ACCEPT";
                break;
            case SSL_ERROR_WANT_X509_LOOKUP:
                error_str = "SSL_ERROR_WANT_X509_LOOKUP";
                break;
            case SSL_ERROR_SYSCALL:
                error_str = "SSL_ERROR_SYSCALL";
                break;
            case SSL_ERROR_SSL:
                error_str = "SSL_ERROR_SSL";
                break;
        }
        
        std::cerr << "SSL_write error: " << error_str << " (Code: " << error_code << ")" << std::endl;
        
        throw std::runtime_error("SSLSocket::Write(const void *, size_t)");
    }
   return st;
}

void Socket::SSLLoadCertificates( const char * certFileName, const char * keyFileName ) {
   // SSL_CTX * context = instance variable
   int st;

   if ( SSL_CTX_use_certificate_file( (SSL_CTX*) this->SSLContext, certFileName, SSL_FILETYPE_PEM ) <= 0 ) {	 // set the local certificate from CertFile
      st = SSL_get_error( (SSL *) this->SSLStruct, st );
      ERR_print_errors_fp( stderr );
      abort();
   }

   if ( SSL_CTX_use_PrivateKey_file( (SSL_CTX*) this->SSLContext, keyFileName, SSL_FILETYPE_PEM ) <= 0 ) {	// set the private key from KeyFile (may be the same as CertFile)
      st = SSL_get_error( (SSL *) this->SSLStruct, st );
      ERR_print_errors_fp( stderr );
      abort();
   }

   if ( ! SSL_CTX_check_private_key( (SSL_CTX*) this->SSLContext ) ) {	// verify private key
      st = SSL_get_error( (SSL *) this->SSLStruct, st );
      ERR_print_errors_fp( stderr );
      std::cout << "error: ( ! SSL_CTX_check_private_key( (SSL_CTX*) this->SSLContext ) )" << std::endl;
      abort();
   }
}
/**
 *  Connect
 *     use SSL_connect to establish a secure conection
 *
 *  Create a SSL connection
 *
 *  @param	char * hostName, host name
 *  @param	int port, service number
 *
 **/
int Socket::SSLConnect( const char * hostName, int port ) {
   int st;
   int error = -1;
   st = this->DoConnect( hostName, port );		// Establish a non ssl connection first
   if (st == -1)
   {
      perror("SSLSocket::Connect( char *, int ) failed");
   }
   // The SSL_set_fd function assigns a socket to a Secure Sockets Layer (SSL) structure.
   error = SSL_set_fd((SSL *)this->SSLStruct, this->idSocket);
   if (error <= 0)
   {
      perror("SSL set fd failed");
   }
   
   // The SSL_connect function starts a Secure Sockets Layer (SSL) session with a remote server application.
   error = SSL_connect((SSL *)this->SSLStruct);
   if (error == -1)
   {
      perror("SSL_connect failed");
      throw std::runtime_error( "SSLConnect( const char * hostName, int port )" );
   } else if (error == 0)
   {
      perror("SSL_connect equal to zero");
      throw std::runtime_error( "SSLConnect( const char * hostName, int port )" );
   }
   

   return st;

}
void Socket::SSLInitServerContext() {
   
   SSL_library_init();
   OpenSSL_add_all_algorithms();
   SSL_load_error_strings();


   const SSL_METHOD* method = TLS_server_method();

   SSL_CTX* context = SSL_CTX_new(method);

   if (!context) {
      perror("Cant intialize context");
   }

   this->SSLContext = (void *) context;

   if ( nullptr == method ) {
      throw std::runtime_error( "SSLSocket::InitContext( bool )" );

   }

}

void Socket::SSLShowCerts() {
   X509 *cert;
   char *line;

   cert = SSL_get_peer_certificate( (SSL *) this->SSLStruct );		 // Get certificates (if available)
   if ( nullptr != cert ) {
      printf("Server certificates:\n");
      line = X509_NAME_oneline( X509_get_subject_name( cert ), 0, 0 );
      printf( "Subject: %s\n", line );
      free( line );
      line = X509_NAME_oneline( X509_get_issuer_name( cert ), 0, 0 );
      printf( "Issuer: %s\n", line );
      free( line );
      X509_free( cert );
   } else {
      printf( "No certificates.\n" );
   }
}

Socket * Socket::Accept() {
   int id;
   Socket * other;

   id = this->DoAccept();

   other = new Socket( id );

   return other;
}

/**
  * Connect method
  *   use "connect" Unix system call
  *
  * @param	char * host: host address in dot notation, example "10.1.104.187"
  * @param	int port: process address, example 80
  *
 **/
int Socket::Connect( const char * host, int port ) {
   return this->DoConnect( host, port );
}


/**
  * Read method
  *   use "read" Unix system call (man 3 read)
  *
  * @param	void * text: buffer to store data read from socket
  * @param	int size: buffer capacity, read will stop if buffer is full
  *
 **/
size_t Socket::Read( void * text, size_t size ) {
   int st = read(idSocket, text, size);
   std::cout << "Read" << std::endl;
   if ( -1 == st ) {
      throw std::runtime_error( "Socket::Read( const void *, size_t )" );
   }

   return st;

}


/**
  * Write method
  *   use "write" Unix system call (man 3 write)
  *
  * @param	void * buffer: buffer to store data write to socket
  * @param	size_t size: buffer capacity, number of bytes to write
  *
 **/
size_t Socket::Write( const void *text, size_t size ) {
   int st = write(idSocket, text, size);
   std::cout << "Write 1" << std::endl;

   if ( -1 == st ) {
      throw std::runtime_error( "Socket::Write( void *, size_t )" );
   }

   return st;

}


/**
  * Write method
  *
  * @param	char * text: string to store data write to socket
  *
  *  This method write a string to socket, use strlen to determine how many bytes
  *
 **/
size_t Socket::Write( const char *text ) {
   int st = write(idSocket, text, strlen(text));
   std::cout << "write 2" << std::endl;

   if ( -1 == st ) {
      throw std::runtime_error( "Socket::Write( void *, size_t )" );
   }

   return st;

}

