#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "e1000_dev.h"
#include "net.h"

#define TX_RING_SIZE 16
static struct tx_desc tx_ring[TX_RING_SIZE] __attribute__((aligned(16)));
static struct mbuf *tx_mbufs[TX_RING_SIZE];//发送（transmission）缓冲区环

#define RX_RING_SIZE 16
static struct rx_desc rx_ring[RX_RING_SIZE] __attribute__((aligned(16)));// 存储接收描述符，用于指示网卡如何处理接收到的数据
static struct mbuf *rx_mbufs[RX_RING_SIZE];//rx_mbufs 用于实际存储这些数据的缓冲区。

// remember where the e1000's registers live.
static volatile uint32 *regs;//volatile关键字指示该变量可能由外界影响改变，让编译器每次都从内存中读取变量，

struct spinlock e1000_lock;

// called by pci_init().
// xregs is the memory address at which the
// e1000's registers are mapped.
// 初始化e1000网络控制器
// 参数:
//   xregs: 指向e1000寄存器的32位指针数组
void
e1000_init(uint32 *xregs)
{
  int i;

  // 初始化锁，用于e1000的访问控制
  initlock(&e1000_lock, "e1000");

  // 设置寄存器指针
  regs = xregs;

  // 重置设备
  regs[E1000_IMS] = 0; // 关闭中断
  regs[E1000_CTL] |= E1000_CTL_RST; // 发送复位信号
  regs[E1000_IMS] = 0; // 再次关闭中断
  __sync_synchronize(); // 确保写操作完成

  // 初始化传输环缓冲区
  memset(tx_ring, 0, sizeof(tx_ring)); // 清零传输环缓冲区
  for (i = 0; i < TX_RING_SIZE; i++) {
    tx_ring[i].status = E1000_TXD_STAT_DD; // 设置数据已传输完成的状态
    tx_mbufs[i] = 0; // 清空相关的mbuf
  }
  regs[E1000_TDBAL] = (uint64) tx_ring; // 设置传输环缓冲区的底地址
  if(sizeof(tx_ring) % 128 != 0)
    panic("e1000"); // 确保传输环缓冲区大小是128的倍数
  regs[E1000_TDLEN] = sizeof(tx_ring); // 设置传输环缓冲区的长度
  regs[E1000_TDH] = regs[E1000_TDT] = 0; // 设置传输头部和尾部指针

  // 初始化接收环缓冲区
  memset(rx_ring, 0, sizeof(rx_ring)); // 清零接收环缓冲区
  for (i = 0; i < RX_RING_SIZE; i++) {
    rx_mbufs[i] = mbufalloc(0); // 分配接收mbuf
    if (!rx_mbufs[i])
      panic("e1000"); // 确保mbuf分配成功
    rx_ring[i].addr = (uint64) rx_mbufs[i]->head; // 设置mbuf的头部地址
  }
  regs[E1000_RDBAL] = (uint64) rx_ring; // 设置接收环缓冲区的底地址
  if(sizeof(rx_ring) % 128 != 0)
    panic("e1000"); // 确保接收环缓冲区大小是128的倍数
  regs[E1000_RDH] = 0; // 设置接收头部指针
  regs[E1000_RDT] = RX_RING_SIZE - 1; // 设置接收尾部指针
  regs[E1000_RDLEN] = sizeof(rx_ring); // 设置接收环缓冲区长度

  // 配置MAC地址过滤
  regs[E1000_RA] = 0x12005452; // 设置单播MAC地址的低位
  regs[E1000_RA+1] = 0x5634 | (1<<31); // 设置单播MAC地址的高位，并启用接收，(1<<31) 表示设置接收使能位
  //允许网络接口接收匹配此MAC地址的数据包。
  // 清空多播地址表
  //多播地址用于同时向多个设备发送数据包，
  for (int i = 0; i < 4096/32; i++)
    regs[E1000_MTA + i] = 0;

  // 配置传输控制
  regs[E1000_TCTL] = E1000_TCTL_EN | // 启用传输
    E1000_TCTL_PSP | // 启用短包填充
    (0x10 << E1000_TCTL_CT_SHIFT) | // 设置碰撞退避参数
    (0x40 << E1000_TCTL_COLD_SHIFT); // 设置最小帧间隔
  regs[E1000_TIPG] = 10 | (8<<10) | (6<<20); // 设置帧间的间隔时间

  // 配置接收控制
  regs[E1000_RCTL] = E1000_RCTL_EN | // 启用接收器
    E1000_RCTL_BAM | // 启用广播接收
    E1000_RCTL_SZ_2048 | // 设置接收缓冲区大小为2048字节
    E1000_RCTL_SECRC; // 启用自动CRC剥离
  
  // 配置接收中断
  regs[E1000_RDTR] = 0; // 关闭基于时间的接收中断
  regs[E1000_RADV] = 0; // 关闭基于帧数的接收中断
  regs[E1000_IMS] = (1 << 7); // 启用接收描述符写回中断
}
int
e1000_transmit(struct mbuf *m)
{
  acquire(&e1000_lock);// 获取 E1000 的锁，防止多进程同时发送数据出现 race
  uint32 ind = regs[E1000_TDT];// 下一个可用的 buffer 的下标
  struct tx_desc *desc = &tx_ring[ind]; // 获取 buffer 的描述符，其中存储了关于该 buffer 的各种信息
  // 如果该 buffer 中的数据还未传输完，则代表我们已经将环形 buffer 列表全部用完，缓冲区不足，返回错误
  if(!(desc->status & E1000_TXD_STAT_DD)){
    release(&e1000_lock);
    return -1;
  }

  // 如果该下标仍有之前发送完毕但未释放的 mbuf，则释放
  if(tx_mbufs[ind]){
    mbuffree(tx_mbufs[ind]);
    tx_mbufs[ind] = 0;
  }
  // 将要发送的 mbuf 的内存地址与长度填写到发送描述符中
  desc->addr = (uint64)m->head;
  desc->length = m->len;
  // 设置参数，EOP 表示该 buffer 含有一个完整的 packet
  // RS 告诉网卡在发送完成后，设置 status 中的 E1000_TXD_STAT_DD 位，表示发送完成。
  desc->cmd = E1000_TXD_CMD_EOP|E1000_TXD_CMD_RS;
   // 保留新 mbuf 的指针，方便后续再次用到同一下标时释放。
   tx_mbufs[ind] = m;
   // 环形缓冲区内下标增加一。
   regs[E1000_TDT] = (regs[E1000_TDT] + 1) % TX_RING_SIZE;//确保结果在0到TX_RING_SIZE-1的范围内超过就重置
   release(&e1000_lock);
  return 0;
}

static void
e1000_recv(void)
{
  while(1){// 每次 recv 可能接收多个包

    uint32 ind = (regs[E1000_RDT] + 1) % RX_RING_SIZE;

    struct rx_desc *desc = &rx_ring[ind];// 获取接收描述符
     // 如果需要接收的包都已经接收完毕，则退出
     if(!(desc->status & E1000_RXD_STAT_DD)){
        return;
     }
     rx_mbufs[ind]->len = desc->length;// 设置接收到的包长度
     net_rx(rx_mbufs[ind]);// 传递给上层网络栈。上层负责释放 mbuf

     // 分配并设置新的 mbuf，供给下一次轮到该下标时使用
     rx_mbufs[ind] = mbufalloc(0);
     desc->addr = (uint64)rx_mbufs[ind]->head;
     desc->status = 0;

     regs[E1000_RDT] = ind;// 更新接收描述符的指针
  }
}

void
e1000_intr(void)
{
  // tell the e1000 we've seen this interrupt;
  // without this the e1000 won't raise any
  // further interrupts.
  regs[E1000_ICR] = 0xffffffff;

  e1000_recv();
}
