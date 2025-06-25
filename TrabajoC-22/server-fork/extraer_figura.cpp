#include "file_system.hpp"
#include <iostream>

// Programa de ejecución local para extraer una figura específica del sistema de archivos
int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "Uso: " << argv[0] << " <nombre_figura>\n";
        return 1;
    }

    std::string nombre = argv[1];
    FileSystem fs;
    char *figura = fs.find_figura(nombre);

    if (figura)
    {
        std::cout << figura << std::endl;
        delete[] figura;
    }
    else
    {
        std::cerr << "Figura '" << nombre << "' no encontrada.\n";
    }

    return 0;
}
