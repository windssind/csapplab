#include<signal.h>
#include<stdio.h>
void sigint_handler(int sig);
int main(){
    signal(SIGINT,sigint_handler);
    pause();  
}
void sigint_handler(int sig){
    printf("hello\n");
}