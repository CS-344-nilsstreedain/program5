/**
 * @file line_processor.c
 *
 * @author Nils Streedain (https://github.com/nilsstreedain)
 * @brief A multi-threaded text processing application that reads input, removes specified substrings, and formats output.
 *
 * This application reads input from stdin and processes it using four threads. The first thread reads the input, and the
 * following three threads remove specified substrings from the input. The last thread formats and prints the output.
 *
 * Each thread reads and writes data to a shared buffer. The program uses pthread mutexes and condition variables to
 * synchronize access to the shared buffers.
 *
 * Example usage:
 * 1. Compile the program with gcc --std=gnu99 -o line_processor line_processor.c -lpthread.
 * 2. Run the program with ./line_processor.
 * 3. Provide input to the program, and it will print the processed output.
*/
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#define NUM_BUFFS 3
#define MAX_LINES 50
#define LINE_SIZE 1000
#define PRINT_SIZE 80

/**
 * @struct Buffer
 * @brief A structure representing a buffer that holds lines of text.
 *
 * The Buffer structure holds a 2D array of characters to store multiple lines of text, as well as several variables
 * to keep track of the current count of lines, producer index, and consumer index. Additionally, a pthread_mutex_t
 * and a pthread_cond_t are included for synchronization purposes when multiple threads access the buffer.
 *
 * @var Buffer::buff
 * A 2D array of characters that can store up to MAX LINES lines, each with a maximum length of LINE SIZE.
 * @var Buffer::count
 * The current number of lines stored in the buffer.
 * @var Buffer::iProd
 * The index at which the next line will be produced (written) in the buffer.
 * @var Buffer::iCon
 * The index at which the next line will be consumed (read) from the buffer.
 * @var Buffer::mutex
 * A mutex used to synchronize access to the buffer.
 * @var Buffer::full
 * A condition variable used to signal when the buffer has at least one line available for consumption.
 */
typedef struct {
	char buff[MAX_LINES][LINE_SIZE];
	int count, iProd, iCon;
	pthread_mutex_t mutex;
	pthread_cond_t full;
} Buffer;

Buffer buffers[NUM_BUFFS];

/**
 * @brief Retrieves a line of text from the specified buffer and stores it in the output array.
 *
 * The getBuff function locks the buffer's mutex, then waits for the buffer's count to be greater than zero, ensuring
 * that there is a line available for consumption. Once a line is available, the function copies the line from the
 * buffer to the output array, updates the buffer's count, and unlocks the mutex.
 *
 * @param buffer A pointer to the Buffer structure from which a line of text will be retrieved.
 * @param output A character array that will store the retrieved line of text.
 */
void getBuff(Buffer* buffer, char output[]) {
	// Lock mutex and wait until count > 0
	pthread_mutex_lock(&buffer->mutex);
	while (!buffer->count)
		pthread_cond_wait(&buffer->full, &buffer->mutex);
	
	// Copy output to buffer, increment vars, and unlock mutex
	strcpy(output, buffer->buff[buffer->iCon++]);
	buffer->count--;
	pthread_mutex_unlock(&buffer->mutex);
}

/**
 * @brief Replaces all occurrences of a specified substring within a string with a single replacement character.
 *
 * The replaceSubstring function searches for all occurrences of the specified 'remove' substring within the input
 * 'str'. When a match is found, the function replaces the substring with the 'replace' character and moves the
 * remaining characters in the string to fill the gap left by the removed substring. The process is repeated until
 * no more occurrences of the substring are found.
 *
 * @param str A pointer to the input string in which the specified substring will be replaced.
 * @param remove A pointer to the substring that will be replaced within the input string.
 * @param replace The replacement character that will be used to replace the specified substring.
 */
void replaceSubstring(char* str, char* remove, char replace) {
	const size_t remove_len = strlen(remove);
	
	// Replace remove with reaplce until str does not contain remove
	while ((str = strstr(str, remove))) {
		*str = replace;
		memmove(str + 1, str + remove_len, strlen(str + remove_len) + 1);
	}
}

/**
 * @brief Stores a line of text in the specified buffer.
 *
 * The putBuff function locks the buffer's mutex, then copies the input line of text to the buffer using the buffer's
 * iProd index. It increments the buffer's count and signals that the buffer is not empty using the buffer's full
 * condition variable. Finally, the function unlocks the buffer's mutex.
 *
 * @param buffer A pointer to the Buffer structure where the input line of text will be stored.
 * @param input A character array containing the line of text to be stored in the buffer.
 */
void putBuff(Buffer* buffer, char input[]) {
	// Lock mutex, copy input to buffer and increment vars
	pthread_mutex_lock(&buffer->mutex);
	strcpy(buffer->buff[buffer->iProd++], input);
	buffer->count++;
	
	// Signal buffer full and unlock
	pthread_cond_signal(&buffer->full);
	pthread_mutex_unlock(&buffer->mutex);
}

/**
 * @brief Formats and prints the input text with a fixed width of PRINT_SIZE characters per line.
 *
 * The printOutput function appends the input text to an internal static buffer. It then checks if the length of the
 * buffer is greater than or equal to PRINT_SIZE. If so, the function prints the first PRINT_SIZE characters of the
 * buffer as a line, and shifts the remaining characters in the buffer to the beginning. This process is repeated
 * until the length of the buffer is less than PRINT_SIZE.
 *
 * @param input A pointer to the input text that will be formatted and printed with fixed width.
 */
void printOutput(char* input) {
	// Init static string output and concatinate with input
	static char output[LINE_SIZE * MAX_LINES] = {0};
	strcat(output, input);
	
	// Loop over output, printing 80 chars at a time
	while (strlen(output) >= PRINT_SIZE) {
		printf("%.*s\n", PRINT_SIZE, output);
		memmove(output, output + PRINT_SIZE, strlen(output + PRINT_SIZE) + 1);
	}
}

/**
 * @struct ThreadArgs
 * @brief A structure containing arguments required for each processing thread.
 *
 * The ThreadArgs structure holds a set of parameters that are passed to the processThread function. These parameters
 * include the index of the buffer being used, the stop string that indicates the end of processing, the search string
 * and its corresponding replacement character, and flags to determine whether the thread reads from a buffer or stdin,
 * and writes to a buffer or calls printOutput.
 *
 * @var ThreadArgs::iBuffer
 * The index of the buffer being used by the thread.
 * @var ThreadArgs::stopStr
 * A pointer to the stop string that indicates the end of processing.
 * @var ThreadArgs::searchStr
 * A pointer to the search string that will be replaced within the input text.
 * @var ThreadArgs::replaceChar
 * The replacement character that will be used to replace the specified search string.
 * @var ThreadArgs::readBuff
 * A flag that determines whether the thread reads from a buffer (1) or stdin (0).
 * @var ThreadArgs::writeBuff
 * A flag that determines whether the thread writes to a buffer (1) or calls printOutput (0).
 */
typedef struct {
	int iBuffer;
	char* stopStr;
	char* searchStr;
	char replaceChar;
	int readBuff; // 1 for getBuff, 0 for fgets
	int writeBuff; // 1 for putBuff, 0 for printOutput
} ThreadArgs;

/**
 * @brief The main processing function executed by each thread.
 *
 * The processThread function reads lines of text based on the provided ThreadArgs structure. It reads input from either
 * a buffer or stdin and processes the input by replacing specified substrings with a single character, if required.
 * The processed input is then either written to a buffer or printed using the printOutput function. The thread
 * continues processing input until it encounters the specified stop string.
 *
 * @param args A pointer to a ThreadArgs structure containing the arguments required for the processing thread.
 * @return NULL The function returns NULL as it is intended to be used with pthread_create.
 */
void* processThread(void* args) {
	// Get args from thread
	ThreadArgs* tArgs = (ThreadArgs*) args;
	
	// Get, modify and write/output line
	char line[LINE_SIZE];
	while (strcmp(line, tArgs->stopStr)) {
		// Populate line string
		if (tArgs->readBuff)
			getBuff(&buffers[tArgs->iBuffer - 1], line);
		else
			fgets(line, LINE_SIZE, stdin);

		// Optionally modify line string
		if (tArgs->searchStr)
			replaceSubstring(line, tArgs->searchStr, tArgs->replaceChar);
		
		// Write/Output line string
		if (tArgs->writeBuff)
			putBuff(&buffers[tArgs->iBuffer], line);
		else
			printOutput(line);
	}
	return NULL;
}

/**
 * @brief The main function of the multi-threaded text processing application.
 *
 * The main function initializes the shared buffers, creates four threads with their corresponding ThreadArgs
 * structures, and starts the threads. It then waits for all threads to complete execution and cleans up resources
 * by destroying the mutexes and condition variables associated with the buffers. The program reads input from stdin,
 * processes it using the threads, and prints the formatted output to stdout.
 *
 * @param argc The number of command-line arguments.
 * @param argv An array of pointers to the command-line argument strings.
 * @return 0 The function returns 0 to indicate successful execution.
 */
int main(int argc, const char * argv[]) {
	// Init buffers
	memset(buffers, 0, sizeof(buffers));
	for (int i = 0; i < NUM_BUFFS; i++) {
		pthread_mutex_init(&buffers[i].mutex, NULL);
		pthread_cond_init(&buffers[i].full, NULL);
	}
	
	// Init threads & thread arguments
	pthread_t threads[4];
	ThreadArgs threadArgs[] = {
		{0, "STOP\n", NULL, '\0', 0, 1},
		{1, "STOP ", "\n", ' ', 1, 1},
		{2, "STOP ", "++", '^', 1, 1},
		{3, "STOP ", NULL, '\0', 1, 0}
	};

	// Create threads
	for (int i = 0; i < 4; i++)
		pthread_create(&threads[i], NULL, processThread, &threadArgs[i]);

	// Join threads
	for (int i = 0; i < 4; i++)
		pthread_join(threads[i], NULL);

	// Cleanup buffers and exit
	for (int i = 0; i < NUM_BUFFS; i++) {
		pthread_mutex_destroy(&buffers[i].mutex);
		pthread_cond_destroy(&buffers[i].full);
	}
	return 0;
}
