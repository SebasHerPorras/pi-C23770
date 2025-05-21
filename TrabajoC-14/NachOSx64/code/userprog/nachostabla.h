#ifndef NACHOSTABLA_H
#define NACHOSTABLA_H

#include "bitmap.h"
#include "openfile.h" // Necesario para OpenFile*
#include <iostream>

/**
 * Clase para gestionar la tabla de archivos abiertos en NachOS.
 */
class NachosOpenFilesTable {
  #define dbgFileSys 'f'

public:
    NachosOpenFilesTable();      // Inicializa estructuras internas
    ~NachosOpenFilesTable();     // Libera recursos

    int Open(OpenFile* file);  // Después
    int Close(int nachosHandle);     // Cierra un archivo abierto
    bool isOpened(int nachosHandle) const; // Verifica si está abierto
    int getUnixHandle(int nachosHandle) const; // Devuelve Unix handle
    OpenFile* getOpenFile(int nachosHandle);   // Devuelve puntero a OpenFile

    void addThread(int);              // Aumenta contador de hilos
    void delThread();             // Reduce contador de hilos
    void Print() const;           // Imprime contenido para depuración
    int AddSocket(int sockfd); // Agrega un socket a la tabla

private:
    int maxFiles;   // Máximo número de archivos abiertos
    OpenFile** openFiles;             // Array de punteros a OpenFile
    BitMap* openFilesMap;            // Mapa de bits de archivos abiertos
    int* threadsPerFile;             // Hilos que usan cada archivo
    int usage;                       // Número total de hilos que usan la tabla
};

#endif // NACHOSTABLA_H
