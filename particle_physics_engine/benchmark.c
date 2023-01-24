#include "benchmark.h"
#include "memory.h"
#include <stdio.h>

#define _AMD64_
#include <profileapi.h>

struct benchmark_node {
	char *file_name;
	char *function_name;
	u32 line_number;
	f64 time;
	struct benchmark_node *next_timing_node;
	struct benchmark_node *next_group;
};

typedef struct benchmark_node benchmark_node;

benchmark_node *finalized_nodes = 0;
benchmark_node *node_stack = 0;

u64 performance_frequency = {0};

static f64 GetElapsedTime() {
	LARGE_INTEGER pc = {0};
	QueryPerformanceCounter(&pc);
	return (f64)pc.QuadPart / (f64)performance_frequency;
}

void BenchmarkInit() {
	LARGE_INTEGER pf = {
		.QuadPart = 1
	};
	QueryPerformanceFrequency(&pf);
	performance_frequency = pf.QuadPart;
}

void __BenchmarkStart(char *file_name, char *function_name, u32 line_number) {
	
	benchmark_node *n = BumpAlloc(sizeof(benchmark_node));
	n->file_name = file_name;
	n->function_name = function_name;
	n->line_number = line_number;
	
	n->next_group = node_stack;
	node_stack = n;
	
	n->next_timing_node = 0;

	n->time = GetElapsedTime();
}

void __BenchmarkPoll(char *file_name, char *function_name, u32 line_number) {
	// NOTE: This has O(n) performance. it could be more efficient using a tail node or something like that
	
	f64 time_elapsed = GetElapsedTime();
	benchmark_node *n = node_stack;
	
	while (n->next_timing_node != 0) {
		n = n->next_timing_node;
	}

	n->next_timing_node = BumpAlloc(sizeof(benchmark_node));
	n->next_timing_node->file_name = file_name;
	n->next_timing_node->function_name = function_name;
	n->next_timing_node->line_number = line_number;
	n->next_timing_node->time = time_elapsed;
	n->next_timing_node->next_timing_node = 0;
}

void __BenchmarkEnd(char *file_name, char *function_name, u32 line_number) {
	__BenchmarkPoll(file_name, function_name, line_number);
	
	benchmark_node *n = node_stack;
	node_stack = node_stack->next_group;
	n->next_group = finalized_nodes;
	finalized_nodes = n;
}

char *BenchmarkInfoToString() {
	char *final_string = BumpAlloc(2048);
	char *c = final_string;
	
	benchmark_node *start = finalized_nodes;
	
	benchmark_node *n = start;
	while (start) {
		n = n->next_timing_node;
		while (n) {
			char *function_name = n->function_name;
			u32 line_number = n->line_number;
			f64 milliseconds = (n->time - start->time) * 1000.0;
			c += sprintf(c, "[ %s:%d-%d ] %.6f\n", function_name, start->line_number, line_number, milliseconds);
			n = n->next_timing_node;
		}
		start = start->next_group;
	};

	finalized_nodes = 0;
	node_stack = 0;
	
	*c = 0;
	return final_string;
}
