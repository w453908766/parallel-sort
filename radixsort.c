
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "radixsort.h"

struct thread_context context;

unsigned* getCountAddress(unsigned thread_index, long mask){
  return &context.countMatrix[BUCKET_COUNT * thread_index + mask];
}

void count(struct thread_info* info, unsigned bit_pos){
  unsigned* counts = getCountAddress(info->index, 0);
  memset(counts, 0, sizeof(unsigned) * BUCKET_COUNT);
  ArrayT arr = info->currentArray;
  for (long* p = arr.start; p < arr.start + arr.length; p++) {
    long mask = (*p >> bit_pos) & BUCKET_BITMASK;
    counts[mask]++;
  }
}

void getIndices(struct thread_info* info, unsigned indices[BUCKET_COUNT]){
  memset(indices, 0, BUCKET_COUNT * sizeof(unsigned));
  unsigned index = info->index;
  for(unsigned i = 0; i < index; i++){
    unsigned* counts = getCountAddress(i, 0);
    for(unsigned k = 0; k < BUCKET_COUNT; k++){
      indices[k] += counts[k];
    }
  }
  for(unsigned i = index; i < context.threadNum; i++){
    unsigned* counts = getCountAddress(i, 0);
    for(unsigned k = 0; k < BUCKET_COUNT - 1; k++){
      indices[k+1] += counts[k];
    }
  }
  for(unsigned k = 0; k < BUCKET_COUNT - 1; k++){
    indices[k+1] += indices[k];
  }
}

void moveValues(struct thread_info* info, unsigned bit_pos, unsigned indices[BUCKET_COUNT]){
  /* Move values to correct position. */
  ArrayT arr = info->currentArray;
  for (long* p = arr.start; p < arr.start + arr.length; p++) {
    long mask = (*p >> bit_pos) & BUCKET_BITMASK;
    unsigned index = indices[mask];
    info->brr[index] = *p;
    indices[mask]++;
  }
}

void swapSrcDest(struct thread_info* info){
  unsigned index = info->index;
  unsigned elemNum = context.elemNum;
  unsigned threadNum = context.threadNum;

  unsigned len = elemNum / threadNum;
  unsigned start = index * len;

  info->currentArray.start = info->brr + start,
  info->currentArray.length = index == threadNum - 1 ? elemNum - start : len;

  long* tmp = info->arr;
  info->arr = info->brr;
  info->brr = tmp;
}

void* radix_sort_thread(void* arg){
  struct thread_info* info = (struct thread_info*)arg;
  unsigned indices[BUCKET_COUNT];

  for (unsigned bit_pos = 0; bit_pos < BITS; bit_pos += BUCKET_BITS) {
    count(info, bit_pos);

    pthread_barrier_wait(&context.barrier);
    getIndices(info, indices);
    moveValues(info, bit_pos, indices);
    pthread_barrier_wait(&context.barrier);
    swapSrcDest(info);
  }
  return NULL;
}

long* radix_sort1(long* Arr, long* Brr, ArrayT* inputs, unsigned elemNum, unsigned threadNum) {

  unsigned countMatrix[BUCKET_COUNT * threadNum];

  context.elemNum = elemNum;
  context.threadNum = threadNum;
  context.countMatrix = countMatrix;

  pthread_barrier_init(&context.barrier, NULL, threadNum);

  pthread_t thread_handles[threadNum];
  struct thread_info args[threadNum];
  for (unsigned i = 0; i < threadNum; i++) {
    struct thread_info p = {
      .arr = Arr,
      .brr = Brr,
      .index = i,
      .currentArray = inputs[i]
    };
    args[i] = p;
    pthread_create(&thread_handles[i], NULL, radix_sort_thread, (void *)(args+i));
  }

  for (unsigned i = 0; i < threadNum; i++) {
    pthread_join(thread_handles[i], NULL);
  }

  pthread_barrier_destroy(&context.barrier);

  return BITS / BUCKET_BITS % 2 == 0 ? Arr : Brr;
}

long* radix_sort(long* Arr, long* Brr, unsigned elemNum, unsigned threadNum) {
  ArrayT arrs[threadNum];
  unsigned len = elemNum / threadNum;
  for (unsigned i = 0; i < threadNum; i++) {
    unsigned start = i * len;
    arrs[i].start = Arr + start;
    arrs[i].length = i == threadNum - 1 ? elemNum - start : len;
  }
  return radix_sort1(Arr, Brr, arrs, elemNum, threadNum);
}