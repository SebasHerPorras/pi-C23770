#include <iostream>
#include <thread>
#include <map>
#include <mutex>
#include <sstream>
#include <chrono>
#include <cstring>
#include <vector>
#include <algorithm>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "Socket.h"

// Constantes utilizadas
#define DISCOVERY_PORT 5353   // Puerto UDP para descubrimiento
#define TCP_SERVER_PORT 8081  // Puerto TCP del servidor de figuras
#define CLIENT_HTTP_PORT 8080 // Puerto donde el cliente hace peticiones HTTP
#define TIMEOUT_RESPONSE 5    // Tiempo de espera máximo para recibir respuestas UDP
#define BROADCAST_WAIT 120    // Espera entre rondas de descubrimiento (segundos)

using namespace std;

// Lista de direcciones broadcast para descubrimiento (solo localhost en este ejemplo)
vector<string> broadcast_ips = {
    "127.0.0.255", // broadcast local (para pruebas)
    "172.16.123.15", // /28 (Profes)
    "172.16.123.31", // /28 ISLA 1
    "172.16.123.47", // /28 ISLA 2
    "172.16.123.63", // /28 ISLA 3
    "172.16.123.79", // /28 ISLA 4
    "172.16.123.95", // /28 ISLA 5
    "172.16.123.111", // /28 ISLA 6
    "10.1.141.68", // publica de isla 6
    "10.1.137.31"
};

// Tabla de ruteo: figura -> IP del servidor que la contiene
map<string, string> tabla_ruteo;
mutex tabla_mutex; // Protege el acceso a la tabla

char* generar_figura_error()
{
    // Definir una figura de error en un buffer dinámico
    char *figura_de_error = new char[400]; // Asignar suficiente espacio para la figura

    // Llenar la figura con el diseño de error
    const char *error =
        "           ______\n\
          /      \\\n\
         |  X  X  |\n\
         |    ^   |  \n\
         |   ---  |   \n\
          \\______/\n\
            |  |\n\
     _______|  |_______\n\
    |      ERROR       |\n\
    |  SYSTEM FAILURE  |\n\
    | FIGURE NOT FOUND |\n\
    |__________________|\n\
       __/        \\__\n\
      /              \\\n\
     /________________\\\n";

    // Copiar el contenido del error al buffer
    std::strcpy(figura_de_error, error);
    // std::cout << figura_de_error << std::endl;

    // Retornar el puntero a la figura de error
    return figura_de_error;
}

// ---------------------------------------------
// Hilo de descubrimiento: envía broadcast y recibe respuestas UDP
// ---------------------------------------------
void discovery_thread()
{
    Socket s('d'); // Socket tipo datagrama (UDP)
    s.BuildSocket('d');

    // Permitir broadcast en este socket
    int yes = 1;
    setsockopt(s.idSocket, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes));

    struct timeval timeout;
    timeout.tv_sec = TIMEOUT_RESPONSE;
    timeout.tv_usec = 0;
    setsockopt(s.idSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(DISCOVERY_PORT);

    while (true)
    {
        cout << "[DISCOVERY] Nueva ronda de descubrimiento\n";

        // Enviar broadcast a todas las IPs configuradas
        string mensaje = "GET /servers";
        for (const auto &ip : broadcast_ips)
        {
            inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
            s.sendTo(mensaje.c_str(), mensaje.size(), &addr);
            cout << "[BROADCAST] Enviado a " << ip << endl;
        }

        // Esperar respuestas durante TIMEOUT_RESPONSE segundos
        auto start = chrono::steady_clock::now();
        char buffer[512];
        sockaddr_in senderAddr{};

        while (true)
        {
            auto now = chrono::steady_clock::now();
            if (chrono::duration_cast<chrono::seconds>(now - start).count() > TIMEOUT_RESPONSE)
                break;

            // Recibir datos
            size_t len = s.recvFrom(buffer, sizeof(buffer) - 1, &senderAddr);
            if (len == 0)
            {
                // Timeout sin datos, seguir escuchando hasta que termine TIMEOUT_RESPONSE
                continue;
            }
            buffer[len] = '\0';

            // Esperado: ServerName|ip|figura1,figura2,...
            string respuesta(buffer);
            std::cout << "respuesta de server: " << respuesta <<std::endl;
            istringstream iss(respuesta);
            string nombre, ip, lista;

            // Parsear respuesta y registrar figuras
            if (getline(iss, nombre, '|') &&
                getline(iss, ip, '|') &&
                getline(iss, lista))
            {
                // Limpiar espacios
                nombre.erase(remove(nombre.begin(), nombre.end(), ' '), nombre.end());
                ip.erase(remove(ip.begin(), ip.end(), ' '), ip.end());
                lista.erase(remove(lista.begin(), lista.end(), ' '), lista.end());

                istringstream figs_stream(lista);
                string figura;
                lock_guard<mutex> lock(tabla_mutex);
                while (getline(figs_stream, figura, ','))
                {
                    tabla_ruteo[figura] = ip;
                    cout << "[RUTEO] Figura '" << figura << "' registrada con IP " << ip << endl;
                }
            }
        }

        // Esperar antes de hacer otra ronda
        this_thread::sleep_for(chrono::seconds(BROADCAST_WAIT));
    }

    s.Close();
}

// ---------------------------------------------
// Función que atiende una conexión HTTP
// ---------------------------------------------
void manejar_peticion_http(VSocket *cliente)
{
    char buffer[2048] = {0};
    size_t bytes = cliente->Read(buffer, sizeof(buffer) - 1);
    buffer[bytes] = '\0';

    string request(buffer);
    string prefix = "GET /figure?name=";
    size_t pos = request.find(prefix);

    if (pos != string::npos)
    {
        // Extraer el nombre de la figura de la URL
        string nombre_figura = request.substr(pos + prefix.length());
        size_t fin = nombre_figura.find(' ');
        if (fin != string::npos)
            nombre_figura = nombre_figura.substr(0, fin);

        std::cout<<nombre_figura<<std::endl;
        // Filtrar caracteres no válidos
        /*nombre_figura.erase(remove_if(nombre_figura.begin(), nombre_figura.end(),
                                      [](char c)
                                      { return !isalnum(c) && c != '_' && c != '-'; }),
                            nombre_figura.end());*/

        string ip_destino;
        {
            // Buscar la figura en la tabla de ruteo
            lock_guard<mutex> lock(tabla_mutex);
            if (tabla_ruteo.find(nombre_figura) != tabla_ruteo.end())
                ip_destino = tabla_ruteo[nombre_figura];
        }
        std::cout << "ip de la figura: " << ip_destino << std::endl;
        if (!ip_destino.empty())
        {
            try
            {
                Socket servidor_tcp('s');
                servidor_tcp.BuildSocket('s');
                servidor_tcp.MakeConnection(ip_destino.c_str(), TCP_SERVER_PORT);

                // Solicitar la figura al servidor correspondiente
                string solicitud = "GET /figure/" + nombre_figura;
                servidor_tcp.Write(solicitud.c_str(), solicitud.size());

                char respuesta[2048] = {0};
                struct timeval timeout;
                timeout.tv_sec = TIMEOUT_RESPONSE;
                timeout.tv_usec = 0;
                setsockopt(servidor_tcp.idSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

                size_t bytes = servidor_tcp.Read(respuesta, sizeof(respuesta) - 1);

                if (bytes == 0)
                {
                    // No se recibió nada, se considera error
                    throw std::runtime_error("Respuesta vacía del servidor");
                }

                respuesta[bytes] = '\0';

                // Construir respuesta HTTP válida con el contenido (figura o error)
                string body = "<html><body><pre>\n" + string(respuesta) + "\n</pre></body></html>";
                string http_response =
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/html; charset=UTF-8\r\n"
                    "Content-Length: " +
                    to_string(body.size()) + "\r\n\r\n" + body;

                cliente->Write(http_response.c_str(), http_response.size());
                cout << "[HTTP] Figura '" << nombre_figura << "' enviada al cliente.\n";
                servidor_tcp.Close();
            }
            catch (exception e)
            {
                std :: cout << "error del try catch: " << e.what() << std::endl;
                // Si ocurre una excepción, se responde con 404
                string err = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
                cliente->Write(err.c_str(), err.size());
                cout << "[HTTP] Timeout/error al contactar servidor de figura '" << nombre_figura << "'.\n";
            }
        }
        else
        {
            // Si la figura no está en la tabla de ruteo, responder con figura de error
            char *error_figura = generar_figura_error();

            string body = "<html><body><pre>\n" + string(error_figura) + "\n</pre></body></html>";
            string http_response =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html; charset=UTF-8\r\n"
                "Content-Length: " +
                to_string(body.size()) + "\r\n\r\n" + body;

            cliente->Write(http_response.c_str(), http_response.size());
            cout << "[HTTP] Figura '" << nombre_figura << "' no registrada. Figura de error enviada.\n";

            delete[] error_figura;
        }
    }
    else if (request.find("GET /list") != string::npos)
    {
        // Construir lista HTML de todas las figuras disponibles
        string body = "<html><body><h2>Figuras disponibles:</h2><ul>";
        {
            lock_guard<mutex> lock(tabla_mutex);
            for (const auto &par : tabla_ruteo)
                body += "<li>" + par.first + "</li>";
        }
        body += "</ul></body></html>";

        string http_response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html; charset=UTF-8\r\n"
            "Content-Length: " +
            to_string(body.size()) + "\r\n\r\n" + body;

        cliente->Write(http_response.c_str(), http_response.size());
        cout << "[HTTP] Lista de figuras enviada al cliente.\n";
    }
    else
    {
        // Cualquier otra solicitud no válida
        string err = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
        cliente->Write(err.c_str(), err.size());
        cout << "[HTTP] Solicitud inválida.\n";
    }

    cliente->Close();
    delete cliente;
}

// ---------------------------------------------
// Hilo HTTP que atiende a múltiples clientes concurrentes
// ---------------------------------------------
void atender_clientes_http()
{
    VSocket *servidor = new Socket('s');
    servidor->Bind(CLIENT_HTTP_PORT);
    servidor->MarkPassive(5);
    cout << "[HTTP] Servidor escuchando en puerto " << CLIENT_HTTP_PORT << "\n";

    while (true)
    {
        VSocket *cliente = servidor->AcceptConnection();
        thread(manejar_peticion_http, cliente).detach(); // Cada cliente en su hilo
    }

    delete servidor;
}

// ---------------------------------------------
// Función principal
// ---------------------------------------------
int main()
{
    cout << "[INIT] Tenedor activo.\n";

    // Iniciar hilos: descubrimiento y HTTP
    thread t1(discovery_thread);
    thread t2(atender_clientes_http);


    t1.join();
    t2.join();

    return 0;
}
