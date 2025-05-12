#include <iostream>
#include <string>
#include "Socket.h"

#define PORT 1231
#define BUFSIZE 1024

int main(int argc, char *argv[])
{
    char buffer[BUFSIZE];
    std::string figure_name = "gato";        // Por defecto
    std::string server_ip = "172.28.234.96"; // IP por defecto (quemada)

    // Interpretación de argumentos
    if (argc == 1)
    {
        std::cout << "\nUsando figura por defecto: 'gato'" << std::endl;
        std::cout << "IP por defecto: " << server_ip << std::endl;
        std::cout << "\nEjemplo de uso: ./MirrorClient gato <opcional>172.28.234.96\n"
                  << std::endl;
    }
    else if (argc == 2)
    {
        figure_name = argv[1];
        std::cout << "\nFigura solicitada: '" << figure_name << "'" << std::endl;
        std::cout << "IP por defecto: " << server_ip << std::endl;
    }
    else if (argc >= 3)
    {
        figure_name = argv[1];
        server_ip = argv[2];
        std::cout << "\nFigura solicitada: '" << figure_name << "'" << std::endl;
        std::cout << "IP proporcionada: " << server_ip << std::endl;
    }

    // Crear y conectar socket
    Socket *s = new Socket('s');
    s->MakeConnection(server_ip.c_str(), PORT);

    // Armar y enviar request HTTP
    std::string request = "GET /figure/" + figure_name;
    s->Write(request.c_str(), request.size());

    // Leer y mostrar la respuesta
    int bytesRead = 0;
    do
    {
        bytesRead = s->Read(buffer, BUFSIZE - 1);
        if (bytesRead > 0)
        {
            buffer[bytesRead] = '\0';
            printf("%s", buffer);
        }
    } while (bytesRead > 0);

    std::cout << std::endl;
    return 0;
}
