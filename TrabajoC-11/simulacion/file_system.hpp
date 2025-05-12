#include <string>
#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdio>  // Para remove()
#include <vector>


#ifndef FILE_SYSTEM_HPP
#define FILE_SYSTEM_HPP

const int BLOCK_SIZE = 252;
const int FULL_BLOCK_SIZE = 256; // Tamaño total del bloque (incluyendo cabecera)
const int BLOCK_COUNT = 1024; // Número total de bloques en el sistema de archivos
const int FILE_SIZE_NAME = 28; // Tamaño máximo del nombre del archivo
const int FILE_ENTRY_SIZE = 28;  // Tamaño de la entrada de archivo



class FileSystem {
  public:
    FileSystem();
    FileSystem(bool);
    ~FileSystem();
    
    void close_file();
    void escribir_bloque(int bloque_num, const char* datos);
    void leer_root();
    void write_file(char* path_file, const char* datos);
    void formatear_sistema();
    void init_file();
    void guardar_archivo( char* nombre_archivo, const char* datos);
    void poner_ceros();
    void cargar_figuras_base();
    int find_free_block(); // Método para encontrar un bloque libre en el sistema de archivos
    short find_figura_post(const std::string& nombre_figura);
    char* find_figura(const std::string& nombre_figura);
    char* find_figura_char(short bloque);
    char* find_figura_chara(short bloque);
    char* find_figura_error();
    std::vector<std::string> get_figuras();
  private:
    std::string filePath;
    std::fstream archivo;


};

#endif // FILE_SYSTEM_HPP

#include <fstream>
#include <cstring>



