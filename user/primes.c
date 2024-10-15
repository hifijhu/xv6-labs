#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"



void prime(int p[2]){

    
    int buf = 0;
    int q[2];
    int curr_prime;
    int curr_num;
    char *endptr;
    pipe(q);

    close(p[1]);

    int n = read(p[0], (void *)&buf, sizeof(int));

    if(n > 0){
        curr_prime = buf;    
        printf("prime %d\n", curr_prime);
    }
    else{
        
        close(p[0]);
        close(q[0]);
        close(q[1]);
        exit(0);
    }

    while(read(p[0], &buf, sizeof(buf)) > 0){
        
        curr_num = buf;
    
        if(curr_num % curr_prime != 0){
            write(q[1], &buf, sizeof(buf));
        }
    }

    if(fork() == 0){

        close(p[0]);
        prime(q);
        
        
    }
    else{

        close(q[0]);
        close(q[1]);
        close(p[0]);

        wait(0);
    }
    exit(0);
    
}
int
main(int argc, char *argv[])
{
    
    int p[2];
    int nums[34];

    pipe(p);

    for(int i=2; i<36; i++){
        nums[i-2] = i;
    }



    if(fork() == 0){
        prime(p);
    }
    else{
        
        write(p[1], &nums, 34*sizeof(int));
        
        close(p[1]);
        wait(0);
        close(p[0]);
        
    }

    exit(0);
}