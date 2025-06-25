# Compilador
CXX := g++
CXXFLAGS := -g -c -std=c++11

# Directorios
SRC := .
OBJDIR := obj
BINDIR := exec

VPATH := $(SRC)

$(shell mkdir -p $(OBJDIR) $(BINDIR))

# Objetos por ejecutable
OBJS_CLIENT := $(OBJDIR)/MirrorClient.o $(OBJDIR)/VSocket.o $(OBJDIR)/Socket.o
OBJS_FIGURA := $(OBJDIR)/figura_discovery.o $(OBJDIR)/VSocket.o $(OBJDIR)/Socket.o $(OBJDIR)/file_system.o
OBJS_TENEDOR := $(OBJDIR)/tenedor_discovery.o $(OBJDIR)/VSocket.o $(OBJDIR)/Socket.o

OBJS_LISTAR := $(OBJDIR)/listar_figuras.o $(OBJDIR)/file_system.o
OBJS_EXTRAER := $(OBJDIR)/extraer_figura.o $(OBJDIR)/file_system.o

# Ejecutables
TARGET_CLIENT := $(BINDIR)/MirrorClient.out
TARGET_FIGURA := $(BINDIR)/figura_discovery.out
TARGET_TENEDOR := $(BINDIR)/tenedor_discovery.out
TARGET_LISTAR := $(BINDIR)/listar_figuras.out
TARGET_EXTRAER := $(BINDIR)/extraer_figura.out

TARGETS := $(TARGET_CLIENT) $(TARGET_FIGURA) $(TARGET_TENEDOR) $(TARGET_LISTAR) $(TARGET_EXTRAER)

.PHONY: all clean run client figuras tenedor listar extraer

# Regla por defecto
all: $(TARGETS)

# Compilación genérica
$(OBJDIR)/%.o: %.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(OBJDIR)/%.o: %.cc | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

# Enlace de ejecutables
$(TARGET_CLIENT): $(OBJS_CLIENT) | $(BINDIR)
	$(CXX) $^ -o $@

$(TARGET_FIGURA): $(OBJS_FIGURA) | $(BINDIR)
	$(CXX) $^ -o $@

$(TARGET_TENEDOR): $(OBJS_TENEDOR) | $(BINDIR)
	$(CXX) $^ -o $@

$(TARGET_LISTAR): $(OBJS_LISTAR) | $(BINDIR)
	$(CXX) $^ -o $@

$(TARGET_EXTRAER): $(OBJS_EXTRAER) | $(BINDIR)
	$(CXX) $^ -o $@

# Crear directorios si no existen
$(OBJDIR) $(BINDIR):
	mkdir -p $@

# Targets individuales
client: $(TARGET_CLIENT)
figuras: $(TARGET_FIGURA)
tenedor: $(TARGET_TENEDOR)
listar: $(TARGET_LISTAR)
extraer: $(TARGET_EXTRAER)

# Limpieza
clean:
	rm -rf $(OBJDIR) $(BINDIR)

# Ejecución simple
run:
	@echo "Uso: make run_[client|figuras|tenedor|listar|extraer]"

run_client: client
	./$(TARGET_CLIENT)

run_figuras: figuras
	./$(TARGET_FIGURA)

run_tenedor: tenedor
	./$(TARGET_TENEDOR)

run_listar: listar
	./$(TARGET_LISTAR)

run_extraer: extraer
	@echo "Uso: ./$(TARGET_EXTRAER) <nombre_figura>"
