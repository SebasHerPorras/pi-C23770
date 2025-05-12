#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>    // memset
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>  // waitpid

#include "Socket.h" // Asegúrate de que esta clase soporte SSL, igual que en el ejemplo inicial.

#define PORT 1235
#define BUFSIZE 1024

Socket* s1 = nullptr; // usaré Socket en vez de VSocket, para poder hacer SSL.

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

void Service(Socket* client) {
    const char* ServerResponse = "\n<Body>\n\
\t<Server>os.ecci.ucr.ac.cr</Server>\n\
\t<dir>ci0123</dir>\n\
\t<Name>Proyecto Integrador Redes y Sistemas Operativos</Name>\n\
\t<NickName>PIRO</NickName>\n\
\t<Description>Consolidar e integrar los conocimientos de redes y sistemas operativos</Description>\n\
\t<Author>profesores PIRO</Author>\n\
</Body>\n";
    const char *validMessage = "\n<Body>\n\
\t<UserName>piro</UserName>\n\
\t<Password>ci0123</Password>\n\
</Body>\n";

    char buf[BUFSIZE] = {0};

    client->SSLAccept();
    client->SSLShowCerts();

    int bytes = client->SSLRead(buf, sizeof(buf) - 1); // -1 para terminar en \0
    if (bytes > 0) {
        buf[bytes] = '\0';
        printf("Client msg: \"%s\"\n", buf);

        if (!strcmp(validMessage, buf)) {
            client->SSLWrite(ServerResponse, strlen(ServerResponse));
        } else {
            client->SSLWrite("Invalid Message", strlen("Invalid Message"));
        }
    }

    client->Close();
}

int main(int argc, char** argv) {
    signal(SIGINT, sigint_handler);
    signal(SIGCHLD, sigchld_handler);

    int port = PORT;
    if (argc > 1) {
        port = atoi(argv[1]);
    }

    // Crear socket tipo servidor con IPv6 dual-stack
    s1 = new Socket('s', /*useIPv6=*/true);
    s1->Bind(port);
    s1->MarkPassive(5); // o Listen(5)

    // Inicializar SSL del servidor
    s1->SSLInitServer("ci0123.pem", "ci0123.pem");

    std::cout << "\n[INFO] Server (fork + SSL + IPv6) started on port: " << port << std::endl;

    for (;;) {
        Socket* s2 = s1->Accept(); // usar Accept normal
        s2->SSLCreate(s1); // crear contexto SSL para el cliente

        pid_t childpid = fork();
        if (childpid < 0) {
            perror("server: fork error");
            s2->Close();
            delete s2;
        } else if (childpid == 0) {
            // Código del hijo
            delete s1; // El hijo no necesita el socket de escucha
            Service(s2);
            delete s2;
            exit(0);
        } else {
            // Código del padre
            s2->Close(); // Padre cierra descriptor duplicado
            delete s2;
        }
    }

    return 0;
}
