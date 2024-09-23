#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>

#define STRONG_NO            0b00
#define WEAK_NO              0b01
#define WEAK_YES             0b10
#define STRONG_YES           0b11
#define PREFER_GSHARE        0b00
#define WEAK_PREFER_GSHARE   0b01
#define WEAK_PREFER_BIMODAL  0b10
#define PREFER_BIMODAL       0b11

typedef struct {
	unsigned correct, attempted;
	union {
		bool always_val;
		int table_size;
		int history_size;
	};
} TParams;

typedef struct {
	uint64_t addr, target;
	bool actual; 
} BranchTrace;

BranchTrace *g_traces = NULL;
unsigned     g_traces_count = 0;

void *simulate_always(void *arg) {
	TParams *params = (TParams *)arg;
	bool always_val = params->always_val;
	unsigned correct = 0;

	for (unsigned i = 0; i < g_traces_count; i++) {
		if (g_traces[i].actual == always_val) {
			correct++;
		}
	}

	params->correct = correct;
	return NULL;
}

void *simulate_bimodal_one(void *arg) {
	TParams *params = (TParams *)arg;
	int table_size = params->table_size;
	bool *history_table = (bool *)malloc(table_size * sizeof(bool));
	unsigned correct = 0;

	memset(history_table, true, table_size);

	for (unsigned i = 0; i < g_traces_count; i++) {
		unsigned index = g_traces[i].addr % table_size;
		if (history_table[index] == g_traces[i].actual) {
			correct++;
		}
		history_table[index] = g_traces[i].actual;
	}

	params->correct = correct;
	free(history_table);
	return NULL;
}

void *simulate_bimodal_two(void *arg) {
	TParams *params = (TParams *)arg;
	int table_size = params->table_size;
	unsigned char *history_table = (unsigned char *)malloc(table_size * sizeof(unsigned char));
	unsigned correct = 0;

	memset(history_table, STRONG_YES, table_size);

	for (unsigned i = 0; i < g_traces_count; i++) {
		unsigned index = g_traces[i].addr % table_size;
		bool prediction = history_table[index] >= WEAK_YES;
		if (prediction == g_traces[i].actual) {
			correct++;
		}

		if (g_traces[i].actual && history_table[index] <= WEAK_YES) {
			history_table[index]++;
		} else if (!g_traces[i].actual && history_table[index] >= WEAK_NO) {
			history_table[index]--;
		}
	}

	params->correct = correct;
	free(history_table);
	return NULL;
}

void *simulate_gshare(void *arg) {
	TParams *params = (TParams *)arg;
	int history_size = params->history_size;
	unsigned char history_table[2048];
	unsigned ghr = 0, correct = 0;

	memset(history_table, STRONG_YES, 2048);

	for (unsigned i = 0; i < g_traces_count; i++) {
		unsigned index = (g_traces[i].addr % 2048) ^ ghr;
		bool prediction = history_table[index] >= WEAK_YES;
		if (prediction == g_traces[i].actual) {
			correct++;
		}

		if (g_traces[i].actual && history_table[index] <= WEAK_YES) {
			history_table[index]++;
		} else if (!g_traces[i].actual && history_table[index] >= WEAK_NO) {
			history_table[index]--;
		}

		ghr = ((ghr << 1) + g_traces[i].actual) & ~(0xFFFF << history_size);
	}

	params->correct = correct;
	return NULL;
}

void *simulate_tournament(void *arg) {
	TParams *params = (TParams *)arg;
	unsigned char gshare[2048], bimodal[2048], selector[2048];
	unsigned ghr = 0, correct = 0, history_size = 11;

	memset(gshare, STRONG_YES, 2048);
	memset(bimodal, STRONG_YES, 2048);
	memset(selector, PREFER_GSHARE, 2048);

	for (unsigned i = 0; i < g_traces_count; i++) {
		unsigned g_index = (g_traces[i].addr % 2048) ^ ghr;
		unsigned b_index = g_traces[i].addr % 2048;

		bool gshare_correct = (gshare[g_index] >= WEAK_YES) == g_traces[i].actual;
		bool bimodal_correct = (bimodal[b_index] >= WEAK_YES) == g_traces[i].actual;

		if (gshare_correct) {
			correct += selector[b_index] <= WEAK_PREFER_GSHARE;
		} else if (bimodal_correct) {
			correct += selector[b_index] >= WEAK_PREFER_BIMODAL;
		}

		if (bimodal_correct != gshare_correct) {
			if (bimodal_correct && selector[b_index] <= WEAK_PREFER_BIMODAL) {
				selector[b_index]++;
			} else if (gshare_correct && selector[b_index] >= WEAK_PREFER_GSHARE) {
				selector[b_index]--;
			}
		}

		ghr = ((ghr << 1) + g_traces[i].actual) & ~(0xFFFF << history_size);

		if (g_traces[i].actual) {
			if (gshare[g_index] <= WEAK_YES) gshare[g_index]++;
			if (bimodal[b_index] <= WEAK_YES) bimodal[b_index]++;
		} else {
			if (gshare[g_index] >= WEAK_NO) gshare[g_index]--;
			if (bimodal[b_index] >= WEAK_NO) bimodal[b_index]--;
		}
	}

	params->correct = correct;
	return NULL;
}

void *simulate_btb(void *arg) {
	TParams *params = (TParams *)arg;
	bool history_table[512];
	uint64_t btb_table[512] = {0};
	unsigned correct = 0, attempted = 0;

	memset(history_table, true, sizeof(history_table));

	for (unsigned i = 0; i < g_traces_count; i++) {
		unsigned index = g_traces[i].addr % 512;

		if (history_table[index]) {
			attempted++;
			if (g_traces[i].target == btb_table[index]) {
				correct++;
			}
		}

		history_table[index] = g_traces[i].actual;
		if (g_traces[i].actual) {
			btb_table[index] = g_traces[i].target;
		}
	}

	params->attempted = attempted;
	params->correct = correct;
	return NULL;
}

int main(int argc, char *argv[]) {
	if (argc != 3) {
		fprintf(stderr, "Usage: predictors input_trace.txt output.txt\n");
		return EXIT_FAILURE;
	}

	FILE *input = fopen(argv[1], "r");
	FILE *output = fopen(argv[2], "w");

	if (!input || !output) {
		fprintf(stderr, "Failed to open input or output file.\n");
		return EXIT_FAILURE;
	}
	char behavior[10];
	g_traces = malloc(25000100 * sizeof(BranchTrace));
	while (fscanf(input, "%llx %10s %llx\n", &g_traces[g_traces_count].addr, behavior, &g_traces[g_traces_count].target) != EOF) {
		g_traces[g_traces_count].actual = (strcmp(behavior, "T") == 0);
		g_traces_count++;
	}

	// Initializing thread structures here and start simulation logic similarly to the original code.

	free(g_traces);
	fclose(input);
	fclose(output);

	return EXIT_SUCCESS;
}
