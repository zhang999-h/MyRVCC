#include<stdio.h>
#include<stdlib.h>
int main(int Argc,char **Argv){
    printf("  .globl main\n");
    
    printf("main:\n");

    printf("  li a0,%d\n",atoi(Argv[1]));

    printf("  ret\n");
}
