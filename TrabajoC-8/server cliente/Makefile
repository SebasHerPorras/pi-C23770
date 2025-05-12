# Linux Makefile

CC  := gcc
CXX := g++
CFLAGS := -g -c
CXXFLAGS := -g -c -std=c++11
LDFLAGS := 
TARGETS := MirrorClient.out ThreadMirrorServer.out

all: $(TARGETS)

# Regla general para compilar archivos .cc a .o
%.o: %.cc
	$(CXX) $(CXXFLAGS) $< -o $@

# MirrorClient.out depende de MirrorClient.o, VSocket.o y Socket.o
MirrorClient.out: MirrorClient.o VSocket.o Socket.o
	$(CXX) $^ $(LDFLAGS) -o $@

# ThreadMirrorServer.out depende de ThreadMirrorServer.o, VSocket.o, Socket.o y file_system.o
ThreadMirrorServer.out: ThreadMirrorServer.o VSocket.o Socket.o file_system.o
	$(CXX) $^ $(LDFLAGS) -o $@

# Limpieza de archivos objeto y ejecutables
clean:
	rm -f *.o *.out
