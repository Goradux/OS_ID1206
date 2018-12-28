#include <assert.h>
#include <stddef.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include <time.h>

#include <stdbool.h>

#include "rand.c"

#define MIN 5
#define LEVELS 8
#define PAGE 4096

int request();

int compare (const void * a, const void * b) {
  return (*(int*)a - *(int*)b);
}

int blocks4k = 0;
int fourKblocks = 0;

enum flag {Free, Taken};

struct head *flists[LEVELS] = {NULL};

struct head {
  enum flag status;
  short int level;
  struct head *next;
  struct head *prev;
};


//has to be changed later to handle larger params
struct head *new() {
  struct head *new = (struct head *) mmap(NULL,
                                          PAGE,
                                          PROT_READ | PROT_WRITE,
                                          MAP_PRIVATE | MAP_ANONYMOUS,
                                          -1,
                                          0);
  if (new == MAP_FAILED) {
    return NULL;
  }
  assert(((long int)new & 0xfff) == 0);
  new->status = Free;
  new->level = LEVELS - 1;
  return new;
}

//whos ur buddy
struct head *buddy(struct head* block) {
  int index = block->level;
  long int mask = 0x1 << (index + MIN);
  return (struct head*)((long int)block ^ mask);
}

//merge two buddies
/*
struct head *merge(struct head* block, struct head* sibling) {
  struct head* primary;
  if (sibling < block)
    primary = sibling;
  else
    primary - block;
  primary->level = primary->level + 1;
  return primary;
}
*/

struct head *primary(struct head* block) {
  int index = block -> level;
  long int mask = 0xffffffffffffffff << (1 + index + MIN);
  return (struct head*)((long int)block & mask);
}

/*
struct head *split(struct head *block, int index) {
  int mask = 0x1 << (index + MIN);
  return (struct head *)((long int) block | mask);
}
*/

struct head *split(struct head *block) {
  int index = block->level - 1;
  int mask = 0x1 << (index + MIN);
  return (struct head *)((long int)block | mask);
}

//the magic
void *hide (struct head* block) {
  return (void*)(block + 1);
}

struct head *magic(void *memory) {
  return ((struct head*)memory - 1);
}

//level
int level(int req) {
  int total = req + sizeof(struct head);

  int i = 0;
  int size = 1 << MIN;
  while (total > size) {
    size <<= 1;
    i += 1;
  }
  return i;
}

//implement a test() function

//create a new block
// divide it in two
// divide it in two
// hide its header
// find its header using magic
//merge two blocks

/*
void levelUp(int index){
  if (flists[index] != NULL) {
    split(flists[index]);
    find(index-1);
  } else {
    levelUp(index+1);
  }
}
*/

void checkIfTaken(int index) {
  if (flists[index] -> status == Taken) {
    flists[index] = flists[index] -> next;
  }
}


int *findblock(int index){
    if (index < LEVELS) {
      if(flists[index] != NULL){
        struct head *block1 = flists[index];
        struct head *block2 = split(block1);
        //Now we have to change the variables of each new block
        //block1;
        if(block1->next != NULL) {
          block1->next->prev = NULL;
          flists[index] = block1->next;
        } else {
          flists[index] = NULL;
        }
        block1->status = 0;
        block1->level = index - 1;
        block1->next = block2;
        block1->prev = NULL;
        //block2
        block2->status = 0;
        block2->level = index - 1;
        block2->next = NULL;
        block2->prev = block1;
        //Re-arrange the block;
        if(flists[index-1] != NULL){
            //add it to the end of the linked list of those blocks
        }else{
          flists[index - 1] = block1;//So it puts the first of the two blocks on the list but one level below
        }
        return NULL; //True
      }else{
        findblock(index + 1); //Goes to the next block to split
      }
    }
    else{
      struct head *block = new();
      //printf("Address of block: %p\n", block);
      //blocks4k++;
      fourKblocks++;
      flists[LEVELS-1] = block;
      //return 0; //We cant find a block / false
    }
}

struct head *find(int index) {
  if (index < LEVELS) {
    if (flists[index] != NULL) { //So if there exists a block for this index
          struct head *foundblock = flists[index];
          if (foundblock->next != NULL) {
            foundblock->next->prev = NULL; //Since the next block will be the first in the list
            flists[index] = foundblock->next;
          }else{
            flists[index] = NULL;
          }
          foundblock->status = 1;
          foundblock->next = NULL;
          foundblock->prev = NULL;
          return foundblock; //REturn the address of that empty block
        //if we get hear it means that there were no avaliable blocks
    }else{
        //If we end up here that means there was no empty blocks for that index
        //We now have to go up the list and find other blocks to split
        findblock(index + 1);
        return find(index);
    }
    return NULL;
  }//End of index
}


int init = 0;

struct head *find_M(int index) {

  if (init == 0) {
    flists[7] = new();
    init++;
  }

  for (int i = 1; i <= LEVELS - index; i++) {
    if (flists[index] != NULL) {
      struct head *block = flists[index];

      if ((block->next) != NULL) {
        block->next->prev = NULL;
        flists[index] = block-> next;
      } else {
        flists[index] = NULL;
      }
      block->status = Taken;
      block->next = NULL;
      block->prev = NULL;
      return block;
    } else
    if (flists[index + i] != NULL) {
      struct head *block = flists[index + i];
      struct head *block2 = split(block);

      if (block->next) {
        block->next->prev = NULL;
        flists[index + i] = block->next;
      } else {
        flists[index + i] = NULL;
      }

      short int level = block->level - 1;

      block->level = block->level - 1;
      block->status = Free;
      block->next = block2;
      block->prev = NULL;

      block2->level = block->level;
      block2->status = Free;
      block2->next = NULL;
      block2-> prev = block;
      flists[index + i - 1] = block;

      //printf("Block 1: %d, %d\n", block->status, block->level);
      //printf("Block 2: %d, %d\n", block2->status, block2->level);

      return find(index);
    } else {
      //flists[7] = new();

      //return find(index);
    }
  }
  return NULL;
}

int indexGlobal = -1;
bool indexGlobalChanged = false;

struct head *find_old_new(int index) {
  printf("check before %d\n", index);
  if (indexGlobalChanged == false) {
    indexGlobal = index;
    indexGlobalChanged = true;
    printf("indexGlobalChanged for %d\n", index);
  }
  if (flists[index] != NULL) {
    if (flists[index] -> status == Free) {

      printf("found one!\n");
      if (indexGlobal != index) {
        struct head *block2 = split(flists[index]);
        flists[index]->status = Free;
        block2->next = flists[index]->next;
        flists[index]->next = block2;
        block2->prev = flists[index];
        flists[index]->level = index - 1;
        block2->status = Free;
        block2->level = index - 1;
        return find(index-1);
      }
      flists[index] -> status = Taken;
      //flists[index] -> level = index;
      struct head *block = flists[index];
      indexGlobalChanged = false;
      printf("This is the final return %d, %d\n", block->status, block->level);
      return block;
    } else  //instead of this have a for loop
    if (flists[index] -> status == Taken) {
      //printf("choke? for %d\n", index);
      flists[index] = flists[index] -> next;

      return find(index);
    }
  } else
  {//not free not taken -> doesnt exist -> lvl up               lvl up?
    return find(index+1);
  }
}

struct head *find_old(int index) {
  printf("check before %d\n", index);
  if (flists[index] != NULL){
    for (int i = 0; i < sizeof(flists[index]); i++) {
      if (flists[index] != NULL){
        printf("%d\n", i);
        if (flists[index]->status == Free) {
          //this is reached GOOD!
          flists[index]->status = Taken;
          printf("found a slot at %d\n", index);
          return (struct head*)(flists[index]);
        } else {
          flists[index] = flists[index]-> next;
        }
      }
    }
  } else {
    while (1) {
      int indexOld = index;
      index++;
      if (flists[index] != NULL) {
        printf("Achievement!!!\n");
        flists[index-1] = split(flists[index]);
        //find(index-1);
      }
    }
    find(index-1);

    //
    // if (flists[index+1] != NULL){
    //   split(flists[index+1]);
    // } else {
    //   return find(index + 1);
    // }
  }
  printf("check after %d\n", index);    // <- this never reaches !good!
  return NULL;
}



//balloc(20) will allocate 20 to level 1
//balloc calls find???
// in test we just call balloc and it finds the
void *balloc(size_t size) {
  if (size==0){
    return NULL;
  }
  //level of 20 is 1
  int index=level(size);
  struct head *taken = find(index);
  return hide(taken);
}

/*
void mergeCheck(struct head *block) {
  struct head *buddyBlock = buddy(block);
  if (buddyBlock->status == Free) {
    block->level = block->level + 1;
    buddyBlock->level = buddyBlock->level + 1;
    //flists[block->level] = block;
    flists[block->level] = primary (block);
    //block = primary(block);
    //mergeCheck(block);
    mergeCheck(flists[block->level]);
  } else
  if (buddyBlock->status == Taken) {
    // do nothing
  }
}

void insert( struct head *block){
  block->status = Free;
  mergeCheck(block);
}
*/

void insert(struct head *block) {

  block->status = 0;
  if (block->level < LEVELS-1) {
  struct head *buddyBlock = buddy(block);

    if (buddyBlock->status == 0 && (buddyBlock->level == block->level)) {//IF there exists such a buddy

      //printf("The block we are finding a buddy for has the status: %d\n", block->status);
      //printf("The block we are finding a buddy for has the level: %d\n", block->level);
      //printf("status of the buddy: %d\n", buddyBlock->status);
      //printf("level of the buddy: %d\n\n", buddyBlock->level);

      int tempLevel = block->level;
      struct head *mergedBlock = primary(block);
      mergedBlock->level = tempLevel + 1;
      mergedBlock->status = 0;
      mergedBlock->prev = NULL;
      mergedBlock->next = NULL;
      flists[tempLevel] = NULL;
      insert(mergedBlock);
    }else{
      //Then we cant do anything other than putting it in the list
      block->next = NULL;
      block->prev = NULL;
      flists[block->level] = block;
    }
  }else{
    flists[LEVELS-1] = block;
  }
}


void insert_2(struct head *block) {
  //S block is the block we wanna Free
  //First we should set it to be free
  //Then we check if this block at this level have any buddies
  //If it has, merge this block with thta block, and increase the level and add it yp index+1 LEVEL
  //return this till it cabt find a buddy anymore and leave it

  //Making it Free
  block->status = 0;
  struct head *buddyBlock = buddy(block);
  if (block->level < 7) {
    if (buddyBlock->status == 0) {//IF there exists such a buddy
      int tempLevel = block->level;
      struct head *mergedBlock = primary(block);
      mergedBlock->level = tempLevel + 1;
      mergedBlock->status = 0;
      mergedBlock->prev = NULL;
      mergedBlock->next = NULL;
      flists[tempLevel] = NULL;
      insert(mergedBlock);
    }else{
      //Then we cant do anything other than putting it in the list
      block->next = NULL;
      block->prev = NULL;
      flists[block->level] = block;
    }
  }else{

    flists[7] = block;
  }
}

void bfree(void *memory) {
  if (memory != NULL) {
    struct head *block = magic(memory);
    insert(block);
  }
  return;
}


void checklist(void){
  for(int i = 7; i >= 0; i--){
    struct head *temp = flists[i];
    int blockCounter = 0;
    while (temp != NULL) {
      printf("block in flists[%d] has the level: %d, status: %d, next: %p, prev %p\n", i,temp->level, temp->status, temp->next, temp->prev);
      blockCounter++;
      temp = temp->next;
    }
    //printf("Index: %d has %d blocks  \n", i, blockCounter);
    blockCounter = 0;

  }
  printf("\n");
}

/*
int length_of_free() {
  int i = 0;
  struct head *next  = flists;
  while(next != NULL) {
    i++;
    next = next->next;
  }
  return i;
}
*/
void sizes(int *buffer, int max) {
  int j = 0;
  for (int i = 0; i < LEVELS; i++) {
    struct head *block  = flists[i];		//flist was changed to flists[something], what is something?
    while(block->next != NULL & (i < max)) {
    int bytes = pow(2, LEVELS) * 32;
    buffer[i] = bytes;
    j++;
    block = block->next;
    }
  }
}


void test() {
  //struct head *block = new();
  //printf("NEW status: %d, level: %d\n", block->status, block->level);
  //flists[7] = block;
  //struct head *hidden = hide(block);
  //printf("NEW status: %d, level: %d\n", block->status, block->level);
  //struct head *blockAlloc = balloc(2000);
  //checklist();

  int *check8_1 = balloc(8);
  printf("%d\n", *check8_1);
  printf("%p\n", check8_1);

  int *check8_2 = balloc(8);
  printf("%d\n", *check8_2);
  printf("%p\n", check8_2);

  int *check8_3 = balloc(8);
  printf("%d\n", *check8_3);
  printf("%p\n", check8_3);

  int *check8_4 = balloc(8);
  printf("%d\n", *check8_4);
  printf("%p\n", check8_4);

  int *check32_1 = balloc(32);
  printf("%d\n", *check32_1);
  printf("%p\n", check32_1);

  int *check32_2 = balloc(32);
  printf("%d\n", *check32_2);
  printf("%p\n", check32_2);

  checklist();
  printf("splitting done \n");


  bfree(check8_1);
  checklist();

  bfree(check8_2);
  checklist();

  bfree(check8_3);
  checklist();

  bfree(check8_4);
  checklist();

  bfree(check32_1);
  checklist();

  bfree(check32_2);
  checklist();


  //balloc(50);
  //if (*blockAlloc != NULL) {
  //printf("balloc status: %d, level: %d\n", blockAlloc->status, blockAlloc->level);
  //}
  //printf("balloc status: %d, level: %d\n", blockAlloc->status, blockAlloc->level);
  //struct head *block2 = split(block, 6);
  //struct head block2 = *new();
  //printf("SPLIT status: %d, level: %d\n", block2->status, block2->level);
  //struct head *block3 = buddy(block2);
  //printf("BUDDY status: %d, level: %d\n", block3->status, block3->level);

  printf("level of 20 should be 1: %d\n", level(20));



  //determine a level()
  //then balloc() with hide()

  //hide() from header to the data
  //magic goes up from data to the pointero

}
//int request();
void bench1(int rounds) {
  for (int i = 0; i<rounds; i++){
    int requestedBlockSize = request();
    int *p;
    p = (int *)balloc(requestedBlockSize);
    *p = requestedBlockSize;
    printf("%d\n", requestedBlockSize);
    bfree(p);
  }
}

void bench2 (int rounds) {
  int allocatedData[rounds];
  int usedDataOfAllocatedBlock[rounds];
  int amountof4KBlocks[rounds];
  FILE *fp;

  for (int i = 0; i < rounds; i++) {
    int requestedBlockSize = request();
    int *p;
    p = (int *)balloc(requestedBlockSize);

    if (i == 0) {
      allocatedData[0] = (int) pow (2, MIN + level(requestedBlockSize));
      usedDataOfAllocatedBlock[0] = requestedBlockSize;
      amountof4KBlocks[0] = fourKblocks;
    } else {
      allocatedData[i] = (int) pow (2,MIN + level(requestedBlockSize)) + allocatedData[i-1];
      usedDataOfAllocatedBlock[i] = requestedBlockSize + usedDataOfAllocatedBlock [ i -1 ];
      amountof4KBlocks[i] = fourKblocks;
    }

    //allocatedData[i] = (int) pow(2, MIN + level(requestedBlockSize));
    //usedDataOfAllocatedBlock[i] = requestedBlockSize;
    //amountof4KBlocks[i] =fourKblocks;
  }

  fp = fopen("resultsFinal.dat", "w");
  if (fp == NULL) {
    printf("cant open file for writing\n");
    exit(0);
  }

  for (int i = 0; i < rounds; i++) {
    fprintf(fp, "%d, %d, %d\n", allocatedData[i], usedDataOfAllocatedBlock[i], amountof4KBlocks[i]*4096 );
  }

  fclose(fp);
}

void bench3old(int iterations, int memory) {
  //FILE *fp;
  //fp = fopen("bench3", "w");
  double total_t;
  clock_t start_balloc = clock();
  for(int i = 0; i < iterations; i++) {
    int *ballocate = balloc(memory);
    bfree(ballocate);
  }
  clock_t end_balloc = clock();
  clock_t balloc_time = (end_balloc - start_balloc);
  //fclose(fp);
  total_t = (double)(balloc_time);
  printf("Total time taken by balloc: %f\n", total_t  );

  clock_t start_malloc = clock();
  for(int i = 0; i < iterations; i++) {
    int *mallocate = malloc(memory);
    free(mallocate);
  }
  clock_t end_malloc = clock();
  clock_t malloc_time = (end_malloc - start_malloc);
  //fclose(fp);
  total_t = (double)(malloc_time);
  printf("Total time taken by malloc: %f\n", total_t  );
}

void bench3(int iterations) {

  int blocks[8] = {32,64,100,220,480,1000,2000,4000};

  for (int y = 0; y < 8; y++) {

      double total_t;
      clock_t start_balloc = clock();
      for(int i = 0; i < iterations; i++) {
        int *ballocate = balloc(blocks[y]);
        bfree(ballocate);
      }
      clock_t end_balloc = clock();
      clock_t balloc_time = (end_balloc - start_balloc);
      //fclose(fp);
      total_t = (double)(balloc_time);
      printf("Total time taken by balloc for size %d: %f\n", blocks[y], total_t  );

      clock_t start_malloc = clock();
      for(int i = 0; i < iterations; i++) {
        int *mallocate = malloc(blocks[y]);
        free(mallocate);
      }
      clock_t end_malloc = clock();
      clock_t malloc_time = (end_malloc - start_malloc);
      //fclose(fp);
      total_t = (double)(malloc_time);
      printf("Total time taken by malloc for size %d: %f\n", blocks[y], total_t  );
  }

}

int main(int argc, char *argv[]) {
  //test();

  if (argc<3) {
    printf("usage <rounds> <type>\n");
    exit(1);
  }

  int rounds = atoi(argv[1]);
  int loop = atoi(argv[2]);
  //char *name = argv[3];

  //bench2(rounds);
  bench3(rounds);

}




//add a file buddy.h that holds a declaration of the test procedure

// write a main() procedure in a test.c
// compile link and run
