/*
 * File         : time.c
 * Description  : User program to time process execution
 * Author       : Alexander DuPree
 * Date         : 13 Jan 2019
*/

#include "types.h"
#include "user.h"

int
main(int argc, char* argv[])
{
    int start_ticks = uptime();
    int pid = fork();

    if(pid < 0)
    {
        printf(2, "TIME: Fork error occured. Exiting.");
    }
    else if(pid == 0) // Child
    {
        // Execute command, no need to check RC.
        exec(argv[1], argv + 1);
    }
    else
    {
        if(wait() < 0) 
        { 
            printf(2, "TIME: Wait error occured");
        }
        else
        {
            int elapsed = uptime() - start_ticks;
            int seconds = elapsed / 1000;
            int ms      = elapsed % 1000;

            printf(1, "%s ran in %d.%ds\n", argv[1], seconds, ms);
        }
    }
    exit();
}
