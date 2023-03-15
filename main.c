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
	char line[LINE_SIZE];
	while (strcmp(line, "STOP\n") != 0) {
		fgets(line, LINE_SIZE, stdin);
		putBuff(&buffers[0], line);
	}
	return NULL;
}

void* remLineSep(void* args) {
	char line[LINE_SIZE];
	while (strcmp(line, "STOP ") != 0) {
		getBuff(&buffers[0], line);
		replaceSubstring(line, "\n", ' ');
		putBuff(&buffers[1], line);
	}
	return NULL;
}

void* remPlusPlus(void* args) {
	char line[LINE_SIZE];
	while (strcmp(line, "STOP ") != 0) {
		getBuff(&buffers[1], line);
		replaceSubstring(line, "++", '^');
		putBuff(&buffers[2], line);
	}
	return NULL;
}

void* writeOutput(void* args) {
	char input[LINE_SIZE];
	while (strcmp(input, "STOP ") != 0) {
		getBuff(&buffers[2], input);
		static char outputBuffer[LINE_SIZE * MAX_LINES] = {0};

		strcat(outputBuffer, input);

		while (strlen(outputBuffer) >= 80) {
			char line[81];
			strncpy(line, outputBuffer, 80);
			line[80] = '\0';
			printf("%s\n", line);

			memmove(outputBuffer, outputBuffer + 80, strlen(outputBuffer + 80) + 1);
		}
	}
	return NULL;
}

//void* processThread(char* stopStr) {
//	char input[LINE_SIZE];
//	while (strcmp(input, stopStr) != 0) {
//		getBuff(&buffers[1], input);
//		replaceSubstring(input, "++", '^');
//		putBuff(&buffers[2], input);
//	}
//	return NULL;
//}

int main(int argc, const char * argv[]) {
	initBuffs();
	pthread_t input, line_sep, plus_sign, output;
	
	// Create the threads
	pthread_create(&input, NULL, getInput, NULL);
	pthread_create(&line_sep, NULL, remLineSep, NULL);
	pthread_create(&plus_sign, NULL, remPlusPlus, NULL);
	pthread_create(&output, NULL, writeOutput, NULL);
	
	// Wait for the threads to terminate
	pthread_join(input, NULL);
	pthread_join(line_sep, NULL);
	pthread_join(plus_sign, NULL);
	pthread_join(output, NULL);

	cleanupBuffs();
	return 0;
}
