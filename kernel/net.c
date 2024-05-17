// networking protocol support (IP, UDP, ARP, etc.).
//网络协议支持

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "net.h"
#include "defs.h"

static uint32 local_ip = MAKE_IP_ADDR(10, 0, 2, 15); // qemu's idea of the guest IP
static uint8 local_mac[ETHADDR_LEN] = { 0x52, 0x54, 0x00, 0x12, 0x34, 0x56 };
static uint8 broadcast_mac[ETHADDR_LEN] = { 0xFF, 0XFF, 0XFF, 0XFF, 0XFF, 0XFF };

// Strips data from the start of the buffer and returns a pointer to it.
// Returns 0 if less than the full requested length is available.
/*用于从mbuf结构体中的头部（head）取出长度为len的数据，并将mbuf的长度（len）减去取出数据的长度，
同时更新头部指针。如果可用长度小于请求的长度，则返回0，否则返回取出数据的指针。*/
char *
mbufpull(struct mbuf *m, unsigned int len)
{
  char *tmp = m->head;
  if (m->len < len)
    return 0;
  m->len -= len;
  m->head += len;
  return tmp;
}

// Prepends data to the beginning of the buffer and returns a pointer to it.
/*该函数用于将数据添加到缓冲区的开头，并返回指向新数据的指针。函数接受两个参数，一个是struct mbuf *m，
表示要操作的缓冲区，另一个是unsigned int len，表示要添加的数据长度。*/
char *
mbufpush(struct mbuf *m, unsigned int len)
{
  m->head -= len;
  if (m->head < m->buf)
    panic("mbufpush");
  m->len += len;
  return m->head;
}

// Appends data to the end of the buffer and returns a pointer to it.
/*该函数用于将数据追加到缓冲区的末尾，并返回指向追加数据位置的指针。 函数接受两个参数，
一个是struct mbuf *m，指向要追加数据的缓冲区，另一个是unsigned int len，表示要追加的数据长度。*/
char *
mbufput(struct mbuf *m, unsigned int len)
{
  char *tmp = m->head + m->len;
  m->len += len;
  if (m->len > MBUF_SIZE)
    panic("mbufput");
  return tmp;
}

// Strips data from the end of the buffer and returns a pointer to it.
// Returns 0 if less than the full requested length is available.
/*该函数用于缩减mbuf结构体中的可用长度，并返回调整后的新头部地址。输入参数m指向要缩减长度的mbuf结构体，
len为要缩减的长度。如果len大于m的当前长度，则函数返回0；否则，将m的长度减去len，并返回新头部地址。*/
char *
mbuftrim(struct mbuf *m, unsigned int len)
{
  if (len > m->len)
    return 0;
  m->len -= len;
  return m->head + m->len;
}

// Allocates a packet buffer.
/**
 * mbufalloc - 分配一个数据包缓冲区
 * @headroom: 需要的头部空间的大小
 * 
 * 这个函数分配一个具有指定头部空间的mbuf结构。如果请求的头部空间大于MBUF_SIZE，则函数失败并返回NULL。
 * 成功时返回指向分配的mbuf结构的指针，失败时返回0。
 */
struct mbuf *
mbufalloc(unsigned int headroom)
{
  struct mbuf *m;
 
  // 检查请求的headroom是否超过预定义的最大值
  if (headroom > MBUF_SIZE)
    return 0;
  
  // 尝试从内存池中分配一个mbuf结构体
  m = kalloc();
  if (m == 0)
    return 0; // 分配失败，返回NULL
  
  // 初始化mbuf结构
  m->next = 0; // 将下一个mbuf指针初始化为NULL
  m->head = (char *)m->buf + headroom; // 设置数据头部的起始位置
  m->len = 0; // 初始化数据长度为0
  memset(m->buf, 0, sizeof(m->buf)); // 清零缓冲区
  return m;
}

// Frees a packet buffer.
void
mbuffree(struct mbuf *m)
{
  kfree(m);
}

// Pushes an mbuf to the end of the queue.
/**
 * 将一个 mbuf 结构体推送到队列的尾部。
 * 
 * @param q 指向 mbufq 队列结构体的指针，代表目标队列。
 * @param m 指向待推送的 mbuf 结构体的指针。
 * 
 * 该函数首先将待推送的 mbuf 的 next 字段设置为 0，然后检查队列头是否为空。
 * 如果队列头为空，则同时将头和尾指针指向该 mbuf；如果不为空，则将当前尾部 mbuf 的
 * next 字段指向该 mbuf，并更新尾指针。该操作确保了 mbuf 的追加操作是在队列的尾部进行。
 */
void
mbufq_pushtail(struct mbufq *q, struct mbuf *m)
{
  // 初始化新加入的mbuf的next指针为NULL
  m->next = 0;
  
  // 如果队列为空，将头和尾指针都指向新的mbuf
  if (!q->head){
    q->head = q->tail = m;
    return;
  }
  
  // 如果队列不为空，将新mbuf追加到队列尾部
  q->tail->next = m;
  q->tail = m; // 更新尾指针
}

// Pops an mbuf from the start of the queue.
/**
 * 从队列的开始处弹出一个mbuf。
 * 
 * @param q 指向mbuf队列的指针。
 * @return 返回弹出的mbuf头；如果队列为空，则返回0。
 */
struct mbuf *
mbufq_pophead(struct mbufq *q)
{
  struct mbuf *head = q->head; // 获取队列头
  if (!head) // 如果队列头为空，说明队列为空，直接返回0
    return 0;
  q->head = head->next; // 将队列头指向下一个元素，实现弹出操作
  return head; // 返回被弹出的队列头
}

// Returns one (nonzero) if the queue is empty.
int
mbufq_empty(struct mbufq *q)
{
  return q->head == 0;
}

// Intializes a queue of mbufs.
void
mbufq_init(struct mbufq *q)
{
  q->head = 0;
}

// This code is lifted from FreeBSD's ping.c, and is copyright by the Regents
// of the University of California.
/*
 * 计算数据的校验和。
 * 
 * 这个函数对给定的数据块执行IPv4校验和计算。它被设计为高效处理网络数据，
 * 其中校验和的计算是网络协议中的常见需求。
 *
 * @param addr 指向需要计算校验和的数据块的指针。
 * @param len 数据块的长度，以字节为单位。
 * @return 计算得到的校验和，这是一个16位的无符号短整型。
 *
 * 注意：这个函数在计算校验和时采用了一些技巧和优化，
 * 包括对数据进行预处理以避免处理奇数字节，以及通过位运算将
 * 32位累加和折叠为16位。
 */
static unsigned short
in_cksum(const unsigned char *addr, int len)
{
  int nleft = len; // 剩余待处理的数据长度
  const unsigned short *w = (const unsigned short *)addr; // 强制转换为16位指针
  unsigned int sum = 0; // 用于计算校验和的32位累加器
  unsigned short answer = 0; // 存储最终的16位校验和

  // 主循环，针对数据块中的16位单词进行累加
  while (nleft > 1)  {
    sum += *w++;
    nleft -= 2;
  }

  // 处理剩余的奇数字节，如果有的话
  if (nleft == 1) {
    *(unsigned char *)(&answer) = *(const unsigned char *)w;
    sum += answer;
  }

  // 将累加器的高16位向低16位折叠，并处理可能的进位
  sum = (sum & 0xffff) + (sum >> 16);
  sum += (sum >> 16);
  // 此时，低16位包含正确的校验和

  answer = ~sum; // 取反以得到最终的校验和值
  return answer;
}

// sends an ethernet packet
// 函数：net_tx_eth
// 描述：发送一个以太网数据包。
// 参数：
// - m：指向包含待传输数据包的mbuf结构的指针。
// - ethtype：以太网类型字段，表示负载的协议。
// 注意：此函数无返回值。
static void
net_tx_eth(struct mbuf *m, uint16 ethtype)
{
  struct eth *ethhdr;

  // 在mbuf链首添加一个以太网头部。
  ethhdr = mbufpushhdr(m, *ethhdr);
  // 将源MAC地址设置为本地机器的MAC地址。
  memmove(ethhdr->shost, local_mac, ETHADDR_LEN);
  // 由于我们未完全支持ARP，将目的MAC地址设置为广播地址。
  memmove(ethhdr->dhost, broadcast_mac, ETHADDR_LEN);
  // 将以太网类型字段编码到头部。
  ethhdr->type = htons(ethtype);
  // 尝试使用e1000_transmit函数发送数据包。
  // 若发送失败，释放mbuf。
  if (e1000_transmit(m)) {
    mbuffree(m);
  }
}

// sends an IP packet
// 函数：net_tx_ip
// 描述：通过创建IP头部并将数据包传递给以太网层来发送IP数据包。
// 参数：
//    - m: 指向包含待传输数据的mbuf（消息缓冲区）的指针。
//    - proto: 表示正在传输的数据类型的协议号（例如，TCP、UDP）。
//    - dip: 目标IP地址，网络字节序表示。
// 注意：此函数无返回值。
static void
net_tx_ip(struct mbuf *m, uint8 proto, uint32 dip)
{
  struct ip *iphdr;

  // 在mbuf前部添加IP头部
  iphdr = mbufpushhdr(m, *iphdr);
  memset(iphdr, 0, sizeof(*iphdr)); // 清零IP头部
  // 初始化IP头部字段
  iphdr->ip_vhl = (4 << 4) | (20 >> 2); // IP版本和头部长度
  iphdr->ip_p = proto; // 协议类型
  iphdr->ip_src = htonl(local_ip); // 源IP地址（转换为网络字节序）
  iphdr->ip_dst = htonl(dip); // 目标IP地址（转换为网络字节序）
  iphdr->ip_len = htons(m->len); // IP包总长度
  iphdr->ip_ttl = 100; // 生存时间
  // 计算并设置校验和
  iphdr->ip_sum = in_cksum((unsigned char *)iphdr, sizeof(*iphdr));

  // 将数据包传递给以太网层进行进一步处理和传输
  net_tx_eth(m, ETHTYPE_IP);
}

// sends a UDP packet
/**
 * 发送一个UDP数据包。
 *
 * 此函数构造一个UDP头部，并将其封装在一个IP数据包中进行传输。
 * 它不会在UDP头部计算或包含校验和。
 *
 * @param m 要发送的数据的mbuf（数据包）。
 * @param dip 目标IP地址，作为无符号32位整数。
 * @param sport 源端口号，主机字节顺序。
 * @param dport 目标端口号，主机字节顺序。
 */
void
net_tx_udp(struct mbuf *m, uint32 dip,uint16 sport, uint16 dport)
{
  struct udp *udphdr;

  // 在mbuf的开头添加UDP头部。
  udphdr = mbufpushhdr(m, *udphdr);
  // 填充源和目标端口号。
  udphdr->sport = htons(sport);
  udphdr->dport = htons(dport);
  // 设置UDP数据的总长度。
  udphdr->ulen = htons(m->len);
  // 初始化校验和字段为0，表示未计算校验和。
  udphdr->sum = 0;

  // 将封装了UDP头部的mbuf传递给IP层进行传输。
  net_tx_ip(m, IPPROTO_UDP, dip);
}

// sends an ARP packet
// 发送一个ARP（地址解析协议）数据包
//
// 参数:
//   op: ARP操作类型（例如，ARP请求或应答）
//   dmac: 目标设备的MAC地址
//   dip: 目标IP地址
//
// 返回值:
//   成功发送返回0，失败返回-1。
static int
net_tx_arp(uint16 op, uint8 dmac[ETHADDR_LEN], uint32 dip)
{
  struct mbuf *m; // 用于存储数据包的缓冲区
  struct arp *arphdr; // ARP头部的指针

  // 分配用于ARP数据包的缓冲区
  m = mbufalloc(MBUF_DEFAULT_HEADROOM);
  if (!m)
    return -1; // 如果分配失败，则返回-1

  // 初始化ARP头部的通用部分
  arphdr = mbufputhdr(m, *arphdr);
  arphdr->hrd = htons(ARP_HRD_ETHER); // 硬件类型设置为以太网
  arphdr->pro = htons(ETHTYPE_IP); // 协议类型设置为IP
  arphdr->hln = ETHADDR_LEN; // 硬件地址长度设置为以太网地址长度
  arphdr->pln = sizeof(uint32); // 协议地址长度设置为IP地址长度
  arphdr->op = htons(op); // 操作类型转换为网络字节序

  // 设置ARP头部的以太网和IP部分
  memmove(arphdr->sha, local_mac, ETHADDR_LEN); // 源MAC地址
  arphdr->sip = htonl(local_ip); // 源IP地址
  memmove(arphdr->tha, dmac, ETHADDR_LEN); // 目标MAC地址
  arphdr->tip = htonl(dip); // 目标IP地址

  // 头部准备就绪，发送数据包
  net_tx_eth(m, ETHTYPE_ARP); // 通过以太网发送ARP数据包
  return 0; // 成功发送，返回0
}

// receives an ARP packet
// 接收ARP（地址解析协议）数据包
//
// 参数:
//   m: 指向包含ARP数据包的MBUF（内存缓冲区）的指针
//
static void
net_rx_arp(struct mbuf *m)
{
  struct arp *arphdr; // 指向ARP头部的指针
  uint8 smac[ETHADDR_LEN]; // 发送方以太网地址
  uint32 sip; // 发送方IP地址
  uint32 tip; // 目标IP地址

  // 从MBUF中提取ARP头部
  arphdr = mbufpullhdr(m, *arphdr);
  if (!arphdr)
    goto done; // 如果无法提取头部，则跳至结束处理

  // 验证ARP头部
  if (ntohs(arphdr->hrd) != ARP_HRD_ETHER ||
      ntohs(arphdr->pro) != ETHTYPE_IP ||
      arphdr->hln != ETHADDR_LEN ||
      arphdr->pln != sizeof(uint32)) {
    goto done; // 如果头部格式不正确，则跳至结束处理
  }

  // 目前仅支持ARP请求
  // 检查我们的IP是否被请求
  tip = ntohl(arphdr->tip); // 目标IP地址转换为网络字节序
  if (ntohs(arphdr->op) != ARP_OP_REQUEST || tip != local_ip)
    goto done; // 如果不是ARP请求或目标IP地址不匹配，则跳至结束处理

  // 处理ARP请求
  memmove(smac, arphdr->sha, ETHADDR_LEN); // 从ARP头部复制发送方的以太网地址
  sip = ntohl(arphdr->sip); // 发送方IP地址转换为网络字节序
  net_tx_arp(ARP_OP_REPLY, smac, sip); // 发送ARP回复

done:
  mbuffree(m); // 释放MBUF
}

// receives a UDP packet
// 接收一个UDP数据包
//
// 参数:
//   m: 指向包含UDP数据包的MBUF缓冲区的指针。
//   len: UDP数据包的长度。
//   iphdr: 指向IP头部的指针，用于获取IP层的信息。
//
// 说明:
//   该函数负责处理接收到的UDP数据包，包括校验和的验证（目前是TODO）、长度验证，
//   以及将数据包传递给相应的接收函数。
static void
net_rx_udp(struct mbuf *m, uint16 len, struct ip *iphdr)
{
  struct udp *udphdr;
  uint32 sip;
  uint16 sport, dport;

  // 从MBUF缓冲区中提取UDP头部
  udphdr = mbufpullhdr(m, *udphdr);
  if (!udphdr)
    goto fail; // 如果无法提取UDP头，则释放缓冲区并退出

  // TODO: 验证UDP校验和

  // 验证头部报告的长度是否一致
  if (ntohs(udphdr->ulen) != len)
    goto fail;
  len -= sizeof(*udphdr); // 从总长度中减去UDP头的长度
  if (len > m->len)
    goto fail; // 如果数据长度超过缓冲区长度，则视为无效
  // 调整缓冲区长度以匹配有效载荷长度
  mbuftrim(m, m->len - len);

  // 解析必要的字段，并将数据包传递给接收函数
  sip = ntohl(iphdr->ip_src); // 源IP地址转换为网络字节序
  sport = ntohs(udphdr->sport); // 源端口转换为网络字节序
  dport = ntohs(udphdr->dport); // 目标端口转换为网络字节序
  sockrecvudp(m, sip, dport, sport);
  return;

fail:
  mbuffree(m); // 发生错误时释放MBUF缓冲区
}

// 接收一个IP数据包
static void
net_rx_ip(struct mbuf *m)
{
  struct ip *iphdr;
  uint16 len;

  // 从数据包中提取IP头部
  iphdr = mbufpullhdr(m, *iphdr);
  if (!iphdr)
	  goto fail; // 如果无法提取IP头部，则失败处理

  // 检查IP版本和头部长度
  if (iphdr->ip_vhl != ((4 << 4) | (20 >> 2)))
    goto fail;

  // 验证IP校验和
  if (in_cksum((unsigned char *)iphdr, sizeof(*iphdr)))
    goto fail;

  // 不支持分片的IP数据包
  if (htons(iphdr->ip_off) != 0)
    goto fail;

  // 检查数据包是否发送给我们
  if (htonl(iphdr->ip_dst) != local_ip)
    goto fail;

  // 当前仅支持UDP协议
  if (iphdr->ip_p != IPPROTO_UDP)
    goto fail;

  // 计算有效载荷长度并传递给UDP处理函数
  len = ntohs(iphdr->ip_len) - sizeof(*iphdr);
  net_rx_udp(m, len, iphdr);
  return;

fail:
  // 错误处理：释放数据包
  mbuffree(m);
}

// called by e1000 driver's interrupt handler to deliver a packet to the
/**
 * 网络接收处理函数
 * 
 * 该函数用于处理接收到的网络数据包。它首先解析以太网头部，然后根据头部中的类型字段，
 * 将数据包交给相应的处理函数进行进一步处理。支持的类型包括IP和ARP数据包。
 * 不支持的数据包将被丢弃。
 * 
 * @param m 指向接收到的数据包的指针。该数据包包含以太网头部。
 */
void net_rx(struct mbuf *m)
{
  struct eth *ethhdr;
  uint16 type;

  // 从数据包中提取以太网头部
  ethhdr = mbufpullhdr(m, *ethhdr);
  if (!ethhdr) {
    // 如果无法提取头部，释放数据包并返回
    mbuffree(m);
    return;
  }

  // 将以太网头部中的类型字段转换为主机字节序
  type = ntohs(ethhdr->type);
  // 根据类型字段的不同，将数据包交给不同的处理函数
  if (type == ETHTYPE_IP)
    net_rx_ip(m); // 处理IP数据包
  else if (type == ETHTYPE_ARP)
    net_rx_arp(m); // 处理ARP数据包
  else
    // 不支持的数据包类型，释放数据包
    mbuffree(m);
}
