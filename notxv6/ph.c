#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>

#define NBUCKET 5
#define NKEYS 100000

struct entry {
  int key;
  int value;
  struct entry *next;
};
struct entry *table[NBUCKET];
pthread_mutex_t locks[NBUCKET];
int keys[NKEYS];
int nthread = 1;
pthread_mutex_t locks[NBUCKET];//c语言的互斥锁,对每个桶加锁，#include <pthread.h>

double
now()
{
 struct timeval tv;
 gettimeofday(&tv, 0);
 return tv.tv_sec + tv.tv_usec / 1000000.0;
}

static void 
insert(int key, int value, struct entry **p, struct entry *n)
{
  struct entry *e = malloc(sizeof(struct entry));
  e->key = key;
  e->value = value;
  e->next = n;
  *p = e;
}

/*这个函数是一个线程安全的哈希表put操作的实现。函数使用静态定义，表示其作用范围仅限于该函数所在的源文件内。

函数接受两个参数：key和value，表示要插入的键值对。函数首先计算key的哈希值，并根据哈希值对哈希表的桶进行索引。
然后，函数对相应的桶加锁，以确保在多线程环境下的线程安全。

接下来，函数遍历桶中的链表，检查是否存在与给定key相等的键。如果找到相等的键，函数会更新该键对应的值为value。

如果在链表中没有找到相等的键，函数会将新的键值对插入到链表的末尾。插入操作通过调用insert函数实现，
insert函数的细节没有给出，但可以推测它负责创建一个新的entry结构体，并将其插入到链表中。

最后，函数释放对桶的锁，完成操作。*/
static 
void put(int key, int value)
{
 NBUCKET;
  
  int i = key % NBUCKET;
  pthread_mutex_lock(&locks[i]);//对每个桶加锁
  // is the key already present?
  struct entry *e = 0;
  for (e = table[i]; e != 0; e = e->next) {
    if (e->key == key)
      break;
  }
  if(e){
    // update the existing key.
    e->value = value;
  } else {
    // the new is new.
    insert(key, value, &table[i], table[i]);
  }
  pthread_mutex_unlock(&locks[i]);
}
/*该函数是一个静态函数，其功能是在哈希表中根据给定的key值查找对应的entry。

首先，根据key值计算出哈希桶的索引i。
然后，使用pthread_mutex_lock函数对相应的锁进行加锁，以保证线程安全。
接着，使用循环遍历哈希桶中的链表，查找是否存在key值相同的entry。如果找到，则跳出循环。
最后，使用pthread_mutex_unlock函数对锁进行解锁，并返回找到的entry或0（如果未找到）。*/
static struct entry*
get(int key)
{
  int i = key % NBUCKET;
  pthread_mutex_lock(&locks[i]);

  struct entry *e = 0;
  for (e = table[i]; e != 0; e = e->next) {
    if (e->key == key) break;
  }
  pthread_mutex_unlock(&locks[i]);
  return e;
}

static void *
put_thread(void *xa)
{
  int n = (int) (long) xa; // thread number
  int b = NKEYS/nthread;

  for (int i = 0; i < b; i++) {
    put(keys[b*n + i], n);
  }

  return NULL;
}

static void *
put_thread(void *xa)
{
  int n = (int) (long) xa; // thread number
  int b = NKEYS/nthread;

  for (int i = 0; i < b; i++) {
    put(keys[b*n + i], n);
  }

  return NULL;
}

static void *
get_thread(void *xa)
{
  int n = (int) (long) xa; // thread number
  int missing = 0;

  for (int i = 0; i < NKEYS; i++) {
    struct entry *e = get(keys[i]);
    if (e == 0) missing++;
  }
  printf("%d: %d keys missing\n", n, missing);
  return NULL;
}

int
main(int argc, char *argv[])
{
  pthread_t *tha;
  void *value;
  double t1, t0;
  for(int i = 0; i < NBUCKET; i++){
    pthread_mutex_init(&locks[i],NULL);//初始化互斥锁
  }
  if (argc < 2) {
    fprintf(stderr, "Usage: %s nthreads\n", argv[0]);
    exit(-1);
  }
  nthread = atoi(argv[1]);
  tha = malloc(sizeof(pthread_t) * nthread);
  srandom(0);
  assert(NKEYS % nthread == 0);
  for (int i = 0; i < NKEYS; i++) {
    keys[i] = random();
  }
  
  for(int i=0;i<NBUCKET;i++) {
    pthread_mutex_init(&locks[i], NULL); 
  }

  //
  // first the puts
  //
  t0 = now();
  for(int i = 0; i < nthread; i++) {
    assert(pthread_create(&tha[i], NULL, put_thread, (void *) (long) i) == 0);
  }
  for(int i = 0; i < nthread; i++) {
    assert(pthread_join(tha[i], &value) == 0);
  }
  t1 = now();

  printf("%d puts, %.3f seconds, %.0f puts/second\n",
         NKEYS, t1 - t0, NKEYS / (t1 - t0));

  //
  // now the gets
  //
  t0 = now();
  for(int i = 0; i < nthread; i++) {
    assert(pthread_create(&tha[i], NULL, get_thread, (void *) (long) i) == 0);
  }
  for(int i = 0; i < nthread; i++) {
    assert(pthread_join(tha[i], &value) == 0);
  }
  t1 = now();

  printf("%d gets, %.3f seconds, %.0f gets/second\n",
         NKEYS*nthread, t1 - t0, (NKEYS*nthread) / (t1 - t0));
}
