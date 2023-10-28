#include<stdio.h>
int isTmax(int x) {
  int y=((~x)+(~x));
  return !y;
}
int main(){
    printf("%d",isTmax(2147483647));
}