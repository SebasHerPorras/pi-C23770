#include "file_system.hpp"

FileSystem ::FileSystem() {

    this->filePath = "filesystem.dat";
    this->open_file();
    
}

FileSystem::FileSystem(bool init) {
    this->filePath = "filesystem.dat";
    if (init) {
        this->init_file();  // Inicializa el archivo con ceros
    } else {
        this->open_file();  // Abre el archivo existente
    }
}

void FileSystem::leer_root() {
    std::ifstream archivo("filesystem.dat", std::ios::binary);
    if (!archivo) {
        std::cerr << "Error al abrir el archivo." << std::endl;
        return;
    }

    // Leer el bloque raíz (bloque 0)
    archivo.seekg(0 * FULL_BLOCK_SIZE, std::ios::beg); // Bloque 0
    char bloque[FULL_BLOCK_SIZE] = {0};
    archivo.read(bloque, FULL_BLOCK_SIZE);

    if (archivo.gcount() != FULL_BLOCK_SIZE) {
        std::cerr << "Error: no se pudo leer el bloque completo." << std::endl;
        archivo.close();
        return;
    }

    std::cout << "\nContenido del bloque raíz (bloque 0):\n";

    // Recorrer el bloque inicial
    bool root_end = false;
    while (!root_end) {
        // Iterar sobre el bloque e imprimir el contenido
        for (int i = 0; i < BLOCK_SIZE; ++i) {
            // Cada 28 bytes (en posiciones 24-25 y 26-27)
            if (i % 28 == 24) {
                // Extraer el valor de 24-25 como un short
                short ocupado = *(short*)&bloque[i];
                std::cout << "Valor ocupado (24-25): " << ocupado;
                ++i;
            }
            if (i % 28 == 26) {
                // Extraer el valor de 26-27 como un short
                short puntero = *(short*)&bloque[i];
                std::cout << " Valor puntero a figura (26-27): " << puntero << "";
                ++i;
            }

            // Imprimir el contenido del bloque, representando '\0' como '.'
            if (bloque[i] == '\0') {
                std::cout << ".";  // Imprimir punto por el caracter nulo
            } else {
                std::cout << bloque[i];  // Imprimir el caracter tal cual
            }
            if ((i + 1) % 28 == 0) {
                std::cout << "\n";  // Nueva línea cada 28 bytes
            }
        }

        // Verificar si hay más bloques: Verificar si el bloque está ocupado y leer el puntero al siguiente bloque
        short ocupado = *(short*)&bloque[252];  // 252-253: ocupado (se espera que sea diferente de 0)
        short siguiente_puntero = *(short*)&bloque[254];  // 254-255: puntero al siguiente bloque

        std::cout << "\nValor ocupado (252-253): " << ocupado;
        std::cout << "\nValor puntero (254-255): " << siguiente_puntero << "\n";

        if (ocupado != 0) {
            // Si el bloque está ocupado, proceder a verificar el siguiente bloque
            if (siguiente_puntero != 0) {
                // Si el puntero al siguiente bloque no es 0, se mueve al siguiente bloque
                archivo.seekg(siguiente_puntero * FULL_BLOCK_SIZE, std::ios::beg);
                archivo.read(bloque, FULL_BLOCK_SIZE);

                if (archivo.gcount() != FULL_BLOCK_SIZE) {
                    std::cerr << "Error: no se pudo leer el siguiente bloque." << std::endl;
                    archivo.close();
                    return;
                }

                std::cout << "\nContenido del siguiente bloque (Bloque " << siguiente_puntero << "):\n";
            } else {
                root_end = true;  // Si el puntero es 0, hemos llegado al final
            }
        } else {
            root_end = true;  // Si el bloque no está ocupado, terminamos
        }
    }

    archivo.close();
    std::cout << "\n--- Fin de los bloques ---\n" << std::endl;
}




FileSystem::~FileSystem() {
    // Destructor implementation
    if (archivo.is_open()) {
        archivo.close();
    }
}

void FileSystem::open_file() {
    // Intenta abrir el archivo en modo lectura/escritura binario
    this->archivo.open("filesystem.dat", std::ios::in | std::ios::out | std::ios::binary);

    // Si el archivo no existe o no se puede abrir, créalo
    if (!this->archivo) {
        std::cerr << "El archivo no existe. Creando filesystem.dat..." << std::endl;
        
        // Crear un nuevo archivo vacío
        std::ofstream nuevo("filesystem.dat", std::ios::binary);
        if (!nuevo) {
            std::cerr << "Error al crear filesystem.dat." << std::endl;
            return;
        }
        nuevo.close();

        // Intenta abrir el archivo nuevamente
        this->archivo.open("filesystem.dat", std::ios::in | std::ios::out | std::ios::binary);
        if (!this->archivo) {
            std::cerr << "Error al abrir el archivo recién creado." << std::endl;
            return;
        }
    }

    // Asegura que el puntero de lectura esté al inicio
    this->archivo.seekg(0, std::ios::beg);
    std::cout << "Archivo filesystem.dat abierto correctamente.\n";
}



void FileSystem::close_file() {
    if (archivo.is_open()) {
        archivo.close();
    } 
}

void FileSystem::guardar_archivo(char* nombre_archivo, const char* datos) {
    std::fstream archivo("filesystem.dat", std::ios::in | std::ios::out | std::ios::binary);
    if (!archivo) {
        std::cerr << "No se pudo abrir filesystem.dat para guardar en el root.\n";
        return;
    }
    if (strlen(nombre_archivo) > 24) {
        std::cerr << "El nombre del archivo no puede exceder los 24 caracteres.\n";
        nombre_archivo[24] = '\0';
        std::cout << "Nombre truncado a 24: " << nombre_archivo << std::endl;
    }

    bool archivo_guardado = false;
    char entrada[FILE_SIZE_NAME] = {0};
    // char bloque[FULL_BLOCK_SIZE] = {0};

    std::streampos pos_actual = 0; // Empezamos en el bloque 0 (root)

    archivo.seekg(pos_actual, std::ios::beg);

    while (!archivo_guardado) {
        archivo.read(entrada, FILE_SIZE_NAME);
        short ocupado = *(short*)&entrada[24];

        if (ocupado == 0) {
            // Preparar entrada
            std::memset(entrada, 0, FILE_SIZE_NAME);
            std::memcpy(entrada, nombre_archivo, std::min<size_t>(24, strlen(nombre_archivo)));
            *(short*)&entrada[24] = 1;  // Marcado como ocupado

            // se necesita llamar a find_free_block() para obtener un bloque libre 
            // además de escribir el bloque de datos en la posición correspondiente
            short bloque_libre = this->find_free_block();
            this->escribir_bloque(bloque_libre, datos);  // Escribir los datos en el bloque libre
            *(short*)&entrada[26] = bloque_libre;  // Supuesto bloque de datos 

            archivo.seekp(pos_actual);
            archivo.write(entrada, FILE_SIZE_NAME);
            archivo.flush();

            //std::cout << "Archivo guardado en posición: " << pos_actual
            // << " la figura empieza en: " << bloque_libre << std::endl;
            archivo_guardado = true;
        } else {
            // Mover a la siguiente posición
            pos_actual += FILE_SIZE_NAME;

            if ((static_cast<size_t>(pos_actual) + 4) % FULL_BLOCK_SIZE == 0) {
                //std::cout << "Fin de bloque root. Revisando siguiente bloque...\n";

                // Leer punteros al final del bloque actual
                char punteros[4] = {0};
                archivo.seekg(pos_actual, std::ios::beg);
                archivo.read(punteros, 4);

                short next_block = *(short*)&punteros[2];

                if (next_block == 0) {
                    // Crear nuevo bloque root
                    int nuevo_bloque = this->find_free_block();
                    if (nuevo_bloque == -1) {
                        std::cerr << "No hay bloques disponibles.\n";
                        return;
                    }

                    *(short*)&punteros[2] = nuevo_bloque;

                    archivo.seekp(pos_actual);
                    archivo.write(punteros, 4);
                    archivo.flush();

                    pos_actual = nuevo_bloque * FULL_BLOCK_SIZE;
                    archivo.seekg(pos_actual, std::ios::beg);
                } else {
                    //std::cout << "yendo al bloque: " << next_block << std::endl;
                    pos_actual = next_block * FULL_BLOCK_SIZE;
                    archivo.seekg(pos_actual, std::ios::beg);
                }
            }
        }
    }

    archivo.close();
}

void FileSystem::formatear_sistema() {
    this->close_file();  // Cierra el archivo antes de formatear

    // Elimina el archivo si existe
    if (std::remove("filesystem.dat") != 0) {
        std::cerr << "Advertencia: No se pudo eliminar filesystem.dat (puede que no exista)." << std::endl;
    }

}

void FileSystem::init_file() {
    // Cerrar el archivo si está abierto
    if (this->archivo.is_open()) {
        this->archivo.close();
    }

    // std::cout << "Inicializando filesystem.dat con ceros...\n";

    // Crear un nuevo archivo truncado y escribir NULL en todo el espacio
    std::ofstream archivo("filesystem.dat", std::ios::binary | std::ios::trunc);
    if (!archivo) {
        std::cerr << "Error al crear filesystem.dat." << std::endl;
        return;
    }

    // Crear un buffer lleno de '\0' (inicializa todo en ceros)
    char buffer[FULL_BLOCK_SIZE] = {0};  // Usamos FULL_BLOCK_SIZE
    char buffer_root[FULL_BLOCK_SIZE] = {0};  // Buffer para la raíz

    // Modificar los últimos 4 bytes (252-255) para que contengan 0 en short
    short cero = 0;  // 'short' 0 es 0x00 0x00
    short uno = 1;  // 'short' 1 es 0x00 0x01

    // Escribir el primer 'short' (bytes 252-253) como 0
    std::memcpy(&buffer[252], &cero, sizeof(cero));
    // Escribir el segundo 'short' (bytes 254-255) como 1
    std::memcpy(&buffer_root[252], &uno, sizeof(uno));
    
    // Escribir el segundo 'short' (bytes 254-255) como 0
    std::memcpy(&buffer[254], &cero, sizeof(cero));

    // Escribir todos los bloques en el archivo
    archivo.write(buffer_root, FULL_BLOCK_SIZE);  // Escribir el primer bloque (raíz)
    for (int i = 1; i < BLOCK_COUNT; i++) {

        archivo.write(buffer, FULL_BLOCK_SIZE);
    }

    std::streampos pos_actual = 252;  // Guardar la posición actual
    this->archivo.seekp(pos_actual, std::ios::beg);  // Mover el puntero al bloque actual


    archivo.close();
    // std::cout << "filesystem.dat inicializado correctamente.\n";

    // Reabrir el archivo para operaciones futuras
    this->archivo.open("filesystem.dat", std::ios::in | std::ios::out | std::ios::binary);
    if (!this->archivo) {
        std::cerr << "Error al abrir filesystem.dat tras inicialización." << std::endl;
    }
    this->poner_ceros();  // Llenar el archivo con ceros
}

int FileSystem::find_free_block() {
    std::fstream archivo("filesystem.dat", std::ios::in | std::ios::out | std::ios::binary);
    if (!archivo) {
        std::cerr << "Error al abrir filesystem.dat para buscar bloques libres.\n";
        return -1;
    }

    // std::cout << "\n[find_free_block] Buscando bloque libre...\n";

    char bloque[FULL_BLOCK_SIZE] = {0};
    short ocupado;
    int bloque_libre = -1;

    for (int i = 0; i < BLOCK_COUNT; ++i) {
        archivo.seekg(i * FULL_BLOCK_SIZE, std::ios::beg);
        archivo.read(bloque, FULL_BLOCK_SIZE);

        std::memcpy(&ocupado, &bloque[252], sizeof(short));

        // std::cout << "Bloque " << i << " ocupado = " << ocupado << std::endl;

        if (ocupado == 0) {
            // Marcar como ocupado
            short uno = 1;
            std::memcpy(&bloque[252], &uno, sizeof(short));

            // Puntero al siguiente bloque = 0
            short cero = 0;
            std::memcpy(&bloque[254], &cero, sizeof(short));

            // Reescribir el bloque
            archivo.seekp(i * FULL_BLOCK_SIZE, std::ios::beg);
            archivo.write(bloque, FULL_BLOCK_SIZE);
            archivo.flush();

            //std::cout << "Bloque libre encontrado y marcado como ocupado: " << i << std::endl;

            bloque_libre = i;
            break;
        }
    }

    archivo.close();

    if (bloque_libre == -1) {
        std::cout << "No se encontró ningún bloque libre.\n";
    }

    return bloque_libre;
}


void FileSystem::poner_ceros() {
    // Abrir el archivo en modo lectura/escritura binaria
    this->archivo.seekg(0, std::ios::beg);  // Mover el puntero al inicio del archivo

    char bloque[FULL_BLOCK_SIZE] = {0};  // Buffer para leer un bloque de 256 bytes
    short cero = 0;  // Valor 0 en tipo short
    short uno = 1;  // Valor 1 en tipo short
    std::memcpy(&bloque[252], &uno, sizeof(uno));  // Escribir 1 en los bytes 252-253

    this->archivo.write(bloque, FULL_BLOCK_SIZE);  // Escribir el bloque inicializado
    std::memset(&bloque[252], cero, sizeof(short));  // Llenar los bytes 252-253 con ceros
    std::memset(&bloque[254], cero, sizeof(short));  // Llenar los bytes 254-255 con ceros
    // Escribir ceros en todo el archivo
    for (int i = 0; i < BLOCK_COUNT; ++i) {
        this->archivo.write(bloque, FULL_BLOCK_SIZE);  // Escribir el bloque lleno de ceros
    }
        short ocupado;
    archivo.seekg(252, std::ios::beg);
    archivo.read(reinterpret_cast<char*>(&ocupado), sizeof(ocupado));
    // std::cout << "\n\nOcupado: " << ocupado << std::endl;


    this->archivo.seekg(0, std::ios::beg);  // Mover el puntero al inicio del archivo para la próxima lectura
}

void FileSystem::escribir_bloque(int bloque_num, const char* datos) {
    std::fstream archivo("filesystem.dat", std::ios::in | std::ios::out | std::ios::binary);
    if (!archivo) {
        std::cerr << "Error al abrir filesystem.dat para escribir el bloque.\n";
        return;
    }

    int cantidad_bloques = strlen(datos) / BLOCK_SIZE + 1;  // Calcular la cantidad de bloques necesarios

    // Calcular la posición del bloque a escribir de manera inicial
    archivo.seekp(bloque_num * FULL_BLOCK_SIZE, std::ios::beg);

    short ocupado = 1;  // Valor inicial de ocupado
    short puntero = 0;  // Valor inicial del punteros

   for (int i = 1; i <= cantidad_bloques; ++i) {
        char buffer[BLOCK_SIZE] = {0};
        size_t bytes_restantes = strlen(datos);
        size_t bytes_a_copiar = std::min(bytes_restantes, (size_t)BLOCK_SIZE);
        std::memcpy(buffer, datos, bytes_a_copiar);

        archivo.write(buffer, BLOCK_SIZE);  // ✅ Rellenado correctamente
        archivo.write(reinterpret_cast<char*>(&ocupado), sizeof(ocupado));
        
        puntero = (i < cantidad_bloques) ? this->find_free_block() : 0;  // Si es el último, puntero = 0
        archivo.write(reinterpret_cast<char*>(&puntero), sizeof(puntero));

        datos += BLOCK_SIZE;  // Avanzar en los datos
        if (puntero > 0) {
        archivo.seekp(puntero * FULL_BLOCK_SIZE, std::ios::beg);
        }
    }
    archivo.close();
}

char* FileSystem::find_figura(const std::string& nombre_figura) {
    short puntero = this->find_figura_post(nombre_figura);
    if (puntero == -1) {
        std::cerr << "No se encontró la figura: " << nombre_figura << std::endl;

        return this->find_figura_error();  // Retorna nullptr si no se encuentra la figura 
        // return nullptr;  // Retorna nullptr si no se encuentra la figura
    }
    char* figura;
    figura = this->find_figura_char(puntero);  // Llama a la función para encontrar la figura      
    return figura;  // Retorna el puntero a la figura encontrada
    
}

short FileSystem::find_figura_post(const std::string& nombre_figura) {
    std::ifstream archivo("filesystem.dat", std::ios::binary);
    if (!archivo) {
        std::cerr << "Error al abrir el archivo." << std::endl;
        return -1; // Retorna -1 si no se puede abrir el archivo
    }

    // Leer el bloque raíz (bloque 0)
    archivo.seekg(0 * FULL_BLOCK_SIZE, std::ios::beg); // Bloque 0
    char bloque[FULL_BLOCK_SIZE] = {0};
    archivo.read(bloque, FULL_BLOCK_SIZE);

    if (archivo.gcount() != FULL_BLOCK_SIZE) {
        std::cerr << "Error: no se pudo leer el bloque completo." << std::endl;
        archivo.close();
        return -1;
    }

    bool root_end = false;
    short puntero_figura = -1; // Inicialmente, no se ha encontrado ningún puntero
    short siguiente_puntero = 0; // Inicializar el puntero al siguiente bloque
    while (!root_end) {
        // Iterar sobre el bloque e imprimir el contenido de cada archivo (28 bytes)
        for (int i = 0; i < BLOCK_SIZE; i += 28) {
            // Leer el nombre del archivo de 24 bytes (arreglo de char)
            char nombre_archivo[25] = {0}; // 24 bytes para el nombre + 1 para '\0'
            memcpy(nombre_archivo, &bloque[i], 24);

            // Asegurarse de que la cadena esté bien terminada
            nombre_archivo[24] = '\0';  // Asegura que la cadena termine con un carácter nulo

            // Buscar coincidencia con el nombre proporcionado
            if (nombre_figura == nombre_archivo) {
                // Si se encuentra el nombre, extraemos el puntero a la figura en los bytes 26-27
                puntero_figura = *(short*)&bloque[i + 26];
                std::cout << "Figura encontrada: " << nombre_figura << std::endl;
                std::cout << "Puntero a la figura: " << puntero_figura << std::endl;
                archivo.close();
                return puntero_figura;  // Retornar el puntero a la figura, o -1 si no se encontró
            }
            if((*(short*)&bloque[i + 26]) == 0) {
                // Si el puntero es 0, significa que no hay más figuras en este bloque
                // std::cout << "soy el archivo num: " << i/28 << " del bloque: " << siguiente_puntero << std::endl;
            }
        }
        siguiente_puntero = *(short*)&bloque[254];  // 254-255: puntero al siguiente bloque

        if (siguiente_puntero != 0) {
            // Mover al siguiente bloque si el puntero es válido
            archivo.seekg(siguiente_puntero * FULL_BLOCK_SIZE, std::ios::beg);
            archivo.read(bloque, FULL_BLOCK_SIZE);

            if (archivo.gcount() != FULL_BLOCK_SIZE) {
                std::cerr << "Error: no se pudo leer el siguiente bloque." << std::endl;
                archivo.close();
                return -1;
            }
            std::cout << "\nLeyendo siguiente bloque de raiz: " << siguiente_puntero << std::endl;
        } else {
            root_end = true;  // No hay más bloques, hemos llegado al final
        }
    }

    archivo.close();
    return puntero_figura;  // Retornar el puntero a la figura, o -1 si no se encontró
}

char* FileSystem::find_figura_chara(short puntero_figura) {
    std::ifstream archivo("filesystem.dat", std::ios::binary);
    if (!archivo) {
        std::cerr << "Error al abrir el archivo." << std::endl;
        return nullptr;  // Retorna nullptr si no se puede abrir el archivo
    }

    // Usar un vector para acumular los caracteres de la figura
    std::vector<char> figura;

    // Comenzar a leer desde el bloque donde se encuentra la figura
    archivo.seekg(puntero_figura * FULL_BLOCK_SIZE, std::ios::beg);
    char bloque[FULL_BLOCK_SIZE] = {0};
    archivo.read(bloque, FULL_BLOCK_SIZE);

    if (archivo.gcount() != FULL_BLOCK_SIZE) {
        std::cerr << "Error: no se pudo leer el bloque completo." << std::endl;
        archivo.close();
        return nullptr;  // Si no se puede leer el bloque, retornar nullptr
    }

    bool figura_end = false;

    while (!figura_end) {
        // Extraer los datos hasta el byte 252 (contenido de la figura)
        for (int i = 0; i < 252; ++i) {
            if (bloque[i] == '\0') {
                figura.push_back('.');  // Si es '\0', agregar un punto para la figura
            } else {
                figura.push_back(bloque[i]);  // Agregar el carácter tal cual
            }
        }

        // Verificar si la figura continúa en el siguiente bloque (bytes 254-255)
        short siguiente_puntero = *(short*)&bloque[254];  // Puntero al siguiente bloque

        if (siguiente_puntero != 0) {
            // Si hay más bloques, mover al siguiente bloque
            archivo.seekg(siguiente_puntero * FULL_BLOCK_SIZE, std::ios::beg);
            archivo.read(bloque, FULL_BLOCK_SIZE);

            if (archivo.gcount() != FULL_BLOCK_SIZE) {
                std::cerr << "Error: no se pudo leer el siguiente bloque." << std::endl;
                archivo.close();
                return nullptr;  // Si no se puede leer el siguiente bloque, retornar nullptr
            }

            std::cout << "\nLeyendo siguiente bloque: " << siguiente_puntero << std::endl;
        } else {
            figura_end = true;  // La figura ha terminado
        }
    }

    archivo.close();

    // Convertir el vector de char a un arreglo dinámico de char*
    char* figura_char = new char[figura.size() + 1];  // +1 para el terminador nulo '\0'
    std::copy(figura.begin(), figura.end(), figura_char);
    figura_char[figura.size()] = '\0';  // Asegurar que la cadena esté terminada en '\0'

    return figura_char;  // Retornar el puntero a la figura
}
char* FileSystem::find_figura_char(short puntero_figura) {
    std::ifstream archivo("filesystem.dat", std::ios::binary);
    if (!archivo) {
        std::cerr << "Error al abrir el archivo." << std::endl;
        return nullptr;  // Retorna nullptr si no se puede abrir el archivo
    }

    // Usar un vector para acumular los caracteres de la figura
    std::vector<char> figura;

    // Comenzar a leer desde el bloque donde se encuentra la figura
    archivo.seekg(puntero_figura * FULL_BLOCK_SIZE, std::ios::beg);
    char bloque[FULL_BLOCK_SIZE] = {0};
    archivo.read(bloque, FULL_BLOCK_SIZE);

    if (archivo.gcount() != FULL_BLOCK_SIZE) {
        std::cerr << "Error: no se pudo leer el bloque completo." << std::endl;
        archivo.close();
        return nullptr;  // Si no se puede leer el bloque, retornar nullptr
    }

    bool figura_end = false;

    std::cout << "\nLeyendo el primer bloque de la figura: " << puntero_figura << std::endl;
    while (!figura_end) {
        // Extraer los datos hasta el byte 252 (contenido de la figura)
        for (int i = 0; i < 252; ++i) {
            if (bloque[i] == '\0') {
                figura_end = true;  // Si encontramos '\0', terminar
                break;
            }
            figura.push_back(bloque[i]);  // Agregar el carácter tal cual
        }

        // Verificar si la figura continúa en el siguiente bloque (bytes 254-255)
        short siguiente_puntero = *(short*)&bloque[254];  // Puntero al siguiente bloque

        if (siguiente_puntero != 0 && !figura_end) {
            // Si hay más bloques, mover al siguiente bloque
            archivo.seekg(siguiente_puntero * FULL_BLOCK_SIZE, std::ios::beg);
            archivo.read(bloque, FULL_BLOCK_SIZE);

            if (archivo.gcount() != FULL_BLOCK_SIZE) {
                std::cerr << "Error: no se pudo leer el siguiente bloque." << std::endl;
                archivo.close();
                return nullptr;  // Si no se puede leer el siguiente bloque, retornar nullptr
            }

            std::cout << "Leyendo siguiente bloque: " << siguiente_puntero << std::endl;
        } else {
            figura_end = true;  // La figura ha terminado
        }
    }

    std::cout << "Se terminó de leer la figura" << std::endl;

    archivo.close();

    // Convertir el vector de char a un arreglo dinámico de char*
    char* figura_char = new char[figura.size() + 1];  // +1 para el terminador nulo '\0'
    std::copy(figura.begin(), figura.end(), figura_char);
    figura_char[figura.size()] = '\0';  // Asegurar que la cadena esté terminada en '\0'

    return figura_char;  // Retornar el puntero a la figura
}




void FileSystem::cargar_figuras_base() {
    char arbol[] = "arbol_navidad";  // Usar arreglo de char
    const char* arbol_navidad = 
        "        **        \n"
        "       ****       \n"
        "      ******      \n"
        "     ********     \n"
        "    **********    \n"
        "   ************   \n"
        "  **************  \n"
        "        **        \n"
        "        **        \n";
    
    char gato[] = "gato";  // Usar arreglo de char
    const char* gato_figura = 
        "            ＿＿\n"
        "　　　　　 ´ ＞　　フ  \n"
        "　　　　　| 　_　_ ﾉ \n"
        "　 　　　／`ミ _w ノ \n"
        "　　 　 /　　　 　|\n"
        "　　　 /　 ヽ　　 ﾉ\n"
        "　 　 │　　 |　|　|\n"
        "　／￣|　　 |　|　|\n"
        "　| (￣ヽ＿_ヽ_)__)\n"
        "　＼二つ\n";

    char barco[] = "barco";  // Usar arreglo de char
    const char* barco_figura =
        "                  ~.\n"
        "           Ya..._|__..aab     .   .\n"
        "            Y88a  Y88o  Y88a   (     )\n"
        "             Y88b  Y88b  Y88b   `.oo'\n"
        "             :888  :888  :888  ( (`-'\n"
        "    .---.    d88P  d88P  d88P   ..\n"
        "   / .-._)  d8P'\"\"\"|\"\"\"'-Y8P      ..   barco\n"
        "  ( (`._) .-.  .-. |.-.  .-.  .-.   ) )\n"
        "   \\ `---( O )( O )( O )( O )( O )-' /\n"
        "    `.    `-'  `-'  `-'  `-'  `-'  .' CJ\n"
        "~~~~~~~~~~~~~~~\n";

    char sombrilla[] = "sombrilla";  // Usar arreglo de char
    const char* sombrilla_figura =
        "                      ==#==                       \n"
        "                  .=*#######*=.                   \n"
        "             =*###################*=              \n"
        "          =####+.               .+####=           \n"
        "       .###*=                       =*###.        \n"
        "     .*##*                             *##*.      \n"
        "    *##*                                 *##*     \n"
        "  .###                                     *##.   \n"
        " .###-                                     :###:  \n"
        " ###############################################. \n"
        "                       *#+                        \n"
        "                       +#+                        \n"
        "                       +#+                        \n"
        "                       +#+                        \n"
        "                       +#+                        \n"
        "                       +#+                        \n"
        "                       +#+                        \n"
        "                       +#+                        \n"
        "                       +#+                        \n"
        "                       +#+                        \n"
        "                       +#+                        \n"
        "                 ###   *#+                        \n"
        "                 +##*=*##:                        \n"
        "                  .*###+                          \n";

    std::vector<char*> figuras = {arbol, gato, barco, sombrilla};
    std::vector<const char*> figuras_datos = {arbol_navidad, gato_figura, barco_figura, sombrilla_figura};
    for (size_t i = 0; i < figuras.size(); ++i) {
        this->guardar_archivo(figuras[i], figuras_datos[i]);
    }
    std::cout << "Figuras cargadas correctamente en el sistema de archivos.\n";
}
char* FileSystem::find_figura_error() {
    // Definir una figura de error en un buffer dinámico
    char* figura_de_error = new char[400]; // Asignar suficiente espacio para la figura

    // Llenar la figura con el diseño de error
    const char* error = 
        "           ______\n\
          /      \\\n\
         |  X  X  |\n\
         |    ^   |  \n\
         |   ---  |   \n\
          \\______/\n\
            |  |\n\
     _______|  |_______\n\
    |      ERROR       |\n\
    |  SYSTEM FAILURE  |\n\
    |__________________|\n\
       __/        \\__\n\
      /              \\\n\
     /________________\\\n";

    // Copiar el contenido del error al buffer
    std::strcpy(figura_de_error, error);
    //std::cout << figura_de_error << std::endl;

    // Retornar el puntero a la figura de error
    return figura_de_error;
}

