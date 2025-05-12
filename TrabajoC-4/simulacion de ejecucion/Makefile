# Definir el compilador
CC = gcc
CXX = g++

# Definir los flags que deseas usar
CFLAGS = -Wall -Wextra -fopenmp
CXXFLAGS = -Wall -Wextra -fopenmp

# Definir los directorios de origen y salida
SRC_DIR = src
OBJ_DIR = build
EXE_FILE = simulacion

# Encontrar todos los archivos fuente .c y .cpp en el directorio src
SRC_C = $(wildcard $(SRC_DIR)/*.c)
SRC_CPP = $(wildcard $(SRC_DIR)/*.cpp)

# Generar los archivos objeto correspondientes en el directorio build
OBJ_C = $(SRC_C:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
OBJ_CPP = $(SRC_CPP:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

# Combinar todos los archivos objeto
OBJ = $(OBJ_C) $(OBJ_CPP)

# Regla por defecto (compilar todo)
all: $(EXE_FILE)

# Regla para compilar el ejecutable
$(EXE_FILE): $(OBJ)
	$(CXX) $(OBJ) -o $@ $(CXXFLAGS)

# Regla para compilar los archivos .c
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) -c $< -o $@ $(CFLAGS)

# Regla para compilar los archivos .cpp
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) -c $< -o $@ $(CXXFLAGS)

# Crear el directorio de objetos si no existe
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Limpiar los archivos generados
clean:
	rm -rf $(OBJ_DIR) $(EXE_FILE)

# Ejecutar el programa
run: $(EXE_FILE)
	./$(EXE_FILE)
