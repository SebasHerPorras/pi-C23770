#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "Socket.h"

#define TENEDOR_IP "172.16.123.84"
#define TENEDOR_PORT 8080
#define SERVER_DISCOVERY_PORT 5353
#define BUFFER_SIZE 4096

std::vector<std::string> broadcast_ips = {
    "172.16.123.95"};

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Uso:\n"
                  << "  " << argv[0] << " <nombre_figura>\n"
                  << "  " << argv[0] << " shutdown <ServerName>\n";
        return 1;
    }

    std::string cmd = argv[1];

    if (cmd == "shutdown")
    {
        if (argc != 3)
        {
            std::cerr << "[ERROR] Debe especificar el nombre del servidor para apagar.\n";
            return 1;
        }

        std::string server_name = argv[2];
        std::string mensaje = "Shutdown " + server_name;

        Socket udp('d');
        udp.BuildSocket('d');

        int yes = 1;
        setsockopt(udp.idSocket, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(SERVER_DISCOVERY_PORT);

        for (const auto &ip : broadcast_ips)
        {
            inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
            udp.sendTo(mensaje.c_str(), mensaje.size(), &addr);
            std::cout << "[CLIENTE] Enviado broadcast de apagado a " << ip << ": " << mensaje << "\n";
        }

        udp.Close();
        return 0;
    }

    // Cliente de figura normal por TCP
    std::string figura = argv[1];
    try
    {
        Socket client('s');
        std::cout << "[CLIENTE] Conectando al tenedor en " << TENEDOR_IP << ":" << TENEDOR_PORT << "...\n";
        client.MakeConnection(TENEDOR_IP, TENEDOR_PORT);
        std::cout << "[CLIENTE] Conectado al tenedor.\n";

        std::string http_request = "GET /figure?name=" + figura + " HTTP/1.1\r\nHost: cliente\r\n\r\n";
        std::cout << "[CLIENTE] Enviando solicitud HTTP: " << http_request;

        client.Write(http_request.c_str(), http_request.size());

        char buffer[BUFFER_SIZE];
        std::string total_response;

        while (true)
        {
            ssize_t bytes = client.Read(buffer, sizeof(buffer) - 1);
            if (bytes <= 0)
                break;
            buffer[bytes] = '\0';
            total_response += buffer;
        }

        std::cout << "[CLIENTE] Respuesta recibida:\n"
                  << total_response << std::endl;

        client.Close();
    }
    catch (const std::exception &e)
    {
        std::cerr << "[ERROR] " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
