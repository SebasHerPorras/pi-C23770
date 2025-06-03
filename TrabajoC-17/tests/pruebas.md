# ¿Cómo realizar tests?

Para realizar pruebas del sistema `nachos`, es necesario que el cliente se comunique con el servidor desarrollado en la etapa anterior del proyecto. Por ello, **el servidor debe estar ejecutándose** antes de lanzar el cliente desde `nachos`.

## 1. Levantar el servidor

Debes ejecutar los siguientes comandos desde la carpeta `server-cliente`:

```bash
make clean ; make ; ./exec/ThreadMirrorServer.out
```

Este comando limpia, compila y ejecuta el servidor, el cual queda escuchando en el **puerto 1234** y estará listo para recibir solicitudes del cliente.

## 2. Ejecutar el cliente desde Nachos

### Compilar Nachos

Desde la carpeta `userprog`, compila el ejecutable de `nachos`:

```bash
make
```

### Ejecutar el cliente

Una vez compilado, ejecuta el cliente con el siguiente comando:

```bash
./nachos -x ../test/cliente
```

Durante la ejecución, el programa solicitará los siguientes datos:

- La **IP** del dispositivo que ejecutó el servidor
- El **puerto**, que debe ser `1234`
- El **nombre de la figura** que desea extraer

## 3. Figuras disponibles

Puedes solicitar cualquiera de las siguientes figuras:

- `gato`
- `barco`
- `sombrilla`
- `arbol_navidad`
