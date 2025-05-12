#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>    // memset
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>  // waitpid

#include "Socket.h"

#define PORT 1235
#define BUFSIZE 512

VSocket* s1 = nullptr;

void sigint_handler(int signum) {
    std::cout << "\n[INFO] Señal SIGINT capturada. Cerrando socket y saliendo...\n";
    if (s1) {
        s1->Close();
        delete s1;
    }
    exit(0);
}

void sigchld_handler(int signum) {
    // Evita procesos zombis
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main(int argc, char** argv) {
    signal(SIGINT, sigint_handler);
    signal(SIGCHLD, sigchld_handler);

    // Crear socket stream en modo IPv6 (dual-stack)
    s1 = new Socket('s', /*useIPv6=*/ true);

    // Bind al puerto en todas las interfaces IPv6 y IPv4-mapeadas
    s1->Bind(PORT);
    s1->MarkPassive(5);

    std::cout << "\nServer (fork + IPv6) started on port: " << PORT << std::endl;

    for (;;) {
        VSocket* s2 = s1->AcceptConnection();
        pid_t childpid = fork();
        if (childpid < 0) {
            perror("server: fork error");
        } else if (childpid == 0) {
            // Código del hijo
            s1->Close();  // cerrar socket de escucha en el hijo

            char a[BUFSIZE];
            memset(a, 0, BUFSIZE);
            s2->Read(a, BUFSIZE);
            std::cout << "Server received: " << a << " from id: " << s2->idSocket << std::endl;
            s2->Write(a);
            s2->Close();
            exit(0);
        }
        // Código del padre
        s2->Close();  // cerrar socket de conexión en el padre
    }

    // nunca llegará aquí
    delete s1;
    return 0;
}
