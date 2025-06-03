#include "syscall.h"

int main() {
    int id;
    char buffer[512];
    char ip[20];
    char figura[50];
    char puerto[10];
    int port_num = 0;
    char request[150];
    int bytes_leidos;
    int i, j;

    // Pedir IP
    Write("Ingrese IP del servidor (ej: 172.28.xxx.xx): ", 45, 1);
    bytes_leidos = Read(ip, 20, 0);
    if (bytes_leidos > 0 && ip[bytes_leidos - 1] == '\n') {
        ip[bytes_leidos - 1] = '\0';
    }

    // Pedir puerto
    Write("Ingrese el puerto del servidor: ", 32, 1);
    bytes_leidos = Read(puerto, 10, 0);
    if (bytes_leidos > 0 && puerto[bytes_leidos - 1] == '\n') {
        puerto[bytes_leidos - 1] = '\0';
    }

    // Convertir string de puerto a entero (NachOS compatible)
    i = 0;
    port_num = 0;
    while (puerto[i] != '\0') {
        port_num = port_num * 10 + (puerto[i] - '0');
        i++;
    }

    // Pedir nombre de figura
    Write("Ingrese nombre de figura: ", 26, 1);
    bytes_leidos = Read(figura, 50, 0);
    if (bytes_leidos > 0 && figura[bytes_leidos - 1] == '\n') {
        figura[bytes_leidos - 1] = '\0';
    }

    // Crear socket
    id = Socket(AF_INET_NachOS, SOCK_STREAM_NachOS);

    // Conectar usando IP y puerto ingresado
    Connect(id, ip, port_num);

    // Armar request manualmente
    i = 0;
    char* inicio = "GET /figure?name=";
    char* fin = " HTTP/1.1\r\n\r\n";

    // Copiar inicio al request
    while (inicio[i] != '\0') {
        request[i] = inicio[i];
        i++;
    }

    // Copiar figura al request
    j = 0;
    while (figura[j] != '\0') {
        request[i] = figura[j];
        i++;
        j++;
    }

    // Copiar fin al request
    j = 0;
    while (fin[j] != '\0') {
        request[i] = fin[j];
        i++;
        j++;
    }

    request[i] = '\0';  // Terminar string

    // Enviar request
    Write(request, i, id);

    // Leer respuesta en bloques
    while ((bytes_leidos = Read(buffer, 512, id)) > 0) {
        Write(buffer, bytes_leidos, 1);
    }

    // Cerrar socket
    Close(id);

    return 0;
}
