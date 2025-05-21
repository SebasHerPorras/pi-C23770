// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include <sys/socket.h>
#include <arpa/inet.h>	// for inet_pton
#include <fcntl.h>
#include <unistd.h>




void NachOS_TraerFigura();
void traerAux(char* figura);
void returnFromSystemCall();
void TraerFiguraThread(void* arg);
void NachosForkThread(void* p);
void NachOS_Fork();
char* copyStringFromMachine(int userAddr);

#define MAX_FIGURA_LEN 256

char* copyStringFromMachine(int userAddr) {
   char* kernelBuffer = new char[MAX_FIGURA_LEN];
   int i = 0;
   int ch;
   do {
       machine->ReadMem(userAddr + i, 1, &ch);
       kernelBuffer[i] = (char)ch;
       i++;
   } while (ch != 0 && i < MAX_FIGURA_LEN);
   kernelBuffer[MAX_FIGURA_LEN - 1] = '\0';
   return kernelBuffer;
}

/*
 *  System call interface: Halt()
 */
void NachOS_Halt() {		// System call 0

	DEBUG('a', "Shutdown, initiated by user program.\n");
   	interrupt->Halt();

}


/*
 *  System call interface: void Exit( int )
 */
void NachOS_Exit() {
   int status = machine->ReadRegister(4);    // código de salida
   DEBUG('u', "Exit system call with status %d\n", status);

   AddrSpace* space = (AddrSpace*)currentThread->space;
   auto* pageTable = space->getPageTable();
   unsigned numPages = space->getNumPages();

   // Liberar páginas asignadas en el bitmap con exclusión mutua
   for (unsigned i = 0; i < numPages; i++) {
      memoryLock->Acquire();
      MiMapa->Clear(pageTable[i].physicalPage);
      memoryLock->Release();
   }

   printf("Thread %s exiting with status %d\n", currentThread->getName(), status);

   // Avanzar el PC para evitar bucles infinitos de syscalls
   machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
   machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
   machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg) + 4);
   // No es necesario avanzar NextPCReg, solo PCReg y PrevPCReg.
   // El bucle infinito suele ocurrir si no se avanza el PC correctamente.
   // Elimina la línea que avanza NextPCReg.

   currentThread->Finish();
}



/*
 *  System call interface: SpaceId Exec( char * )
 */
void NachOS_Exec() {		// System call 2
}


/*
 *  System call interface: int Join( SpaceId )
 */
void NachOS_Join() {		// System call 3
}


/*
 *  System call interface: void Create( char * )
 */
void NachOS_Create() {		// System call 4
}


/*
 *  System call interface: OpenFileId Open( char * )
 */
bool copyStringFromMachine(int from, char *to, unsigned size) {
   unsigned i = 0;
   int ch;

   while (i < size - 1) {
       machine->ReadMem(from + i, 1, &ch);
       to[i] = (char) ch;
       if (to[i] == '\0') break;
       i++;
   }
   to[i] = '\0';
   return true;
}

void NachOS_Open() {
   int userPtr = machine->ReadRegister(4);
   char filename[256];

   if (!copyStringFromMachine(userPtr, filename, 256)) {
       machine->WriteRegister(2, -1);
   } else {
       OpenFile* file = fileSystem->Open(filename);
       if (file == NULL) {
           machine->WriteRegister(2, -1);
       } else {
           int index = openFilesTable->Open(file);
           if (index == -1) {
               delete file;
               machine->WriteRegister(2, -1);
           } else {
               machine->WriteRegister(2, index);
           }
       }
   }

   // Avanzar el PC SIEMPRE
   machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
   machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
   machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg) + 4);
}




/*
 *  System call interface: OpenFileId Write( char *, int, OpenFileId )
 */
void NachOS_Write() {
   int addr = machine->ReadRegister(4);
   int size = machine->ReadRegister(5);
   int fd = machine->ReadRegister(6);

   DEBUG('u', "Write syscall: fd=%d, addr=0x%x, size=%d\n", fd, addr, size);

   if (size < 0) {
       machine->WriteRegister(2, -1);
       // Avanzar PC
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg) + 4);
       return;
   }

   int bytesToWrite = (size < 511) ? size : 511;
   char buffer[512];

   for (int i = 0; i < bytesToWrite; i++) {
       int val;
       if (!machine->ReadMem(addr + i, 1, &val)) {
           machine->WriteRegister(2, -1);
           // Avanzar PC
           machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
           machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
           machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg) + 4);
           return;
       }
       buffer[i] = (char)val;
   }
   buffer[bytesToWrite] = '\0';

   if (fd == 1 || fd == 2) {
       printf("%.*s", bytesToWrite, buffer);
       machine->WriteRegister(2, bytesToWrite);
   } else {
       OpenFile* file = nullptr;
       if (fd >= 0) {
           file = openFilesTable->getOpenFile(fd);
       }

       if (file == nullptr) {
           DEBUG('u', "Write syscall: Descriptor inválido %d\n", fd);
           machine->WriteRegister(2, -1);
       } else {
           int written = file->Write(buffer, bytesToWrite);
           machine->WriteRegister(2, written);
       }
   }
   // ahorita en memoria está el buffer con muchas letras
   // meter buffer correcto 

   
   
   machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
   machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
   machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg) + 4);
}





/*
 *  System call interface: OpenFileId Read( char *, int, OpenFileId )
 */
void NachOS_Read() { // System call 7
   // Leer parámetros desde los registros
   int bufferAddr = machine->ReadRegister(4);  // Dirección del buffer en espacio de usuario
   int size       = machine->ReadRegister(5);  // Número de bytes a leer
   int fileId     = machine->ReadRegister(6);  // Descriptor de archivo

   // Validación básica
   if (size <= 0 || bufferAddr == 0) {
       machine->WriteRegister(2, -1);
       return;
   }

   char* kernelBuffer = new char[size + 1]; // +1 por seguridad
   int bytesRead = 0;

   // Leer desde archivo/socket real
   bytesRead = read(fileId, kernelBuffer, size);
   
   if (bytesRead < 0) {
       DEBUG('u', "Read syscall: error al leer desde fd=%d\n", fileId);
       machine->WriteRegister(2, -1);
   } else {
       // Copiar desde kernel hacia espacio de usuario
       for (int i = 0; i < bytesRead; i++) {
           machine->WriteMem(bufferAddr + i, 1, (int)kernelBuffer[i]);
       }
       machine->WriteRegister(2, bytesRead); // Cantidad de bytes leídos
   }

   delete[] kernelBuffer;

   // Avanzar el PC
   machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
   machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
   machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg) + 4);
}



/*
 *  System call interface: void Close( OpenFileId )
 */
void NachOS_Close() {		// System call 8
}


/*
 *  System call interface: void Fork( void (*func)() )
 
void NachOS_Fork() {
   DEBUG('u', "System Call: Fork\n");

   // Leer dirección de la función del usuario (en lenguaje MIPS) desde el registro 4
   int userFuncAddr = machine->ReadRegister(4);

   // Crear nuevo hilo
   Thread *newThread = new Thread("child created by Fork");

   // Crear un nuevo espacio de direcciones con pila independiente
   // Usamos un constructor de AddrSpace que copia el código y datos, pero crea una nueva pila
   newThread->space = currentThread->space->Clone();

   // Lanzar el hilo en una función especial del kernel (NachosForkThread)
   newThread->Fork(NachosForkThread, (void *)userFuncAddr);
}

void NachosForkThread(void *arg) {
   // Recuperar dirección de rutina del usuario
   int userFuncAddr = (int)(long)arg;

   // Establecer espacio de direcciones de este hilo
   currentThread->space->InitRegisters();
   currentThread->space->RestoreState();

   // Sobrescribir PC con dirección del hijo y el retorno con Exit
   machine->WriteRegister(PCReg, userFuncAddr);
   machine->WriteRegister(NextPCReg, userFuncAddr + 4);
   machine->WriteRegister(RetAddrReg, 4); // posición del Exit

   // Lanzar el programa (salta al modo usuario)
   machine->Run();
}
*/
void NachOS_TraerFigura() {	// System call 36
   printf("\n\nTraerFigura syscall: haciendo fork para traer figura\n\n");

   // Leer dirección del string de usuario desde el registro 4
   int userStrAddr = machine->ReadRegister(4);
   int size = machine->ReadRegister(5);
   int fd = machine->ReadRegister(6);

   // Copiar string de usuario a kernel
   char* figura = copyStringFromMachine(userStrAddr);

   // Hacer fork pasando la figura como argumento
   NachOS_Fork();

   // Avanzar el PC SIEMPRE en padre
   machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
   machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
   machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg) + 4);
}

void NachOS_Fork() {
   DEBUG('u', "System Call: ForkFigura\n");

   int addr = machine->ReadRegister(4);
   char* figura = copyStringFromMachine(addr);

   // Creamos un nuevo hilo hijo
   Thread* newThread = new Thread("child de TraerFigura");

   // Clonamos espacio de direcciones para que el hilo tenga su propio espacio
   newThread->space = currentThread->space->Clone();

   // Lanzamos el hilo para que ejecute la función kernel TraerFiguraThread (que ejecuta traerAux)
   newThread->Fork(TraerFiguraThread, figura);
}

// Función kernel que ejecuta el hilo hijo
void TraerFiguraThread(void* arg) {
   char* figura = (char*)arg;

   printf("\n\nHilo hijo ejecutando traerAux con figura: %s\n\n\n", figura);
   traerAux(figura);

   // Liberar memoria kernel de figura después de usarla
   delete[] figura;

   // Terminar hilo hijo limpiamente
   currentThread->Finish();
}

void traerAux(char* figura) {
   printf("Traer figura: %s\n\n", figura);
   // Aquí puedes agregar código para traer la figura (p.ej. leer de disco, etc)
}




/*
 *  System call interface: void Yield()
 */
void NachOS_Yield() {		// System call 10
}


/*
 *  System call interface: Sem_t SemCreate( int )
 */
void NachOS_SemCreate() {		// System call 11
}


/*
 *  System call interface: int SemDestroy( Sem_t )
 */
void NachOS_SemDestroy() {		// System call 12
}


/*
 *  System call interface: int SemSignal( Sem_t )
 */
void NachOS_SemSignal() {		// System call 13
}


/*
 *  System call interface: int SemWait( Sem_t )
 */
void NachOS_SemWait() {		// System call 14
}


/*
 *  System call interface: Lock_t LockCreate( int )
 */
void NachOS_LockCreate() {		// System call 15
}


/*
 *  System call interface: int LockDestroy( Lock_t )
 */
void NachOS_LockDestroy() {		// System call 16
}


/*
 *  System call interface: int LockAcquire( Lock_t )
 */
void NachOS_LockAcquire() {		// System call 17
}


/*
 *  System call interface: int LockRelease( Lock_t )
 */
void NachOS_LockRelease() {		// System call 18
}


/*
 *  System call interface: Cond_t LockCreate( int )
 */
void NachOS_CondCreate() {		// System call 19
}


/*
 *  System call interface: int CondDestroy( Cond_t )
 */
void NachOS_CondDestroy() {		// System call 20
}


/*
 *  System call interface: int CondSignal( Cond_t )
 */
void NachOS_CondSignal() {		// System call 21
}


/*
 *  System call interface: int CondWait( Cond_t )
 */
void NachOS_CondWait() {		// System call 22
}


/*
 *  System call interface: int CondBroadcast( Cond_t )
 */
void NachOS_CondBroadcast() {		// System call 23
}


/*
 *  System call interface: Socket_t Socket( int, int )
 */
void NachOS_Socket() { // System call 30
  // AF_INET_NachOS, SOCK_STREAM_NachOS como parametros
   int domain = machine->ReadRegister(4);
   int type = machine->ReadRegister(5);
   // Validar los parámetros
   // Traducción NachOS a POSIX
   domain = (domain == 0) ? AF_INET : AF_UNSPEC;
   type = (type == 0) ? SOCK_STREAM : SOCK_DGRAM;
   DEBUG('u', "Socket syscall: domain=%d, type=%d\n", domain, type);
   // Crear socket con syscall de c
   int sockfd = socket(domain, type, 0);
   if (sockfd < 0) {
       DEBUG('u', "Socket syscall: Error al crear socket\n");
       printf("Error al crear socket\n");
       machine->WriteRegister(2, -1);
   } else {
       DEBUG('u', "Socket syscall: Socket creado con fd=%d\n", sockfd);
       printf("Socket creado con fd=%d\n", sockfd);
       machine->WriteRegister(2, sockfd);
   }
   // poner el sockfd en el registro 2 que es el de retorno
   // Avanzar el PC SIEMPRE
   machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
   machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
   machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg) + 4);

 }
 
 /*
  *  System call interface: Socket_t Connect( char *, int )
  */
 void NachOS_Connect() { // System call 31
   int sockfd = machine->ReadRegister(4);
   int addr_ptr = machine->ReadRegister(5); // dirección en user space
   int port = machine->ReadRegister(6);

   // Leer IP desde memoria de usuario (máx 16 bytes por seguridad)
   char ip[16]; // ejemplo: "127.0.0.1"
   for (int i = 0; i < 15; ++i) {
       char c;
       machine->ReadMem(addr_ptr + i, 1, (int*)&c);
       ip[i] = c;
       if (c == '\0') break;
   }
   ip[15] = '\0';

   DEBUG('u', "Connect syscall: sockfd=%d, ip=%s, port=%d\n", sockfd, ip, port);
   printf("Connect syscall: sockfd=%d, ip=%s, port=%d\n", sockfd, ip, port);

   // Crear sockaddr_in
   struct sockaddr_in serv_addr;
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_port = htons(port);
   serv_addr.sin_addr.s_addr = inet_addr(ip);

   // Realizar conexión
   printf("Connect syscall: intentando conectar...\n");
   int result = connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
   printf("Connect syscall: resultado de connect=%d\n", result);
   if (result < 0) {
       perror("Connect syscall: error en connect");
       machine->WriteRegister(2, -1);
   } else {
       machine->WriteRegister(2, 0); // éxito
         DEBUG('u', "Connect syscall: conexión exitosa\n");
         printf("Connect syscall: conexión exitosa\n");
   }

   // Avanzar el PC SIEMPRE
   machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg)); 
   machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
   machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg) + 4);  
}



/*
 *  System call interface: int Bind( Socket_t, int )
 */
void NachOS_Bind() {		// System call 32
}


/*
 *  System call interface: int Listen( Socket_t, int )
 */
void NachOS_Listen() {		// System call 33
}


/*
 *  System call interface: int Accept( Socket_t )
 */
void NachOS_Accept() {		// System call 34
}


/*
 *  System call interface: int Shutdown( Socket_t, int )
 */
void NachOS_Shutdown() {	// System call 25
}
/*
void NachOS_TraerFigura() {	// System call 36
   // este es el syscall debe llamar al fork con la funcion aux que hace el trabajo
   // de momento sin fork

   printf("\n\nTraerFigura syscall\n\n");
   traerAux("gato");

   // Avanzar el PC SIEMPRE
   machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
   machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
   machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg) + 4);
}

void traerAux(char* figura) {
   // esta funcion es la que hace el trabajo de traer la figura
   // en este caso solo imprime el nombre de la figura
   printf("Traer figura: %s\n", figura);
   // aqui se puede agregar el codigo para traer la figura
   // por ahora solo se imprime el nombre de la figura
   // y se libera la memoria

}*/
void returnFromSystemCall() {
   int pc = machine->ReadRegister(PCReg);
   machine->WriteRegister(PrevPCReg, pc);
   machine->WriteRegister(PCReg, pc + 4);
   // pc + 4 es la siguiente instrucción
   // pc + 8 es la siguiente instrucción después de la syscall
   machine->WriteRegister(NextPCReg, pc + 8);
}


//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------

void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);
    

    switch ( which ) {

       case SyscallException:
          switch ( type ) {
             case SC_Halt:		// System call # 0
                NachOS_Halt();
                break;
             case SC_Exit:		// System call # 1
                NachOS_Exit();
                break;
             case SC_Exec:		// System call # 2
               DEBUG('u', "SC_Exit recibido\n");
               NachOS_Exit();
               break;   
            
             case SC_Join:		// System call # 3
                NachOS_Join();
                break;

             case SC_Create:		// System call # 4
                NachOS_Create();
                break;
             case SC_Open:		// System call # 5
                NachOS_Open();
                break;
             case SC_Read:		// System call # 6
                NachOS_Read();
                break;
             case SC_Write:		// System call # 7
                NachOS_Write();
                break;
             case SC_Close:		// System call # 8
                NachOS_Close();
                break;

             case SC_Fork:		// System call # 9
                NachOS_Fork();
                break;
             case SC_Yield:		// System call # 10
                NachOS_Yield();
                break;

             case SC_SemCreate:         // System call # 11
                NachOS_SemCreate();
                break;
             case SC_SemDestroy:        // System call # 12
                NachOS_SemDestroy();
                break;
             case SC_SemSignal:         // System call # 13
                NachOS_SemSignal();
                break;
             case SC_SemWait:           // System call # 14
                NachOS_SemWait();
                break;

             case SC_LckCreate:         // System call # 15
                NachOS_LockCreate();
                break;
             case SC_LckDestroy:        // System call # 16
                NachOS_LockDestroy();
                break;
             case SC_LckAcquire:         // System call # 17
                NachOS_LockAcquire();
                break;
             case SC_LckRelease:           // System call # 18
                NachOS_LockRelease();
                break;

             case SC_CondCreate:         // System call # 19
                NachOS_CondCreate();
                break;
             case SC_CondDestroy:        // System call # 20
                NachOS_CondDestroy();
                break;
             case SC_CondSignal:         // System call # 21
                NachOS_CondSignal();
                break;
             case SC_CondWait:           // System call # 22
                NachOS_CondWait();
                break;
             case SC_CondBroadcast:           // System call # 23
                NachOS_CondBroadcast();
                break;

             case SC_Socket:	// System call # 30
             printf("\n\nsyscall de socket\n\n");
		NachOS_Socket();
               break;
             case SC_Connect:	// System call # 31
		NachOS_Connect();
               break;
             case SC_Bind:	// System call # 32
		NachOS_Bind();
               break;
             case SC_Listen:	// System call # 33
		NachOS_Listen();
               break;
             case SC_Accept:	// System call # 32
		NachOS_Accept();
               break;
             case SC_Shutdown:	// System call # 33
		NachOS_Shutdown();
               break;
             case SC_TraerFigura:	// System call # 34
      NachOS_TraerFigura();
               break;

             default:
                printf("Unexpected syscall exception %d\n", type );
                ASSERT( false );
                break;
          }
          break;

       case PageFaultException: {
          break;
       }

       case ReadOnlyException:
          printf( "Read Only exception (%d)\n", which );
          ASSERT( false );
          break;

       case BusErrorException:
          printf( "Bus error exception (%d)\n", which );
          ASSERT( false );
          break;

       case AddressErrorException:
          printf( "Address error exception (%d)\n", which );
          ASSERT( false );
          break;

       case OverflowException:
          printf( "Overflow exception (%d)\n", which );
          ASSERT( false );
          break;

       case IllegalInstrException:
          printf( "Ilegal instruction exception (%d)\n", which );
          ASSERT( false );
          break;

       default:
          printf( "Unexpected exception %d\n", which );
          ASSERT( false );
          break;
    }

}
