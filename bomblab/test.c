#include<stdio.h>
int main(){
    int count = 0;
    int n=3;
for (int i = 1; i <= n; i++)
for (int j = 1; j <= i; j++)
for (int k = 1; k <= j; k++)
count++;
printf("count= %d\n",count);
}