/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    int bsize;
    if(M==64&&N==64){
        bsize=4;
        int tmp_1=0,tmp_2=0,tmp_3=0,tmp_4=0,tmp_5,tmp_6,tmp_7,tmp_8;
        for( int ii=0;ii<M;ii+=bsize*2){
            for (int jj=0;jj<N;jj+=bsize*2){
                 //第一步，先把A的上面移动到B的上面(注意B的上面的坐标),并且左上要翻转
                for (int i=ii;i<ii+4;++i){
                        tmp_1=A[i][jj];
                        tmp_2=A[i][jj+1];
                        tmp_3=A[i][jj+2];
                        tmp_4=A[i][jj+3];

                        tmp_5=A[i][jj+4];
                        tmp_6=A[i][jj+5];
                        tmp_7=A[i][jj+6];
                        tmp_8=A[i][jj+7];

                        B[jj][i]=tmp_1; // 左上翻转
                        B[jj+1][i]=tmp_2;
                        B[jj+2][i]=tmp_3;
                        B[jj+3][i]=tmp_4;

                        B[jj][i+4]=tmp_5;
                        B[jj+1][i+4]=tmp_6;
                        B[jj+2][i+4]=tmp_7;
                        B[jj+3][i+4]=tmp_8;
                    
                }
                //把B的右上移动到B的左下，还要把A的左下移动到B的右上角
                for (int j=jj;j<jj+4;++j){
                        tmp_5=A[ii+4][j];
                        tmp_6=A[ii+5][j];
                        tmp_7=A[ii+6][j];
                        tmp_8=A[ii+7][j];//A的左下移动到B的右上角

                        tmp_1=B[j][ii+4];
                        tmp_2=B[j][ii+5];
                        tmp_3=B[j][ii+6];
                        tmp_4=B[j][ii+7];//右上移动到左下

                        

                        B[j][ii+4]=tmp_5;
                        B[j][ii+5]=tmp_6;
                        B[j][ii+6]=tmp_7;
                        B[j][ii+7]=tmp_8;
                        
                        B[j+4][ii]=tmp_1;
                        B[j+4][ii+1]=tmp_2;
                        B[j+4][ii+2]=tmp_3;
                        B[j+4][ii+3]=tmp_4;
                }

                // 第三步，A的右下转移到B的右下
                for (int i=ii+4;i<ii+8;++i){
                    tmp_1=A[i][jj+4];
                    tmp_2=A[i][jj+5];
                    tmp_3=A[i][jj+6];
                    tmp_4=A[i][jj+7];

                    B[jj+4][i]=tmp_1;
                    B[jj+5][i]=tmp_2;
                    B[jj+6][i]=tmp_3;
                    B[jj+7][i]=tmp_4;
                }
            }
        }
        
    }else if(M==32&&N==32){
        bsize=8;
        int tmp_1=0,tmp_2=0,tmp_3=0,tmp_4=0,tmp_5=0,tmp_6=0,tmp_7=0,tmp_8=0;
        for (int ii=0;ii<M;ii+=bsize){
            for (int jj=0;jj<N;jj+=bsize){
                for(int i=ii;i<ii+bsize;++i){
                        tmp_1=A[i][jj];
                        tmp_2=A[i][jj+1];
                        tmp_3=A[i][jj+2];
                        tmp_4=A[i][jj+3];
                        tmp_5=A[i][jj+4];
                        tmp_6=A[i][jj+5];
                        tmp_7=A[i][jj+6];
                        tmp_8=A[i][jj+7];

                        B[jj][i]=tmp_1;
                        B[jj+1][i]=tmp_2;
                        B[jj+2][i]=tmp_3;
                        B[jj+3][i]=tmp_4;
                        B[jj+4][i]=tmp_5;
                        B[jj+5][i]=tmp_6;
                        B[jj+6][i]=tmp_7;
                        B[jj+7][i]=tmp_8;
                    }
                }
            }
        }else{
            bsize=16;


             for (int ii=0;ii<N;ii+=bsize){
                for (int jj=0;jj<M;jj+=bsize){
                    for (int i=ii;i<ii+bsize&&i<N;++i){
                        for (int j=jj;j<jj+bsize&&j<M;++j){
                            B[j][i]=A[i][j];
                        }
                    }
                }
             }
        }
    }

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[j][i];
            B[j][i] = tmp;
        }
    }    

}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[j][i] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

