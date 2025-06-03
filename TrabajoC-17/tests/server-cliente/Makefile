# Compiladores
CXX := g++

# Flags de compilación
CXXFLAGS := -g -c -std=c++11
LDFLAGS :=

# Directorios
SRC := .
OBJDIR := obj
BINDIR := exec

VPATH := $(SRC)

# Objetos (ajustados según nombres de archivos que mencionaste)
OBJS_CLIENT := $(OBJDIR)/MirrorClient.o $(OBJDIR)/VSocket.o $(OBJDIR)/Socket.o
OBJS_SERVER := $(OBJDIR)/ThreadMirrorServer.o $(OBJDIR)/VSocket.o $(OBJDIR)/Socket.o $(OBJDIR)/file_system.o

TARGETS := $(BINDIR)/MirrorClient.out $(BINDIR)/ThreadMirrorServer.out

# Regla principal
all: $(TARGETS)

# Crear carpetas si no existen
$(OBJDIR) $(BINDIR):
	mkdir -p $@

# Reglas genéricas para .cc y .cpp -> .o
$(OBJDIR)/%.o: %.cc | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(OBJDIR)/%.o: %.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

# Enlazado de ejecutables
$(BINDIR)/MirrorClient.out: $(OBJS_CLIENT) | $(BINDIR)
	$(CXX) $^ -o $@

$(BINDIR)/ThreadMirrorServer.out: $(OBJS_SERVER) | $(BINDIR)
	$(CXX) $^ -o $@

# Limpieza
clean:
	rm -rf $(OBJDIR) $(BINDIR)
