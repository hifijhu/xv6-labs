#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

int main(int argc, char *argv[])
{
    char *xargv[MAXARG];
    int xtarget;
    char buf[1];
    int flag = 1;


    if (argc <= 1)
    {
        fprintf(2, "Usage: xargs files...\n");
        exit(1);
    }
    if (argc > 1)
    {
        xtarget = argc - 1;
        for (int i = 0; i < xtarget; i++)
        {
            xargv[i] = argv[i + 1];
            
        }
    }

    
    while (1 == 1)
    {
        
        int pid = fork();
        if (pid == 0)
        {
            xargv[xtarget] = malloc(sizeof(char *));
            char *p = xargv[xtarget];
            while (read(0, buf, sizeof(buf)) > 0)
            {
                

                if (strcmp(buf,"\n")==0)
                {
                    printf("exec ");
                    exec(argv[1], xargv);
                    exit(0);
                }
                else if (strcmp(buf," ")==0)
                {

                    xtarget++;
                    xargv[xtarget] = malloc(sizeof(char *));
                    p = xargv[xtarget];

                }
                else
                {

                    memcpy(p, buf, sizeof(buf));
                    p = p + 1;
  
                    continue;
                }
            }
            flag = 0;
            exec(argv[1], xargv);
        }
        else
        {
            wait(0);
            if (flag == 0)
                break;
            else
                continue;
        }
    }
    
    exit(0);
}