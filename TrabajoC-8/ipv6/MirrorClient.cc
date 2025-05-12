#include <iostream>
#include <cstring>
#include "Socket.h"

#define PORT    1234
#define BUFSIZE 512

int main(int argc, char** argv) {
    VSocket* s;
    char buffer[BUFSIZE];

    // Creamos un socket stream ('s') y habilitamos IPv6 (dual-stack)
    s = new Socket('s', /*useIPv6=*/ true);

    memset(buffer, 0, BUFSIZE);

    // Conexión: puedes usar una IPv6 literal o nombre de host
    // Ejemplo IPv6 loopback: "::1"
    /*
    
      USAR PUERTOS DIFERENTES PARA CADA SERVIDOR, porque aveces no sueltan el puerto y no se puede volver a usar
    
    */

    const char* serverIP = (argc > 1 ? argv[1] : "::1");
    s->MakeConnection(serverIP, PORT);

    // Envío
    const char* msg = (argc > 2 ? argv[2] : "Hello world 2025 ...");
    s->Write(msg);

    // Recepción
    s->Read(buffer, BUFSIZE);
    std::cout << buffer << std::endl;

    s->Close();
    delete s;
    return 0;
}
