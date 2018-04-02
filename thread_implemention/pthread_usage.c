#include <stdio.h>
#include <pthread.h>
// remember to set compilation option -pthread

void *busy(void *ptr) {
	// ptr will point to "Hi"
	puts("Hello World");
	return NULL;
}
int main() {
	pthread_t id0, id1;
	void * result;

	pthread_create(&id0, NULL, busy, "Hi1");
	pthread_create(&id1, NULL, busy, "Hi2");
	// while (1) {} // Loop forever
	pthread_join(id, &result);
	pthread_exit(NULL);	// ensure all thread is finish, then exit
}             
