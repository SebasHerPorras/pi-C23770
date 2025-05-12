/*
 *  Ejemplo de sockets con IPv4
 *
 */

#include <stdio.h>
#include <string.h>

#include "VSocket.h"
#include "Socket.h"
#include <regex>

char* removeHtmlTags(char* input) {
   std::string inputStr(input); // Convertir char* a std::string
   std::regex htmlTagRegex("<[^>]*>");
   std::string result = std::regex_replace(inputStr, htmlTagRegex, "");
   char* output = new char[result.length() + 1]; // Reservar memoria para el resultado
   strcpy(output, result.c_str()); // Copiar el resultado a un char*
   return output;
}

int main( int argc, char * argv[] ) {
   const char * os = "http://os.ecci.ucr.ac.cr/";
   const char * osi = "10.84.166.62";
   const char * ose = "163.178.104.62";
   const char * request = "GET /pirofs/index.php?disk=Disk-01&cmd=ls HTTP/1.1\r\nhost: redes.ecci\r\n\r\n";
   const char * whale = (char *) "GET /aArt/index.php?disk=Disk-01&fig=whale-1.txt\r\nHTTP/v1.1\r\nhost: redes.ecci\r\n\r\n";
   const char * rabbit = (char *) "GET /aArt/index.php?disk=Disk-01&fig=rabbit.txt\r\nHTTP/v1.1\r\nhost: redes.ecci\r\n\r\n";
   const char * seahorse = (char *) "GET /aArt/index.php?disk=Disk-01&fig=seahorse.txt\r\nHTTP/v1.1\r\nhost: redes.ecci\r\n\r\n";
   VSocket * s;	
   char a[512];

   s = new Socket( 's' );
   s->MakeConnection( ose, 80 );
   s->Write(  rabbit );
   s->Read( a, 512 );
   char* result = removeHtmlTags(a);
   printf( "%s\n", result);

}

