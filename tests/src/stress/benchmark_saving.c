#include "benchmark_saving.h"

#include <core/util/logger.h>

#include <sys/stat.h>
#include <time.h>


// TODO: Next up is finishing the input and actually writing results to files. 🫩
void create_dir(char* )
{
    // Create benchmarks dir worst case scenario
    mkdir("../benchmarks", 0755);

    // Create benchmark directory
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char dirname[256];
    snprintf(dirname, sizeof(dirname),
             "../benchmarks/%04d_%02d_%02d_%02d%02d%02d",
             t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
             t->tm_hour, t->tm_min, t->tm_sec);

    mkdir(dirname, 0755);
}

void write_benchmark(benchmark_result *benchmark_result)
{
    create_dir();

    char date[256];


    // Write .md
    FILE *file = fopen("benchmarks/report.md", "w");
    if (!file)
    {
        LOG_ERROR("Failed to open file for writing");
        return;
    }

    printf("Please provide short summary of key differences between this and the last regarding results.");
    char response[512] = {0};
    fgets(response, sizeof(response), stdin);
    while (response[511] != "\0")
    {
        
    }

    fprintf(file, "# Report - v0.1.0\n");
    fprintf(file, "## Summary:\n");

    
    

    fprintf(file, "- Total Requests: %d (%.2f req/sec)\n", total_requests, rate);
    

    // Write .csv
}