#include <iostream>
#include <string>
#include "file_system.hpp"

int main() {
    // FileSystem fs; // Inicializa el sistema de archivos
    FileSystem fs(true); // Inicializa el sistema de archivos y lo formatea
    std::cout << "\n";
    fs.cargar_figuras_base(); // Carga las figuras base en el sistema de archivos
    fs.cargar_figuras_base(); // Carga las figuras base en el sistema de archivos nuevamente
    fs.cargar_figuras_base(); // Carga las figuras base en el sistema de archivos nuevamente
    fs.cargar_figuras_base(); // Carga las figuras base en el sistema de archivos nuevamente
    fs.leer_root(); // Lee el bloque 0 para verificar el contenido

    std::string figura = "gato"; // Nombre de la figura a buscar
    
    std::cout << "buscar a " << figura << std::endl;
    char* hola =fs.find_figura(figura); // Busca la figura "sombrilla" en el sistema de archivos
    std::cout << "la figura " <<figura<<" es la siguiente:\n"<< std::endl;
    std::cout << hola << std::endl; // Imprime la figura encontrada

    std::cout << "buscar a perro" << std::endl;
    hola = fs.find_figura("perro"); // Busca la figura "perro" en el sistema de archivos
    std::cout << hola << std::endl;

    hola = fs.find_figura("sombrilla"); // Busca la figura "sombrilla" en el sistema de archivos
    std::cout << "la figura " << "sombrilla" << " es la siguiente:\n" << std::endl;
    std::cout << hola << std::endl; // Imprime la figura encontrada

    hola = fs.find_figura("arbol_navidad"); // Busca la figura "arbol_navidad" en el sistema de archivos
    std::cout << "la figura " << "arbol_navidad" << " es la siguiente:\n" << std::endl;
    std::cout << hola << std::endl; // Imprime la figura encontrada

    hola = fs.find_figura("barco"); // Busca la figura "barco" en el sistema de archivos
    std::cout << "la figura " << "barco" << " es la siguiente:\n" << std::endl;
    std::cout << hola << std::endl; // Imprime la figura encontrada

    // 
    

    return 0;
}
