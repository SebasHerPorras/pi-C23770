# Servidor Book-keeper (Tenedor)

## Introducción

El servidor **Book-keeper**(tenedor), es responsable de coordinar la comunicación entre los clientes HTTP y los servidores de figuras ASCII. Su objetivo principal es mantener una tabla de ruteo actualizada que relacione cada figura con el servidor que la almacena, y utilizar esa información para reenviar solicitudes y responder a los clientes.

---

## Almacenamiento de datos de otros servidores

El Book-keeper almacena una **tabla de ruteo**, que es una estructura tipo `map<string, string>` donde:

- La **clave** es el nombre de la figura (por ejemplo, `"gato"`, `"sombrilla"`).
- El **valor** es la dirección IP del servidor que ofrece dicha figura.

Esta tabla está protegida mediante un `std::mutex` llamado `tabla_mutex`, que asegura exclusión mutua cuando múltiples hilos acceden o modifican la tabla.

---

## Descubrimiento de servidores y obtención de figuras

El Tenedor realiza periódicamente un proceso de descubrimiento utilizando **UDP Broadcast**. El procedimiento es el siguiente:

1. **Envío de broadcast**: cada cierto intervalo (`BROADCAST_WAIT`), el Tenedor envía un mensaje UDP con el contenido `GET /servers` a múltiples direcciones de broadcast configuradas (por ejemplo, `127.0.0.255` para localhost).

2. **Recepción de respuestas**: los servidores de figuras que reciben esta solicitud responden con un mensaje en formato: NombreServidor | IP | figura1,figura2,figura3,...

3. **Procesamiento de la respuesta**:

- Se extrae el nombre del servidor, su IP y la lista de figuras.
- Cada figura se registra en la tabla de ruteo, asociándola a la IP del servidor que respondió.
- Esta operación está protegida por el mutex para evitar condiciones de carrera.

---

## Atención a clientes HTTP

El Tenedor escucha conexiones HTTP entrantes en el puerto `8080`. Cuando un cliente hace una solicitud como: GET /figure?name=gato

El Tenedor realiza los siguientes pasos:

1. **Parseo del nombre de la figura**.
2. **Búsqueda en la tabla de ruteo**:
   - Si la figura existe:
     - Se reenvía la solicitud al servidor correspondiente usando TCP.
     - El contenido recibido (la figura en ASCII) se encapsula en HTML y se devuelve al cliente.
   - Si la figura no está en la tabla:
     - Se puede devolver un error 404, o alternativamente una figura de error generada localmente o provista por el servidor (dependiendo de la implementación elegida).

---

## Concurrencia

- Las solicitudes HTTP se atienden en hilos separados mediante `std::thread::detach()`, lo que permite múltiples conexiones concurrentes.
- El acceso a la tabla de ruteo está sincronizado con un `mutex`.

---

## Consideraciones

- El descubrimiento se hace de forma periódica, lo que permite que nuevos servidores se integren dinámicamente.
- La figura de error puede ser generada por el propio servidor de figuras cuando no encuentra la solicitada.
- El Tenedor no almacena figuras directamente; actúa como intermediario.
