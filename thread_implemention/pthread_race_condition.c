#include <pthread.h>
#include <stdio.h>
void* myfunc(void* ptr) {
	int i = *((int *) ptr);
	printf("%d ", i);
	return NULL;
}

int main() {
	// Each thread gets a different value of i to process
	// cuz i is a value which can be modified by the main, this is so-called race condition
	// thread safe is import
	int i;
	pthread_t tid;
	for(i =0; i < 10; i++) {
		pthread_create(&tid, NULL, myfunc, &i); // ERROR
	}
	pthread_exit(NULL);
}
