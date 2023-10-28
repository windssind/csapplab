#include "cachelab.h"
#include<stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include<math.h>
#include<string.h>
int hit_count=0;
int miss_count=0;
int eviction_count=0;
void Simulate_cache(int s,int E,int b,char *filename);
void execute_instruction(long *cache,int *valid_table,long *tag_table,char instruction_mode,int addr,int b,int s,int E,int *use_rate_table,int *is_full);
int  Load(long *cache,int *valid_table,long *tag_table,int index,int flag,int bias,int b,int E,int *use_rate_table,int *is_full);
int main(int argc ,char *argv[])
{
    int s,E,b;
    int result;
    char *filename;
    while((result=getopt(argc,argv,"vs:E:b:t:"))!=-1){
        switch (result)
        {
        case 'v':
            break;
        case 's':
            s=*optarg-48;
            break;
        case 'E':
            E=*optarg-48;
            break;
        case 'b':
            b=*optarg-48;
            break;
        case 't':
            filename=optarg;
            Simulate_cache(s,E,b,filename);
            break;
        default:
            break;
        }
    }
    printSummary(hit_count, miss_count, eviction_count);
    return 0;
}

void Simulate_cache(int s,int E,int b,char *filename){
    //先获取文件流
    FILE * f=fopen(filename,"r");
    if(f==NULL){
        fprintf(stderr,"open file fail\nfilename is %s",filename);
        exit(1);
    }
    //根据输入的s，E，b建立一个缓冲区
    int B=pow(2,b);
    int S=pow(2,s);
    long *cache=malloc(S*E*B);
    int *valid_table=(int *)malloc(sizeof(int)*E*S);//有效位表
    long *tag_table=(long *)malloc(sizeof(long)*S*E);// tag表
    int *use_rate_table=(int *)malloc(sizeof(int)*E*S);// 使用频率表单
    int *is_full=(int *)malloc(sizeof(int)*S);//判定某个缓冲区是否已经满了
    /*TOdo:完成分析一行然后修改cache valid table tagtable的函数*/
    char instruction_mode;
    unsigned int addr;
    int size;
    while((instruction_mode=fgetc(f))!=-1){
        instruction_mode=instruction_mode==32?fgetc(f):instruction_mode;// 如果是‘ ’的话再空出一位
        fgetc(f);// 去掉多余的空格符
        fscanf(f,"%x,%d",&addr,&size);//使用fscanf从文件流中获取addr和sieze的信息
        fgetc(f)=='\n'? 1:fgetc(f); // 去掉多余的换行符
        execute_instruction(cache,valid_table,tag_table,instruction_mode,addr,b,s,E,use_rate_table,is_full);
    }
}

void execute_instruction(long *cache,int *valid_table,long *tag_table,char instruction_mode,int addr,int b,int s,int E,int *use_rate_table,int *is_full){
    int index=((((unsigned)(addr<<(32-b-s)))>>(32-b-s))>>b);/*todo:char类型的一串数组转化为数字，然后再运用位操作变为index  这里目前有可能出错*/
    int flag=((unsigned)addr)>>(b+s);
    int bias=(unsigned)(addr<<(32-b))>>(32-b);
    int ret;
    char *tips[3]={"eviction","miss","hit"};
    switch (instruction_mode)
    {
    case 'I':
        break;
    case 'L':
    case 'S':
        ret=Load(cache,valid_table,tag_table,index,flag,bias,b,E,use_rate_table,is_full);
        if(ret ==1) hit_count+=1;
        else if (ret==0) {miss_count+=1;}
        else   {
            eviction_count+=1;miss_count+=1; 
        }
        printf("%c %x,%s\n",instruction_mode,addr,tips[ret+1]);
        break;
    case 'M':
        int num[2];
        for(int i=0;i<2;++i){
            ret=Load(cache,valid_table,tag_table,index,flag,bias,b,E,use_rate_table,is_full);
            if(ret ==1) hit_count+=1;
            else if (ret==0) {miss_count+=1;}
            else   {
                eviction_count+=1;miss_count+=1;
            }
            num[i]=ret;
        }
        printf("%c %x,%s %s\n",instruction_mode,addr,tips[num[0]+1],tips[num[1]+1]);
        break;
    default:
        break;
    }
    
    
}

int  Load(long *cache,int *valid_table,long *tag_table,int index,int flag,int bias,int b,int E,int *use_rate_table,int *is_full){
    //return 1 as hit ,else return 0
    //hit   /*后期如果速度有问题的话，可以优化这个寻找方法
    int i=0;
    int find_index=-1;
    for(;i<E;++i){
        if(*(valid_table+index*E+i)==1){// 先+1
            *(use_rate_table+index*E+i)+=1;
        }
        if(flag==*(tag_table+index*E+i)&&*(valid_table+index*E+i)==1){// hit了
            find_index=i;
        }
    }// 疑问：这里没有hit成功是否要+1,会不会影响最终的判断呢？如果是相同的（不会出现这种情况！！！）
    if(find_index!=-1){
        *(use_rate_table+index*E+find_index)=0;
        return 1;
    }
    //miss 有可能是满了，也有可能是冷不命中
    if(*(is_full+index)){//满了
        int max=*(use_rate_table+index*E);
        int max_index=0;
        for(int j=1;j<E;j++){//找最大的下标
            int now_val=*(use_rate_table+index*E+j);
            if(now_val>max){
                max=now_val;
                max_index=j;
            }
        }
        // 找到了，修改对应的cache，valid_table,tag_table,use_rate_table,is_full  ->实际要修改的只有tag_table和suerate——table
        *(use_rate_table+index*E+max_index)=0;
        *(tag_table+index*E+max_index)=flag;
        return -1;
    }

    //冷不命中
    for(int j=0;j<E;++j){
        if(*(valid_table+index*E+j)==0){//找到了第一个没有用的
            *(tag_table+index*E+j)=flag;
            *(use_rate_table+index*E+j)=0;
            *(valid_table+index*E+j)=1;
            if(j==E-1){
                *(is_full+index)=1;
            }
            return 0;
        }
    }
    return 0;
}

/* TO yi.trace文件有问题，eviction少了一个，明天check以下 具体：1.用-v找出正确的 2.画出图 3.再找出有问题的点进行debug*/