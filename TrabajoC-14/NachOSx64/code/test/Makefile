#
# Makefile para construir programas de usuario para Nachos
#

# Directorio con los ejecutables cruzados
GCCDIR = ../bin/

# Flags para linker y ensamblador
LDFLAGS = -T script -N
ASFLAGS = -mips1

# Incluye directorios de cabeceras
INCDIR = -I../userprog -I../threads

# Flags para compilador C (sin -G 0 para evitar error con 'as')
CFLAGS = -c $(INCDIR) -mips1

# Agrega el directorio bin al PATH para usar compiladores y herramientas
PATH := ../bin:$(PATH)

# Herramientas
# Herramientas
CC = $(GCCDIR)mips-gcc
AS = $(GCCDIR)as
LD = $(GCCDIR)mips-ld

# Usar mips-gcc para el preprocesador
CPP = $(CC) -E
CPPFLAGS = $(INCDIR)


# Regla para generar start.o a partir de start.s y syscall.h
start.o: start.s ../userprog/syscall.h
	$(CPP) $(CPPFLAGS) start.s > strt.s
	$(AS) $(ASFLAGS) -o start.o strt.s
	/bin/rm strt.s

# Reglas para los programas predefinidos

halt.o: halt.c
	$(CC) $(CFLAGS) halt.c

halt: halt.o start.o
	$(LD) $(LDFLAGS) start.o halt.o -o halt.coff
	../bin/coff2noff halt.coff halt

shell.o: shell.c
	$(CC) $(CFLAGS) shell.c

shell: shell.o start.o
	$(LD) $(LDFLAGS) start.o shell.o -o shell.coff
	../bin/coff2noff shell.coff shell

sort.o: sort.c
	$(CC) $(CFLAGS) sort.c

sort: sort.o start.o
	$(LD) $(LDFLAGS) start.o sort.o -o sort.coff
	../bin/coff2noff sort.coff sort

matmult.o: matmult.c
	$(CC) $(CFLAGS) matmult.c

matmult: matmult.o start.o
	$(LD) $(LDFLAGS) start.o matmult.o -o matmult.coff
	../bin/coff2noff matmult.coff matmult

# Regla para fork (agregada)

fork.o: forktest.c
	$(CC) $(CFLAGS) forktest.c

fork: fork.o start.o
	$(LD) $(LDFLAGS) start.o fork.o -o fork.coff
	../bin/coff2noff fork.coff fork

socket.o: socket.c
	$(CC) $(CFLAGS) -c socket.c

socket: socket.o start.o
	$(LD) $(LDFLAGS) start.o socket.o -o socket.coff
	../bin/coff2noff socket.coff socket

# Regla general para compilar programas simples de un solo archivo .c

%: %.c
	make start.o
	$(CC) $(CFLAGS) -c $<
	$(LD) $(LDFLAGS) start.o $*.o -o $*.coff
	../bin/coff2noff $*.coff $@

