#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>
/*在多线程编程中实现一个屏障（Barrier）功能，要求所有参与的线程必须等待，
直到所有其他线程也到达这个点才能继续执行。屏障在并行编程中常用于同步线程，
确保所有线程在关键点达成一致的状态后再继续执行下一步操作。*/
//利用 pthread 提供的条件变量方法，实现同步屏障机制。
static int nthread = 1;
static int round = 0;

struct barrier {
  pthread_mutex_t barrier_mutex;//互斥锁
  pthread_cond_t barrier_cond;//条件变量，线程间通信
  int nthread;      // 记录当前已到达屏障点的线程数量
  //当这个值等于所有参与线程的数量时，表示所有线程都已到达屏障。
  int round;     // 表示当前的屏障轮数。每次所有线程通过屏障后，轮数会递增，这有助于区分不同的同步阶段，
} bstate;

static void
barrier_init(void)
{
  assert(pthread_mutex_init(&bstate.barrier_mutex, NULL) == 0);
  assert(pthread_cond_init(&bstate.barrier_cond, NULL) == 0);
  bstate.nthread = 0;
}

/* 
   函数名称：barrier
   功能描述：一个线程同步的barrier函数，用于让多个线程在满足某个条件时同时继续执行。
   具体做法是，函数首先加锁，然后增加bstate.nthread的值。如果增加后bstate.nthread的值仍然小于线程总数nthread，
   则当前线程进入等待状态，否则执行以下操作：重置bstate.nthread为0，增加bstate.round的值，
   并通过pthread_cond_croadcast广播信号，通知所有等待的线程继续执行。最后解锁并退出函数。
*/

static void 
barrier()
{
  // 加锁，确保线程间同步
  pthread_mutex_lock(&bstate.barrier_mutex);
  
  // 如果当前线程数小于总线程数，则当前线程进入等待状态
  if(++bstate.nthread < nthread){
    pthread_cond_wait(&bstate.barrier_cond,&bstate.barrier_mutex);
  }
  else{
    // 当线程数达到预设总数时，重置线程数为0，递增round，广播通知所有等待线程
    bstate.nthread = 0;
    bstate.round++;
    pthread_cond_cbroadcast(&bstate.barrier_cond);
  }
  
  // 解锁，使其他线程能够继续执行
  pthread_mutex_unlock(&bstate.barrier_mutex);
}

static void *
thread(void *xa)
{
  long n = (long) xa;
  long delay;
  int i;

  for (i = 0; i < 20000; i++) {
    int t = bstate.round;
    assert (i == t);
    barrier();
    usleep(random() % 100);
  }

  return 0;
}

int
main(int argc, char *argv[])
{
  pthread_t *tha;
  void *value;
  long i;
  double t1, t0;

  if (argc < 2) {
    fprintf(stderr, "%s: %s nthread\n", argv[0], argv[0]);
    exit(-1);
  }
  nthread = atoi(argv[1]);
  tha = malloc(sizeof(pthread_t) * nthread);
  srandom(0);

  barrier_init();

  for(i = 0; i < nthread; i++) {
    assert(pthread_create(&tha[i], NULL, thread, (void *) i) == 0);
  }
  for(i = 0; i < nthread; i++) {
    assert(pthread_join(tha[i], &value) == 0);
  }
  printf("OK; passed\n");
}
