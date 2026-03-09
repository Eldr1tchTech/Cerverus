// This is the entry point for the stress test part of the testing.
// This will start the project (bin/project) and then a client, and
#include "stress/client_manager.h"
#include "stress/benchmark_saving.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <signal.h>s
#include <sys/wait.h>
#include <string.h>

void run_stress_tests()
{
    printf("Running stress tests.\n");
    printf("1. Start the server.\n");

    pid_t pid = fork();

    if (pid < 0)
    {
        printf("run_stress_tests - Fork failed: %s\n", strerror(errno));
        return;
    }
    else if (pid == 0)
    {
        // Child process - run the server
        execl("./project", "project", NULL);
        
        // If we get here, exec failed
        printf("run_stress_tests - Exec failed: %s\n", strerror(errno));
        _exit(1);
    }
    else
    {
        printf("2. Start the client.\n");

        client_manager* c_man = client_manager_create();
        if (!c_man) {
            printf("run_stress_tests - Failed to create client manager.\n");
            kill(pid, SIGTERM);  // Kill server before returning
            waitpid(pid, NULL, 0);
            return;
        }
        
        char* URIs[] = {"/index.html", "/style.css", "/architecture.html", "/features.html"};
        c_man->URIs = URIs;

        printf("3. Wait until server is running.\n");
        sleep(5);

        printf("4. Send requests and collect benchmark data.\n");
        benchmark_result* bm_r = client_manager_run(c_man);
        
        if (bm_r) {
            printf("6. Save results into a file.\n");
            write_benchmark(bm_r);
            destroy_benchmark_result(bm_r);
        } else {
            printf("Benchmark failed - no results to save.\n");
        }
        
        client_manager_destroy(c_man);
        
        // CRITICAL: Kill and wait for server process to finish
        printf("7. Shutting down server.\n");
        kill(pid, SIGTERM);
        waitpid(pid, NULL, 0);
        
        printf("Stress tests finished.\n");
    }
}