#include "file_system.hpp"
#include <iostream>
#include <vector>

// Programa de ejecución local para listar las figuras almacenadas en el .dat
int main()
{
    FileSystem fs;
    std::vector<std::string> figuras = fs.get_figuras();

    std::cout << "Figuras almacenadas en el sistema:\n";
    for (const std::string &figura : figuras)
    {
        std::cout << " - " << figura << "\n";
    }

    return 0;
}
