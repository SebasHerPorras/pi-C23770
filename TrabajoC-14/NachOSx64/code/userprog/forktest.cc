// fork_test.c
#include "syscall.h"
#include <stdio.h>



void hola() {
		int i;
		for (i = 0; i < 10; i++) {
				printf("Hola desde el hilo hijo %d\n", i);
		}
}
