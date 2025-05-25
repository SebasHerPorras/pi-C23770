#include "syscall.h"

int main() {
    int id;
    char buffer[512];
    char ip[20];
    char figura[50];
    char request[150];
    int bytes_leidos;

    // Pedir IP
    Write("Ingrese IP del servidor (ej: 172.28.234.96): ", 45, 1);
    bytes_leidos = Read(ip, 20, 0);  // Lee de entrada estándar (0)
    if (bytes_leidos > 0 && ip[bytes_leidos-1] == '\n') ip[bytes_leidos-1] = '\0'; // eliminar salto de línea

    // Pedir nombre de figura
    Write("Ingrese nombre de figura: ", 26, 1);
    bytes_leidos = Read(figura, 50, 0);
    if (bytes_leidos > 0 && figura[bytes_leidos-1] == '\n') figura[bytes_leidos-1] = '\0'; // eliminar salto de línea

    // Crear socket
    id = Socket(AF_INET_NachOS, SOCK_STREAM_NachOS);

    // Conectar a la IP y puerto 1234
    Connect(id, ip, 1234);

    // Armar el request HTTP
    // Ejemplo: "GET /figure?name=gato HTTP/1.1\r\n\r\n"
    // Concatenamos manualmente porque NachOS no tiene sprintf
    int i = 0;
    char* inicio = "GET /figure?name=";
    char* fin = " HTTP/1.1\r\n\r\n";

    // Copiar inicio al request
    while (inicio[i] != '\0') {
        request[i] = inicio[i];
        i++;
    }

    // Copiar figura al request
    int j = 0;
    while (figura[j] != '\0') {
        request[i] = figura[j];
        i++; j++;
    }

    // Copiar fin al request
    j = 0;
    while (fin[j] != '\0') {
        request[i] = fin[j];
        i++; j++;
    }

    request[i] = '\0';  // Terminar string

    // Enviar request
    Write(request, i, id);

    // Leer respuesta completa en bloques de 512
    while ((bytes_leidos = Read(buffer, 512, id)) > 0) {
      Write(buffer, bytes_leidos, 1);  // Mostrar cada bloque en pantalla
    }

    // Cerrar socket
    Close(id);

    return 0;
}
