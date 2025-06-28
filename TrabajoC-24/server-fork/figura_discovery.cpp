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
#define SERVER_NAME "ServidorA"
#define SERVER_IP "172.16.123.84"      // IP local (solo para pruebas)
#define SERVER_PORT 8081           // Puerto TCP donde se reciben solicitudes de figuras
#define SERVER_DISCOVERY_PORT 5353 // Puerto UDP donde se escuchan mensajes de descubrimiento

using namespace std;

// Inicializa el sistema de archivos, cargando las figuras base al arrancar
FileSystem fs(true); // Carga figuras como gato, sombrilla, etc.

// ---------------------------------------------
// Hilo que escucha solicitudes de descubrimiento (UDP)
// ---------------------------------------------
void discovery_listener()
{
    Socket s('d'); // Crear socket tipo datagrama (UDP)
    s.BuildSocket('d');

    // Permitir reutilización del puerto
    int yes = 1;
    setsockopt(s.idSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    // Enlazar el socket al puerto de descubrimiento
    s.Bind(SERVER_DISCOVERY_PORT);

    sockaddr_in cliente{}; // Dirección del cliente que responde
    char buffer[512];

    cout << "[UDP] Escuchando descubrimiento en puerto " << SERVER_DISCOVERY_PORT << "...\n";

    // Bucle infinito para atender solicitudes de descubrimiento
    while (true)
    {
        size_t len = s.recvFrom(buffer, sizeof(buffer) - 1, &cliente); // Esperar mensaje
        buffer[len] = '\0';
        string mensaje(buffer);

        // Si es una solicitud válida de descubrimiento
        if (mensaje == "GET /servers")
        {
            vector<string> figs = fs.get_figuras(); // Obtener lista de figuras
            ostringstream oss;

            // Convertir lista a formato figura1,figura2,...
            for (size_t i = 0; i < figs.size(); ++i)
            {
                oss << figs[i];
                if (i != figs.size() - 1)
                    oss << ",";
            }

            // Armar respuesta: nombre | ip | lista_de_figuras
            string respuesta = string(SERVER_NAME) + " | " + SERVER_IP + " | " + oss.str();
            s.sendTo(respuesta.c_str(), respuesta.size(), &cliente);

            cout << "[DISCOVERY] Respondió: " << respuesta << endl;
        }
        // Mensaje de apagado (opcional, ignorado)
        else if (mensaje.rfind("Shutdown", 0) == 0)
        {
            cout << "[INFO] Mensaje de apagado recibido, ignorado: " << mensaje << endl;
        }
        // Cualquier otro mensaje se ignora
        else
        {
            cout << "[UDP] Mensaje ignorado: " << mensaje << endl;
        }
    }

    s.Close(); // Cierre del socket (nunca se ejecuta en este loop infinito)
}

// ---------------------------------------------
// Hilo TCP que responde con el contenido ASCII de una figura
// ---------------------------------------------
void tcp_figure_server()
{
    VSocket *servidor = new Socket('s'); // Socket tipo stream (TCP)
    servidor->Bind(SERVER_PORT);         // Enlazar al puerto TCP
    servidor->MarkPassive(5);            // Escuchar con cola de 5 conexiones

    cout << "[TCP] Servidor de figuras escuchando en puerto " << SERVER_PORT << "\n";

    // Bucle principal para aceptar conexiones
    while (true)
    {
        VSocket *cliente = servidor->AcceptConnection(); // Esperar conexión de un tenedor

        // Leer solicitud
        char buffer[512] = {0};
        cliente->Read(buffer, sizeof(buffer) - 1);
        buffer[511] = '\0';

        string request(buffer);
        string prefix = "GET /figure/";
        size_t pos = request.find(prefix);

        if (pos != string::npos)
        {
            // Extraer el resto después de "GET /figure/"
            string resto = request.substr(pos + prefix.length());

            // Cortar en el primer espacio (que viene antes del HTTP/1.1)
            size_t espacio = resto.find(' ');
            string nombre_figura = (espacio != string::npos) ? resto.substr(0, espacio) : resto;

            // Eliminar caracteres de nueva línea y retorno de carro
            nombre_figura.erase(remove(nombre_figura.begin(), nombre_figura.end(), '\r'), nombre_figura.end());
            nombre_figura.erase(remove(nombre_figura.begin(), nombre_figura.end(), '\n'), nombre_figura.end());

            // Buscar la figura en el sistema de archivos
            char *ascii = fs.find_figura(nombre_figura);

            if (!ascii)
            {
                // Buscar figura de error como respaldo
                ascii = fs.find_figura_error();
                if (ascii)
                {
                    cout << "[TCP] Figura '" << nombre_figura << "' no encontrada. Enviando figura de error.\n";
                }
                else
                {
                    string msg = "Figura no encontrada";
                    cliente->Write(msg.c_str(), msg.size());
                    cout << "[TCP] Figura '" << nombre_figura << "' no encontrada y figura de error no disponible.\n";
                    cliente->Close();
                    delete cliente;
                    continue;
                }
            }
            else
            {
                cout << "[TCP] Figura '" << nombre_figura << "' enviada correctamente.\n";
            }

            // Enviar figura (encontrada o de error)
            cliente->Write(ascii, strlen(ascii));
            delete[] ascii;
        }
        else
        {
            // Formato incorrecto en la solicitud
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
// Función principal: lanza los dos hilos
// ---------------------------------------------
int main()
{
    // Lanzar hilo UDP de descubrimiento
    thread t1(discovery_listener);
    // Lanzar hilo TCP para atender solicitudes de figuras
    thread t2(tcp_figure_server);

    cout << "[INIT] Servidor de figuras '" << SERVER_NAME << "' activo\n";

    // Esperar que terminen los hilos (no sucede en este diseño)
    t1.join();
    t2.join();

    return 0;
}
