#include "general.h"

void BenchmarkInit();
void __BenchmarkStart(char *file_name, char *function_name, u32 line_number);
void __BenchmarkPoll(char *file_name, char *function_name, u32 line_number);
void __BenchmarkEnd(char *file_name, char *function_name, u32 line_number);

char *BenchmarkInfoToString();

#ifdef DEBUG
#define BenchmarkStart() __BenchmarkStart(__FILE__, __func__, __LINE__)
#define BenchmarkPoll() __BenchmarkPoll(__FILE__, __func__, __LINE__)
#define BenchmarkEnd() __BenchmarkEnd(__FILE__, __func__, __LINE__)
#else
#define BenchmarkStart()
#define BenchmarkPoll()
#define BenchmarkEnd()
#endif
