# Proyecto Integrador - CI0123 Redes y Sistemas Operativos  

## Etapa 2 - Servidor de Figuras ASCII

## Descripción general

Este proyecto implementa un **servidor multihilo** que atiende solicitudes TCP por parte de clientes o tenedores de figuras ASCII. El servidor ofrece las siguientes funcionalidades, siguiendo un protocolo previamente definido:

- Anuncia su disponibilidad a través de un mensaje simulado (`broadcast`).
- Responde solicitudes de descubrimiento (`GET /servers`) indicando qué figuras ofrece.
- Atiende peticiones específicas (`GET /figure/{nombre}`) devolviendo la figura en texto plano.
- Maneja una solicitud de apagado (`Shutdown ServidorA`) y termina la ejecución si aplica.

El sistema incluye un **servidor**, un **cliente simple**, y un **sistema de archivos simulado** basado en bloques binarios para almacenar las figuras.

---

## Archivos

### Lógica principal

- `ThreadMirrorServer.cc`: Servidor multihilo que maneja conexiones, interpreta peticiones y responde según el protocolo definido.
- `MirrorClient.cc`: Cliente simple que se conecta al servidor, solicita una figura y la imprime.

### Comunicación en red

- `VSocket.hpp / VSocket.cpp`: Clase base abstracta para sockets (TCP/UDP, IPv4/IPv6).
- `Socket.hpp / Socket.cpp`: Implementación concreta para sockets stream TCP, extiende `VSocket`.

### Sistema de archivos simulado

- `file_system.hpp / file_system.cpp`: Simula un sistema de archivos tipo FAT con bloques binarios. Permite:
  - Guardar figuras en un archivo binario por bloques.
  - Buscar y extraer figuras almacenadas.
  - Cargar figuras base predefinidas (gato, barco, sombrilla, árbol).
  - Leer el contenido del bloque raíz.

---

## Protocolo

- Se adjunta pdf con el protocolo predefinido por el grupo.

## Archivo filesystem.dat

- **Ubicación:** generado automáticamente al ejecutar el servidor.
- **Propósito:** almacena las figuras ASCII en un formato binario estructurado por bloques.
- **Estructura:**  
  - Cada bloque tiene 256 bytes:  
    - 252 bytes de contenido  
    - 2 bytes para estado de ocupación  
    - 2 bytes para el puntero al siguiente bloque
  - El bloque raíz contiene referencias a las figuras guardadas.

---

## Créditos

Trabajo realizado por:

-Sebas Hernandez Porras C23770

-Fabricio Aguero: C20097

-Jose Guerra Rodríguez C33510
