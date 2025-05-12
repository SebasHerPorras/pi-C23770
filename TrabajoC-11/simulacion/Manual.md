# Manual de Compilación, Ejecución y Pruebas  

## Requisitos

- `g++` (C++11 compatible)
- `make`
- `netcat` (`nc`) para pruebas manuales
- Sistema Linux o WSL

---

## Compilación

Desde la raíz del proyecto, ejecutar:

```bash
make
```

Esto crea:

exec/MirrorClient.out — cliente de prueba

exec/ThreadMirrorServer.out — servidor de figuras

filesystem.dat — archivo binario donde se guardan las figuras (generado automáticamente al iniciar el servidor)

**Nota: filesystem.dat se crea automáticamente en el directorio raíz cuando el servidor se ejecuta por primera vez. Contiene las figuras en formato binario por bloques.**

## Ejecución

Iniciar el servidor (en una terminal)

```bash
./exec/ThreadMirrorServer.out
```

Se muestra en consola algo como:

```csharp
Figuras cargadas correctamente en el sistema de archivos.
[Protocol] Server started on port 1231
[Protocol] Broadcasting announcement: ServidorA | 172.16.123.51 | arbol_navidad, gato, barco, sombrilla
```

Ejecutar el cliente (en otra terminal)

```bash
./exec/MirrorClient.out gato 127.0.0.1
```

Resultado esperado: la figura "gato" se imprime por pantalla y el servidor muestra logs del protocolo, así:

```bash
VSocket::WaitForConnection: client socket: 5
[Protocol] Accepted new connection, socket id: 5
[Protocol] Received request: "GET /figure/gato" from socket id 5
[Protocol] Petición ASCII-art para figura: 'gato'
[Protocol] Figura encontrada (262 bytes), enviando en chunks...
[Protocol]   → Enviado chunk de 262 bytes
```

## Pruebas manuales con nc (netcat)

1. Descubrimiento del servidor simulando comportamiento de un tenedor

```bash
printf "GET /servers" | nc 127.0.0.1 1231
```

Cliente ve:

```bash
ServidorA | 172.16.123.51 | arbol_navidad, gato, barco, sombrilla
```

Servidor muestra:

```csharp
[Protocol] Received request: "GET /servers"
[Protocol] RESPONDIENDO a GET /servers con: ...
```

2. Solicitud de figura existente

```bash
printf "GET /figure/barco" | nc 127.0.0.1 1231
```

Cliente ve: figura del barco en ASCII

Servidor muestra:

```csharp
[Protocol] Received request: "GET /figure/barco"
[Protocol] Petición ASCII-art para figura: 'barco'
[Protocol] Figura encontrada (size) enviando en chunks...
```

3. Solicitud de figura inexistente

```bash
printf "GET /figure/dragon" | nc 127.0.0.1 1231
```

Cliente ve figura de error con mensaje:

```bash
404 Not Found
```

Servidor muestra:

```csharp
[Protocol] Figura 'dragon' no encontrada, enviando 404
```

4. Apagar el servidor con shutdown

```bash
printf "Shutdown ServidorA" | nc 127.0.0.1 1231
```

Servidor muestra:

```csharp
[Protocol] Received request: "Shutdown ServidorA"
[Protocol] Coincide con este servidor → cerrando.
```

El proceso finaliza automáticamente.

### Resultados esperados en archivos

filesystem.dat contiene todas las figuras ASCII almacenadas de forma binaria.

Se estructura por bloques de 256 bytes con cabeceras y punteros internos.

Puede inspeccionar su contenido con:

```bash
hexdump -C filesystem.dat | less
```

### Nota sobre el puerto

Si cierra el servidor y lo reinicia inmediatamente, puede ver:

```perl
Error en bind: Address already in use (98)
```

Esto se debe a que el puerto TCP queda en estado TIME_WAIT por unos segundos.

### Solución: espere unos segundos y vuelva a ejecutar

```bash
./exec/ThreadMirrorServer.out
```

### Limpieza

```bash
make clean
```

Esto elimina:

Archivos .o

Ejecutables .out
