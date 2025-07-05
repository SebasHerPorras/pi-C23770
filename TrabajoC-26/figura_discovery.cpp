#include <iostream>
#include <thread>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cstring>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "Socket.h"
#include "file_system.hpp"

// Parámetros de red
#define SERVER_NAME "HOLA_SERVER"
#define SERVER_IP "172.16.123.36"      // IP del servidor
#define SERVER_PORT 8081               // Puerto TCP para figuras
#define SERVER_DISCOVERY_PORT 5353     // Puerto UDP para descubrimiento

using namespace std;

// Lista de direcciones broadcast
vector<string> broadcast_ips = {
    "172.16.123.15",
    "172.16.123.31",
    "172.16.123.47",
    "172.16.123.63",
    "172.16.123.79",
    "172.16.123.95",
    "172.16.123.111",
    
};

// Inicializa el sistema de archivos
FileSystem fs(true);

// ---------------------------------------------
// Hilo que responde solicitudes de descubrimiento (UDP)
// ---------------------------------------------
void discovery_listener() {
    Socket recv_sock('d');
    recv_sock.BuildSocket('d');

    // Configurar para reutilizar dirección
    int yes = 1;
    setsockopt(recv_sock.idSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    // Bind al puerto de descubrimiento
    recv_sock.Bind(SERVER_DISCOVERY_PORT);

    // Socket para enviar respuestas (no necesita bind)
    Socket send_sock('d');
    send_sock.BuildSocket('d');

    sockaddr_in cliente{};
    char buffer[512];

    cout << "[UDP] Escuchando descubrimiento en puerto " << SERVER_DISCOVERY_PORT << "...\n";

    while (true) {
        size_t len = recv_sock.recvFrom(buffer, sizeof(buffer) - 1, &cliente);
        buffer[len] = '\0';
        string mensaje(buffer);

        if (mensaje == "GET /servers") {
            vector<string> figs = fs.get_figuras();
            ostringstream oss;
            for (size_t i = 0; i < figs.size(); ++i) {
                oss << figs[i];
                if (i != figs.size() - 1)
                    oss << ",";
            }

            string respuesta = string(SERVER_NAME) + " | " + SERVER_IP + " | " + oss.str();
            send_sock.sendTo(respuesta.c_str(), respuesta.size(), &cliente);

            cout << "[DISCOVERY] Respondió: " << respuesta << endl;
        }
        else if (mensaje.rfind("Shutdown", 0) == 0) {
            cout << "[INFO] Mensaje de apagado recibido, ignorado: " << mensaje << endl;
        }
        else {
            cout << "[UDP] Mensaje ignorado: " << mensaje << endl;
        }
    }

    recv_sock.Close();
    send_sock.Close();
}

// ---------------------------------------------
// NUEVO: hilo que anuncia el servidor activamente por broadcast
// ---------------------------------------------
void broadcast_advertiser()
{
    Socket s('d');
    s.BuildSocket('d');

    int yes = 1;
    setsockopt(s.idSocket, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SERVER_DISCOVERY_PORT);

    while (true)
    {
        vector<string> figs = fs.get_figuras();
        ostringstream oss;
        for (size_t i = 0; i < figs.size(); ++i)
        {
            oss << figs[i];
            if (i != figs.size() - 1)
                oss << ",";
        }

        string anuncio = string(SERVER_NAME) + " | " + SERVER_IP + " | " + oss.str();

        for (const auto &ip : broadcast_ips)
        {
            inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
            s.sendTo(anuncio.c_str(), anuncio.size(), &addr);
            cout << "[BROADCAST] Anuncio enviado a " << ip << ": " << anuncio << endl;
        }

        this_thread::sleep_for(chrono::seconds(120)); // espera 2 minutos
    }

    s.Close();
}

// ---------------------------------------------
// Hilo TCP que responde con la figura ASCII
// ---------------------------------------------
void tcp_figure_server()
{
    VSocket *servidor = new Socket('s');
    servidor->Bind(SERVER_PORT);
    servidor->MarkPassive(5);

    cout << "[TCP] Servidor escuchando en puerto " << SERVER_PORT << "\n";

    while (true)
    {
        VSocket *cliente = servidor->AcceptConnection();

        char buffer[512] = {0};
        cliente->Read(buffer, sizeof(buffer) - 1);
        buffer[511] = '\0';

        string request(buffer);
        string prefix = "GET /figure/";
        size_t pos = request.find(prefix);

        if (pos != string::npos)
        {
            string resto = request.substr(pos + prefix.length());
            size_t espacio = resto.find(' ');
            string nombre_figura = (espacio != string::npos) ? resto.substr(0, espacio) : resto;

            nombre_figura.erase(remove(nombre_figura.begin(), nombre_figura.end(), '\r'), nombre_figura.end());
            nombre_figura.erase(remove(nombre_figura.begin(), nombre_figura.end(), '\n'), nombre_figura.end());

            char *ascii = fs.find_figura(nombre_figura);

            if (!ascii)
            {
                ascii = fs.find_figura_error();
                if (ascii)
                    cout << "[TCP] Figura '" << nombre_figura << "' no encontrada. Enviando figura de error.\n";
                else
                {
                    string msg = "Figura no encontrada";
                    cliente->Write(msg.c_str(), msg.size());
                    cliente->Close();
                    delete cliente;
                    continue;
                }
            }
            else
            {
                cout << "[TCP] Figura '" << nombre_figura << "' enviada correctamente.\n";
            }

            cliente->Write(ascii, strlen(ascii));
            delete[] ascii;
        }
        else
        {
            string err = "Formato inválido";
            cliente->Write(err.c_str(), err.size());
            cout << "[TCP] Solicitud inválida recibida: " << request << endl;
        }

        cliente->Close();
        delete cliente;
    }

    delete servidor;
}

// ---------------------------------------------
// MAIN con hilos
// ---------------------------------------------
int main()
{
    thread t1(discovery_listener);
    thread t2(tcp_figure_server);
    thread t3(broadcast_advertiser); // NUEVO

    cout << "[INIT] Servidor de figuras '" << SERVER_NAME << "' activo\n";

    t1.join();
    t2.join();
    t3.join();

    return 0;
}
