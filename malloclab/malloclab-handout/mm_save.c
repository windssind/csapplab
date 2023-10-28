/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7) // 这个可以得到头部 这个就是要让size大小为1-7的向上舍入到8,+7是不影响未进2位的


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))
#define nop 0

/*define some macros ,which will make our manipulation more clearly and concisely*/

/*Basic macros and constants*/
#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1<<12)
#define MINBLOCKSIZE 16

#define MAX(x,y) ((x)>(y)?(x):(y))

/*Pack a size and allocated bit into one word*/
#define PACK(size,allocated) ((size)|(allocated))

/*Read and write a word in address p*/
#define PUT(p,val) ((*(unsigned int *)(p))=(val))
#define GET(p)  (*(unsigned int *)(p)) 

/*Get size and allocated bit in address p*/
#define GET_SIZE(p)        ((GET(p))&(~0x7))
#define GET_ALLOCATED(p)   ((GET(p))&0x1)

/*Get header and footer of ptr p*/
#define  HDRP(p)  ((char *)(p)-WSIZE)
#define  FTRP(p)  ((char *)(p)+GET_SIZE(HDRP(p))-DSIZE)

/*Given block ptr bp,compute address of next and previous blocks*/
#define NEXT_BLKP(bp)  ((char *)(bp)+GET_SIZE((char *)(bp)-WSIZE))
#define PREV_BLKP(bp)  ((char *)(bp)-GET_SIZE((char *)(bp)-DSIZE))    


/*获得当前bp的前驱结点和后继结点*/
#define GET_PREV(bp)    (*(unsigned int *)((char *)(bp)))
#define GET_NEXT(bp)    (*(unsigned int *)((char *)(bp)+WSIZE))
/*修改空闲链表中的前后*/
#define SET_NEXT(bp,val) (*(unsigned int *)((bp+WSIZE))=(val))
#define SET_PREV(bp,val) (*(unsigned int *)((bp))=(val)) 

static char *head_listp;/*总是指向序言块的下一个块（即第一个有用的块）*/
static char *first_of_headlist;
/* 
 * mm_init - initialize the malloc package.
 */

/*creat a new empty block and extend the heap*/
/* 这个函数只有在两种情况下才会调用
    1.初始化了一个堆，需要创建一个新的空闲块
    2. 原有的空闲块空间不够了，需要创建一个新的空闲块
    */
static void *extend_heap(size_t words);

static void * which_head_list(size_t size);

static void * insert_block_into_list(void *bp,size_t size);

static void *find_fit(size_t size);
static void * place(void *bp,size_t size);

static void * delete_block(void *bp);

static void *coallesce(void *bp);

int check_block_marked_free_correctly();
/*int every_block_coalescing(){
    char *ptr=head_listp+WSIZE*2;
    while(ptr!=)// 这个应该怎么弄成终点呢？
}

int every_free_block_actually_in_list(){

}*/
static int mm_check(void);


int mm_init(void)/*TODO：完成mm_init函数*/
{
    /*设置序言块为已分配的8字节*/
    if ((head_listp=mem_sbrk(WSIZE*18))==(void *)-1){
        fprintf(stderr,"allocate head_listp failed\n");
        return 0;
    }
    for (int i=0;i<15;++i){
        PUT(head_listp+i*WSIZE,0);// 先预留出8个位置供链表
    }
    PUT(head_listp+WSIZE*15,PACK(DSIZE,1));
    PUT(head_listp+WSIZE*16,PACK(DSIZE,1));// 这两行是序言块
    PUT(head_listp+WSIZE*17,PACK(WSIZE,0));// 这一行是末尾块
    first_of_headlist=head_listp;
    head_listp+=WSIZE*16; // 这个是将序言块指向序言块的下一块（即中间）
    if(extend_heap(CHUNKSIZE/WSIZE)==NULL){
        return -1;
    }
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
/*malloc采用的是分离首次适配*/
/*又不能用数组，那该怎么办->*/
void *mm_malloc(size_t size)
{
    char *bp;
    int newsize=ALIGN(size+SIZE_T_SIZE); // 这个是对齐后的size长度 而空闲块只需要头部，脚部,prev和next指针
    newsize=(newsize<16)? 16: newsize; // 不足16字节的要分配16字节
    if(bp=(char *)find_fit(newsize)){
        place(bp,newsize);
    }else{
        bp=extend_heap(newsize/4);
        place(bp,newsize);
    }
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    if (ptr==NULL){ // 不要漏了这个
        return ;
    }
    size_t size=GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr),PACK(size,0));// 将块变为空闲块
    PUT(FTRP(ptr),PACK(size,0));
    insert_block_into_list(ptr,size);
    coallesce(ptr);// 将空闲块合并，合并之后再修改指针
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = GET_SIZE(HDRP(ptr));
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;

}

/*creat a new empty block and extend the heap*/
/* 这个函数只有在两种情况下才会调用
    1.初始化了一个堆，需要创建一个新的空闲块
    2. 原有的空闲块空间不够了，需要创建一个新的空闲块
    */
static void *extend_heap(size_t words){
    char * bp;
    size_t size;
    size= (words%2?words+1:words)*WSIZE;
    void *mem_brk=mem_sbrk(0);
    if (!GET_ALLOCATED(mem_brk-DSIZE)) size-=GET_SIZE(mem_brk-DSIZE);
    if((bp=mem_sbrk(size))==(void *)-1){ 
        printf("extend_heap failed\n");
        return NULL;
    }// “分配”这么多的位置给新要求增加的堆
    // 设置头部和脚部
    PUT(HDRP(bp),PACK(size,0));
    PUT(FTRP(bp),PACK(size,0));
    PUT(HDRP(NEXT_BLKP(bp)),PACK(0,1));// 结尾块以0/1结尾 // 这里是否会有越界的风险?没有，只是单纯的指针加减，还没有引用，因此不会触发越界
    insert_block_into_list(bp,GET_SIZE(HDRP(bp)));
    return coallesce(bp);
}

static void * which_head_list(size_t size){
    if (size<32){
        return first_of_headlist;
    }
    size=size>>5;
    int i=0;
    do{
        i+=1;
    }while ((size=(size>>1))&&i<14);// 找到应该是哪一个i作为偏置量
    return first_of_headlist+i*WSIZE;
    
}

static void * insert_block_into_list(void *bp,size_t size){
    /*void *head=(unsigned int *)which_head_list(size);
    unsigned int *_ptr=GET(head);
    if (_ptr==NULL){ // 链表中一个空闲块都没有
        PUT(head,bp);
        SET_PREV(bp,head);
        SET_NEXT(bp,NULL);
        return bp;
    }
    void *first_block=GET(head);
    if (size<=GET_SIZE(HDRP(first_block))){
        SET_PREV(bp,head);
        SET_NEXT(bp,first_block);
        PUT(head,bp);
        SET_PREV(first_block,bp);
        return bp;
    }

    while (_ptr!=NULL&&GET_NEXT(_ptr)!=NULL){
        if (GET_SIZE(HDRP(_ptr))<=size&&GET_SIZE(HDRP(GET_NEXT(_ptr)))>=size){ //要注意两个极端情况，就是插入的块比原链表中的所有块的大小都要大或者都要小
            // 修改bp的指针
            SET_NEXT(bp,GET_NEXT(_ptr));
            SET_PREV(bp,_ptr);
            //修改bp下一个的指针
            SET_PREV(GET_NEXT(_ptr),bp);
            //修改bp上一个的指针
            SET_NEXT(_ptr,bp);
            return bp;
        }
        _ptr=GET_NEXT(_ptr);
        //printf("%p\n",_ptr);
    }
    // 能走到这里说明没有找到合适的，就是插入的块比原链表所有块还大或者还小或者单纯只有1个块(其中ptr指向的是最后一个)
    if(size>=GET_SIZE(HDRP(_ptr))){
        SET_NEXT(_ptr,bp);
        SET_PREV(bp,_ptr);
        SET_NEXT(bp,NULL);
    return bp;

}*/
 if (bp == NULL)
        return;
    void* root = which_head_list(GET_SIZE(HDRP(bp)));
    void* prev = root;
    void* next = GET(root);

    while (next != NULL)
    {
        if (GET_SIZE(HDRP(next)) >= GET_SIZE(HDRP(bp))) break;
        prev = next;
        next = GET_NEXT(next);
    }

    if (prev == root)
    {
        PUT(root, bp);
        SET_PREV(bp, root);
        SET_NEXT(bp, next);
        if (next != NULL) SET_PREV(next, bp);
    }
    else
    {
        SET_PREV(bp, prev);
        SET_NEXT(bp, next);
        SET_NEXT(prev, bp);
        if (next != NULL) SET_PREV(next, bp);
    }
}

static void *find_fit(size_t size){ // 如果找到了，就返回空闲块的内存；如果遍历了所有空闲块都没找到，就返回NULL
    void * head=which_head_list(size);// 获取了选择哪个来寻找
    for (char *root=head;root!=head_listp-WSIZE;root+=WSIZE){
        char *bp=GET(root);
        while (bp!=NULL)
        {
            if (GET_SIZE(HDRP(bp))>=size){
                return bp;
            }
            bp=GET_NEXT(bp);
        } 
    }
    return NULL;
}
static void * place(void *bp,size_t size){ // 返回值是删除的块的指针
    size_t block_size=GET_SIZE(HDRP(bp));
    if (size==block_size){// 恰好相同
        // 让前面和后面的连接起来
        PUT(HDRP(bp),PACK(size,1));
        PUT(FTRP(bp),PACK(size,1));
        delete_block(bp);
        return bp;
    }else if(block_size-size>=MINBLOCKSIZE){ // 块比需要的大p 
        size_t rest_size=block_size-size;
        PUT(HDRP(bp),PACK(size,1));
        PUT(FTRP(bp),PACK(size,1));
        delete_block(bp); //?
        PUT(HDRP(NEXT_BLKP(bp)),PACK(rest_size,0));
        PUT(FTRP(NEXT_BLKP(bp)),PACK(rest_size,0));// 这里其实已经是place的事了
        // 再将分割出来的块插入到合适的链表中
        insert_block_into_list(NEXT_BLKP(bp),rest_size);
    }else{ // 如果分割，分割出来的块不满16字节->直接把多的那部分也送给用户
        PUT(HDRP(bp),PACK(block_size,1));
        PUT(FTRP(bp),PACK(block_size,1));
        delete_block(bp);
    }
}

static void * delete_block(void *bp){ // 双向链表的删除，还是很容易操作的;返回删除的块的指针
    if (bp==NULL){
        return NULL;
    }
    void *_pre=GET_PREV(bp);
    void *_next=GET_NEXT(bp);
    ((char *)_pre-first_of_headlist)<=0x38? PUT(_pre,_next) : SET_NEXT(_pre,_next);
    _next==NULL? nop : SET_PREV(_next,_pre);
    SET_PREV(bp,0);
    SET_NEXT(bp,0);
    return bp;
}// 如何自动创建函数的定义？

static void *coallesce(void *bp){// 函数调用时机：有新的空闲块生成，会调用这个函数进行空闲块的合并；返回合并后的空闲块的指针
    // 有个小bug：如果是extend之后调用的，那不就会出现越界错误？
        //在设计序言块和结尾块的时候已经帮我们考虑好这个问题了
    int pre_alloc=GET_ALLOCATED(FTRP(PREV_BLKP(bp)));
    int next_alloc=GET_ALLOCATED(HDRP(NEXT_BLKP(bp)));
    // case 1 前后都是分配
    if (pre_alloc&&next_alloc){
        return bp;
    }
    // case 2 前面分配，后面没有分配
    if (pre_alloc&&!next_alloc){
        size_t new_size=GET_SIZE(HDRP(bp))+GET_SIZE(HDRP(NEXT_BLKP(bp)));
        delete_block(NEXT_BLKP(bp));// 后面的块要删除了
        PUT(HDRP(bp),PACK(new_size,0));
        PUT(FTRP(bp),PACK(new_size,0));
        delete_block(bp); // 当前的块也要先删除
        insert_block_into_list(bp,new_size);
        return bp;
    }
    // case 3 前面没有分配，后面分配了
    if((!pre_alloc)&&next_alloc){
        char *new_bp=PREV_BLKP(bp);
        size_t new_size=GET_SIZE(HDRP(new_bp))+GET_SIZE(HDRP(bp));
        PUT(HDRP(new_bp),PACK(new_size,0));
        PUT(FTRP(new_bp),PACK(new_size,0));
        // 再更新链表的指针
        delete_block(bp);
        delete_block(new_bp);// 原有的两个空闲块都是在链表中的，需要删掉
        insert_block_into_list(new_bp,new_size);
        return new_bp;
    }
    // case 4 前后都没有分配
    if((!pre_alloc)&&(!next_alloc)){
        char *pre_bp=PREV_BLKP(bp);
        char *next_bp=NEXT_BLKP(bp);
        size_t new_size=GET_SIZE(HDRP(pre_bp))+GET_SIZE(HDRP(next_bp))+GET_SIZE(HDRP(bp));
        PUT(HDRP(pre_bp),PACK(new_size,0));
        PUT(FTRP(pre_bp),PACK(new_size,0));
        delete_block(pre_bp);
        delete_block(bp);
        delete_block(next_bp);
        insert_block_into_list(pre_bp,new_size);
        return pre_bp;
    }
}

int check_block_marked_free_correctly(){
    char *root=first_of_headlist;
    for (char *ptr=root;ptr!=head_listp-WSIZE;root+=WSIZE){
        // 开始一次遍历
        char *bp=GET(ptr);
        while (bp!=NULL)
        {
            if (!GET_ALLOCATED(HDRP(bp))){
                return 0;
            }
            bp=GET_NEXT(bp);
        }
    }
    return 1;// 保证在freeblocklist里面的所有的都成功标记为free了
    
}

/*int every_block_coalescing(){
    char *ptr=head_listp+WSIZE*2;
    while(ptr!=)// 这个应该怎么弄成终点呢？
}

int every_free_block_actually_in_list(){

}*/
static int mm_check(void){
    if(!check_block_marked_free_correctly()){
        fprintf(stderr,"check block in list marked correctly failed\n");
    }
}














