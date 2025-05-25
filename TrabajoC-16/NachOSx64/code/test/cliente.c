#include "syscall.h"

int main() {
   int id;
   char a[512];

   id = Socket(AF_INET_NachOS, SOCK_STREAM_NachOS);
   
   Connect(id, "172.28.234.96", 1234);

   Write("GET /figure?name=gato HTTP/1.1\r\n\r\n", 36, id);
   Read(a, 512, id);
   Write(a, 512, 1); // salida estándar
   Close(id);
}
