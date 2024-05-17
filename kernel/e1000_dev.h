/* 寄存器定义 */
#define E1000_CTL      (0x00000/4)       /* 设备控制寄存器 - 可读写 */
#define E1000_ICR      (0x000C0/4)       /* 中断原因读取 - 只读 */
#define E1000_IMS      (0x000D0/4)       /* 中断掩码设置 - 可读写 */
#define E1000_RCTL     (0x00100/4)       /* 接收控制 - 可读写 */
#define E1000_TCTL     (0x00400/4)       /* 发送控制 - 可读写 */
#define E1000_TIPG     (0x00410/4)       /* 发送包间间隔 - 可读写 */
#define E1000_RDBAL    (0x02800/4)       /* 接收描述符基地址低 - 可读写 */
#define E1000_RDTR     (0x02820/4)       /* 接收延迟计时器 */
#define E1000_RADV     (0x0282C/4)       /* 接收中断绝对延迟计时器 */
#define E1000_RDH      (0x02810/4)       /* 接收描述符头 - 可读写 */
#define E1000_RDT      (0x02818/4)       /* 接收描述符尾 - 可读写 */
#define E1000_RDLEN    (0x02808/4)       /* 接收描述符长度 - 可读写 */
#define E1000_RSRPD    (0x02C00/4)       /* 小包检测中断 */
#define E1000_TDBAL    (0x03800/4)       /* 发送描述符基地址低 - 可读写 */
#define E1000_TDLEN    (0x03808/4)       /* 发送描述符长度 - 可读写 */
#define E1000_TDH      (0x03810/4)       /* 发送描述符头 - 可读写 */
#define E1000_TDT      (0x03818/4)       /* 发送描述符尾 - 可读写 */
#define E1000_MTA      (0x05200/4)       /* 多播表数组 - 可读写数组 */
#define E1000_RA       (0x05400/4)       /* 接收地址 - 可读写数组 */

/* 设备控制 */
#define E1000_CTL_SLU     0x00000040       /* 设置链路启用 */
#define E1000_CTL_FRCSPD  0x00000800       /* 强制速度 */
#define E1000_CTL_FRCDPLX 0x00001000       /* 强制全双工 */
#define E1000_CTL_RST     0x00400000       /* 完全复位 */

/* 发送控制 */
#define E1000_TCTL_RST    0x00000001       /* 软件复位 */
#define E1000_TCTL_EN     0x00000002       /* 启用发送 */
#define E1000_TCTL_BCE    0x00000004       /* 忙检查启用 */
#define E1000_TCTL_PSP    0x00000008       /* 补齐短包 */
#define E1000_TCTL_CT     0x00000ff0       /* 碰撞阈值 */
#define E1000_TCTL_CT_SHIFT 4
#define E1000_TCTL_COLD   0x003ff000       /* 碰撞距离 */
#define E1000_TCTL_COLD_SHIFT 12
#define E1000_TCTL_SWXOFF 0x00400000       /* 软件Xoff传输 */
#define E1000_TCTL_PBE    0x00800000       /* 数据包突发使能 */
#define E1000_TCTL_RTLC   0x01000000       /* 在晚期碰撞时重传 */
#define E1000_TCTL_NRTU   0x02000000       /* 在欠载时无重传 */
#define E1000_TCTL_MULR   0x10000000       /* 支持多请求 */

/* 接收控制 */
#define E1000_RCTL_RST            0x00000001   /* 软件复位 */
#define E1000_RCTL_EN             0x00000002   /* 启用 */
#define E1000_RCTL_SBP            0x00000004   /* 存储错误包 */
#define E1000_RCTL_UPE            0x00000008   /* 启用单播混杂模式 */
#define E1000_RCTL_MPE            0x00000010   /* 启用多播混杂模式 */
#define E1000_RCTL_LPE            0x00000020   /* 长包启用 */
#define E1000_RCTL_LBM_NO         0x00000000   /* 无环回模式 */
#define E1000_RCTL_LBM_MAC        0x00000040   /* MAC环回模式 */
#define E1000_RCTL_LBM_SLP        0x00000080   /* 串行链路环回模式 */
#define E1000_RCTL_LBM_TCVR       0x000000C0   /* TCVR环回模式 */
#define E1000_RCTL_DTYP_MASK      0x00000C00   /* 描述符类型掩码 */
#define E1000_RCTL_DTYP_PS        0x00000400   /* 分包描述符 */
#define E1000_RCTL_RDMTS_HALF     0x00000000   /* 接收描述符最小阈值大小 */
#define E1000_RCTL_RDMTS_QUAT     0x00000100   /* 接收描述符最小阈值四分之一 */
#define E1000_RCTL_RDMTS_EIGTH    0x00000200   /* 接收描述符最小阈值八分之一 */
#define E1000_RCTL_MO_SHIFT       12           /* 多播偏移量移位 */
#define E1000_RCTL_MO_0           0x00000000   /* 多播偏移量11:0 */
#define E1000_RCTL_MO_1           0x00001000   /* 多播偏移量12:1 */
#define E1000_RCTL_MO_2           0x00002000   /* 多播偏移量13:2 */
#define E1000_RCTL_MO_3           0x00003000   /* 多播偏移量15:4 */
#define E1000_RCTL_MDR            0x00004000   /* 多播描述符环0 */
#define E1000_RCTL_BAM            0x00008000   /* 广播启用 */
/* 这些缓冲区大小有效如果E1000_RCTL_BSEX为0 */
#define E1000_RCTL_SZ_2048        0x00000000   /* 接收缓冲区大小2048 */
#define E1000_RCTL_SZ_1024        0x00010000   /* 接收缓冲区大小1024 */
#define E1000_RCTL_SZ_512         0x00020000   /* 接收缓冲区大小512 */
#define E1000_RCTL_SZ_256         0x00030000   /* 接收缓冲区大小256 */
/* 这些缓冲区大小有效如果E1000_RCTL_BSEX为1 */
#define E1000_RCTL_SZ_16384       0x00010000   /* 接收缓冲区大小16384 */
#define E1000_RCTL_SZ_8192        0x00020000   /* 接收缓冲区大小8192 */
#define E1000_RCTL_SZ_4096        0x00030000   /* 接收缓冲区大小4096 */
#define E1000_RCTL_VFE            0x00040000   /* VLAN过滤启用 */
#define E1000_RCTL_CFIEN          0x00080000   /* 规范形式启用 */
#define E1000_RCTL_CFI            0x00100000   /* 规范形式指示 */
#define E1000_RCTL_DPF            0x00400000   /* 抛弃暂停帧 */
#define E1000_RCTL_PMCF           0x00800000   /* 通过MAC控制帧 */
#define E1000_RCTL_BSEX           0x02000000   /* 缓冲区大小扩展 */
#define E1000_RCTL_SECRC          0x04000000   /* 去除以太网CRC */
#define E1000_RCTL_FLXBUF_MASK    0x78000000   /* 灵活缓冲区大小掩码 */
#define E1000_RCTL_FLXBUF_SHIFT   27           /* 灵活缓冲区移位 */

#define DATA_MAX 1518

/* 发送描述符命令定义 [E1000 3.3.3.1] */
#define E1000_TXD_CMD_EOP    0x01 /* 包结束 */
#define E1000_TXD_CMD_RS     0x08 /* 报告状态 */

/* 发送描述符状态定义 [E1000 3.3.3.2] */
#define E1000_TXD_STAT_DD    0x00000001 /* 描述符完成 */

// [E1000 3.3.3]
struct tx_desc
{
  uint64 addr;       /* 数据缓冲区的地址 */
  uint16 length;    /* DMA到数据缓冲区的数据长度 */
  uint8 cso;       /* Checksum Offset */
  uint8 cmd;       /* 命令 */
  uint8 status;     /* 状态 */
  uint8 css;       /* Checksum Start */
  uint16 special;  /* 特殊字段 */
};

/* 接收描述符位定义 [E1000 3.2.3.1] */
#define E1000_RXD_STAT_DD       0x01    /* 描述符完成 */
#define E1000_RXD_STAT_EOP      0x02    /* 包结束 */

// [E1000 3.2.3]
struct rx_desc
{
  uint64 addr;       /* 描述符数据缓冲区的地址 */
  uint16 length;    /* DMA到数据缓冲区的长度 */
  uint16 csum;      /* 数据包校验和 */
  uint8 status;     /* 描述符状态 */
  uint8 errors;     /* 描述符错误 */
  uint16 special;
};
