#include <stdio.h>
#include <pthread/pthread.h>
#include <string.h>

#define NUM_BUFFS 3
#define MAX_LINES 50
#define LINE_SIZE 1000

typedef struct {
	char buff[MAX_LINES][LINE_SIZE];
	int count, prod_idx, con_idx;
	pthread_mutex_t mutex;
	pthread_cond_t full;
} Buffer;

Buffer buffers[NUM_BUFFS];

void initBuffs(void) {
	memset(buffers, 0, sizeof(buffers));
	for (int i = 0; i < NUM_BUFFS; i++) {
		pthread_mutex_init(&buffers[i].mutex, NULL);
		pthread_cond_init(&buffers[i].full, NULL);
	}
}

void cleanupBuffs(void) {
	for (int i = 0; i < NUM_BUFFS; i++) {
		pthread_mutex_destroy(&buffers[i].mutex);
		pthread_cond_destroy(&buffers[i].full);
	}
}

void putBuff(Buffer* buffer, char input[]) {
	pthread_mutex_lock(&buffer->mutex);
	strcpy(buffer->buff[buffer->prod_idx], input);
	buffer->prod_idx++;
	buffer->count++;
	pthread_cond_signal(&buffer->full);
	pthread_mutex_unlock(&buffer->mutex);
}

void getBuff(Buffer* buffer, char output[]) {
	pthread_mutex_lock(&buffer->mutex);
	
	while (buffer->count == 0)
		pthread_cond_wait(&buffer->full, &buffer->mutex);
	
	strcpy(output, buffer->buff[buffer->con_idx]);
	buffer->con_idx++;
	buffer->count--;
	
	pthread_mutex_unlock(&buffer->mutex);
}

void replaceSubstring(char* str, char* remove, char replace) {
	const size_t remove_len = strlen(remove);
	
	while ((str = strstr(str, remove)) != NULL) {
		*str = replace;
		memmove(str + 1, str + remove_len, strlen(str + remove_len) + 1);
	}
}

void* getInput(void* args) {
	char input[LINE_SIZE];
	for (int i = 0; i < MAX_LINES; i++) {
		fgets(input, LINE_SIZE, stdin);
		
		if (strcmp(input, "STOP\n") == 0)
			break;
		
		putBuff(&buffers[0], input);
		printf("1:%s\n", buffers[0].buff[buffers[0].prod_idx - 1]);
	}
	return NULL;
}

void* remLineSep(void* args) {
	char input[LINE_SIZE];
	for (int i = 0; i < MAX_LINES; i++) {
		getBuff(&buffers[0], input);

		replaceSubstring(input, "\n", ' ');
		putBuff(&buffers[1], input);
		printf("2:%s\n", buffers[1].buff[buffers[1].prod_idx - 1]);
	}
	return NULL;
}

void* remPlusPlus(void* args) {
	char input[LINE_SIZE];
	for (int i = 0; i < MAX_LINES; i++) {
		getBuff(&buffers[1], input);

		replaceSubstring(input, "++", '^');
		putBuff(&buffers[2], input);
		printf("3:%s\n", buffers[2].buff[buffers[2].prod_idx - 1]);
	}
	return NULL;
}

int main(int argc, const char * argv[]) {
	initBuffs();
	pthread_t input, line_sep, plus_sign, output;
	
	// Create the threads
	pthread_create(&input, NULL, getInput, NULL);
	pthread_create(&line_sep, NULL, remLineSep, NULL);
	pthread_create(&plus_sign, NULL, remPlusPlus, NULL);
//	pthread_create(&output, NULL, write_output, NULL);
	// Wait for the threads to terminate
	pthread_join(input, NULL);
	pthread_join(line_sep, NULL);
	pthread_join(plus_sign, NULL);
//	pthread_join(output, NULL);
	
	
//	char str[1001] = "+++This ++is +a\nmulti-line\nstring.\n";
//	replaceSubstring(str, "\n", ' ');
//	printf("%s\n", str);
//	replaceSubstring(str, "++", '^');
//	printf("%s\n", str);

	cleanupBuffs();
	return 0;
}
