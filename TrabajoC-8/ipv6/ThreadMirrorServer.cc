#include <iostream>
#include <thread>
#include "Socket.h"   // asume VSocket está declarado en este header

#define PORT 1234
#define BUFSIZE 512

void task(VSocket* client) {
    char buf[BUFSIZE];
    client->Read(buf, BUFSIZE);
    std::cout << "Server received: " << buf 
              << " from id: " << client->idSocket << std::endl;
    client->Write(buf);
    client->Close();
}

int main(int argc, char** argv) {
    // Creamos el socket en modo stream ('s') y habilitamos IPv6 (true)
    VSocket* server = new Socket('s', /* useIPv6 = */ true);

    // Bind al puerto en todas las interfaces IPv6 (in6addr_any)
    server->Bind(PORT);

    // Pasivo, backlog = 5
    server->MarkPassive(5);
    std::cout << "Server (IPv6) started on port " << PORT << std::endl;

    for (;;) {
        // AcceptConnection internamente usa sockaddr_in6
        VSocket* client = server->AcceptConnection();
        // Lanzar hilo para atender al cliente
        std::thread* worker = new std::thread(task, client);
        // Detach el hilo para no bloquear el bucle principal
        worker->detach();
    }

    // nunca llega aquí en un servidor típico
    delete server;
    return 0;
}
