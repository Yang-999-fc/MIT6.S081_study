//
// simple PCI-Express initialization, only
// works for qemu and its e1000 card.
//

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

/**
 * 初始化PCI设备，特别是检测和配置e1000网卡。
 * 该函数不接受参数，也不返回任何值。
 * 
 * 首先，函数定义了e1000网卡寄存器的地址和PCIe配置空间的地址。
 * 然后，它遍历总线0上的每个可能的PCI设备，检测并配置e1000网卡。
 * 如果发现e1000设备，它会启用I/O和内存访问，设置BAR，并初始化e1000网卡驱动。
 */
void
pci_init()
{
  // 定义e1000寄存器的虚拟地址
  uint64 e1000_regs = 0x40000000L;

  // 定义PCIe配置空间的虚拟地址
  uint32  *ecam = (uint32 *) 0x30000000L;
  
  // 遍历总线0上的所有设备，查找e1000网卡
  for(int dev = 0; dev < 32; dev++){
    int bus = 0;
    int func = 0;
    int offset = 0;
    uint32 off = (bus << 16) | (dev << 11) | (func << 8) | (offset);
    volatile uint32 *base = ecam + off;
    uint32 id = base[0];
    
    // 检查设备ID是否为e1000网卡
    if(id == 0x100e8086){
      // 启用I/O和内存访问，设置总线主控
      base[1] = 7;
      __sync_synchronize(); // 确保写入操作完成

      // 重置BAR以获取其大小
      for(int i = 0; i < 6; i++){
        uint32 old = base[4+i];
        base[4+i] = 0xffffffff;
        __sync_synchronize();
        base[4+i] = old;
      }

      // 设置e1000寄存器的物理地址
      base[4+0] = e1000_regs;

      // 初始化e1000网卡驱动
      e1000_init((uint32*)e1000_regs);
    }
  }
}
