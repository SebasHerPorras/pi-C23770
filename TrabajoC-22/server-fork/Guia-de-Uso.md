# Proyecto: Descubrimiento Tenedor/Figuras

---

## Estructura del Proyecto

```
.
├── exec/                    # Ejecutables generados tras compilación
├── obj/                     # Archivos objeto intermedios
├── figura_discovery.cpp    # Código fuente del servidor de figuras
├── tenedor_discovery.cpp   # Código fuente del tenedor (cliente descubridor + HTTP)
├── Socket.h / Socket.cpp   # Implementación de clase Socket basada en VSocket
├── VSocket.h / VSocket.cpp # Clase base VSocket para comunicación en red
├── file_system.hpp / .cpp  # Módulo para manejo y búsqueda de figuras ASCII
├── Makefile                # Script de compilación con targets específicos
└── README.md               # Este manual
```

---

## Compilación

Se utiliza `make` con los siguientes objetivos:

- `make` → Compila todos los ejecutables
- `make figuras` → Solo compila el servidor de figuras
- `make tenedor` → Solo compila el tenedor
- `make clean` → Elimina todos los objetos y ejecutables generados

**Requisitos:**

- Sistema Linux
- Compilador `g++`
- Permisos de red adecuados (para usar sockets UDP y TCP)

---

## Flujo del Sistema

1. El **Tenedor** inicia un hilo que periódicamente (cada 120 segundos):
   - Envía un mensaje UDP tipo `"GET /servers"` a direcciones de broadcast.
   - Escucha respuestas por 5 segundos.

2. El **Servidor de Figuras**:
   - Está escuchando en el puerto UDP 5353.
   - Al recibir `"GET /servers"`, responde con un mensaje que contiene:

     ```
     <nombre_servidor> | <ip> | <lista_figuras>
     ```

   - También escucha en el puerto TCP 8081 para solicitudes de figuras.

3. El **Tenedor**:
   - Recoge los nombres de las figuras anunciadas por cada servidor y construye una tabla de ruteo.
   - También ejecuta un servidor HTTP en el puerto 8080.

4. Un **cliente externo (curl, navegador, etc.)** puede enviar solicitudes HTTP al Tenedor para obtener figuras específicas.

---

## Ejecución

### 1. Levantar el servidor de figuras en una terminal

```bash
 make run_figuras
```

- Carga figuras desde el sistema de archivos (almacenadas localmente).
- Responde a descubrimientos por UDP en el puerto **5353**.
- Atiende solicitudes TCP en el puerto **8081**.

### 2. Levantar el tenedor en otra terminal

```bash
 make run_tenedor
```

- Envía mensajes de descubrimiento por UDP a `127.0.0.255` (o IP de broadcast real).
- Escucha en el puerto **8080** como servidor HTTP.
- Cuando un cliente solicita una figura por HTTP, busca la IP en su tabla de ruteo y contacta al servidor de figuras vía TCP.

---

## Prueba usando navegador

Una vez ambos programas estén corriendo, puede simular un cliente desde otra terminal con:

- Abra el navegador y escriba rutas como:
<localhost:8080/figure?name=gato>
<localhost:8080/figure?name=sombrilla>
<localhost:8080/figure?name=barco>
- El navegador enviará una solicitud HTTP al Tenedor.
- El Tenedor buscará la IP asociada a "gato" en su tabla de ruteo.
- Contactará al servidor de figuras por TCP y pedirá la figura con `GET /figure/gato`.

- También puede listar las figuras conocidas con:
<localhost:8080/list>

## Prueba usando `curl` en otra terminal

```bash
curl http://localhost:8080/figure?name=gato
```

## Prueba levantando cliente en terminal

```bash
 ./exec/MirrorClient.out gato
```

---

## Comportamiento esperado del sistema

- Si se accede a `http://localhost:8080/figure?name=algoquenoexiste`, y esa figura no está en la tabla de ruteo, se devuelve una **respuesta 404 Not Found**.
- Si ocurre un error de red al contactar al servidor de figuras, también se devuelve una **404 Not Found**.
- En esos casos, el cliente verá una página vacía o error según el navegador.
- **Cuando el servidor de figuras responde, pero la figura no existe, se envía una figura de error ASCII.**
- Si se accede a una URL inválida como `http://localhost:8080/list` sin la ruta exacta, también se devolverá una **400 Bad Request**. Esto es **esperado** y se hace para mantener el formato de las rutas estrictamente controlado.

---

Nota: si `/list` no coincide exactamente con la lógica de parseo esperada, es posible que devuelva 400. Asegúrese de escribir la ruta tal como fue definida.

