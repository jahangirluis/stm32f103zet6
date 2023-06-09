#  ReleaseNotes

## [OneOS-V2.0.0] 2021.07.23

### 内核

1. 采用全新内核架构，并对各种内核机制进行优化，最大中断响应性能得到大幅度提升；
2. 对部分接口形式进行优化，但大部分接口未发生变化。

### 组件

1.  Molink增加ctwing接入功能、多模组自动匹配功能；
2.  Fota功能进行了较大优化，app侧减少30%以上flash和ram占用；
3.  完善了高级语言组件，优化micropython的使用体验以及ams功能；
4.  优化文件系统，支持rm指令递归删除文件；
5.  优化端云组件，增加百度物联网云的接入功能；
6.  更新了ip协议栈，解决了上一版本的bug；
7.  完善了蓝牙协议栈，支持蓝牙mesh；
8.  增加对嵌入式调试工具openocd的支持；
9.  新增ecoredump调试工具。

### BSP

1. 修改工程管理方式，新增 templates 工程模板目录；
2. 重构 device，提供阻塞读写、非阻塞读写功能；
3. 新增超过800款MCU型号适配，屏蔽底层硬件差异。

### 安全

 1.新增OneTLS安全通信协议栈，支持TLS1.3规范（RFC8446）和DTLS1.3规范（draft38）。

### 定位

 1.新增蓝牙定位模式，提供基于rssi的蓝牙定位功能。

## [OneOS-V2.0.1] 2021.08.19

### 内核

1. 修复滴答处理函数bug。



