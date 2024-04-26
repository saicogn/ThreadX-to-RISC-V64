# 简介
移植Eclipse ThreadX RTOS到RISC-V64架构，验证平台为Milk-V Duo（cv180x，双核C906，指令集为RV64GCV（RV64IMAFDCV））。ThreadX运行于无MMU的小核C906L上，Linux运行于大核C906B上。

本工程基于Milk-V Duo官方SDK（tag: Duo-V1.0.9）修改而来，暂时只支持cv180x系列CPU。

# 编译构建
## 全SDK编译
将本工程拉取到Milk-V Duo官方SDK下，修改duo-buildroot-sdk/build/milkvsetup.sh脚本的FREERTOS_PATH环境变量为本仓库的相对路径：
```sh
#FREERTOS_PATH="$TOP_DIR"/freertos # 修改此项
FREERTOS_PATH="$TOP_DIR"/threadx
```
运行Milk-V Duo官方的一件编译脚本，-j[x]为启用x核编译（详细配置见Milk-V Duo官方仓库README或官方Wiki）
```
# ./build.sh milkv-duo -j6
```
编译结束后生成的fip.bin文件即包含了ThreadX。

## 单独编译fip.bin
若不想每次都编译全部SDK文件，拷贝本工程的rtos_build.sh脚本至SDK根目录（和build.sh同级目录）后执行该脚本，默认将只编译生成fip.bin。
```
# ./rtos_build.sh milkv-duo -j6
```
也可以注释掉clean_uboot，开启make rtos_clean相关内容，可避免重复编译fip.bin中除RTOS以外的内容
```sh
#clean_uboot || return $?

_build_uboot_env
cd "$BUILD_PATH" || return
make rtos-clean

build_uboot || return $?
```

（若只想编译RTOS，不修改SDK，可以直接运行threadx/build_cv180x.sh脚本，生成cvirtos.bin。但需要提前手动导出编译所需的环境变量，并在编译完成后手动运行fiptool.py脚本合成fip.bin文件）

# 目录结构
```
├── build_freertos.sh  // cv181x构建脚本, 暂时不用
├── rtos_build.sh      // 修改的构建脚本, 需要复制到SDK根目录后运行
├── cvitek             // 官方BSP
├── threadx            // ThreadX源文件和移植文件
│   ├── common         // ThreadX核心源文件
│   ├── ports          // 芯片架构和编译器特定文件
│   │   ├── risc-v64   // RISC-V64架构移植相关文件
│   │   └── ...
│   └── ...
└── README-zh.md       // REAMME
```

# 主要移植内容
1. 修改tx_initialize_low_level.S文件，增加首个空闲内存地址、配置trap handler入口、跳转至定时器中断配置函数port_specific_pre_initialization。
2. 实现trap handler。按照ThreadX官方惯例也放在tx_initialize_low_level.S文件中（参考Milk-V Duo中FreeRTOS的实现）。
3. 新增定时器中断配置相关的文件port_specific_pre_initialization.c。
4. 修改tx_thread_context_save.S、tx_thread_context_restore.S、tx_thread_schedule.S文件中对于退出trap后mstatus寄存器的处理，避免因为浮点问题导致死机。
5. 修改tx_port.h头文件，新增tx_user.h文件（位于threadx/cvitek/kernel/include/riscv64/tx_user.h）

# 示例
提供2个简单示例，位于threadx/cvitek/task/comm/src/riscv64/comm_main.c文件中，通过宏```USE_MAILBOX_EXAMPLE```控制条件编译。
1. ThreadX默认的demo（增加了打印log，原始代码见threadx/threadx/samples目录下的demo_threadx.c）。
2. Milk-V Duo官方的大小核邮箱通信demo，通过大核的Linux通知小核的RTOS控制LED灯。

# 参考
ThreadX repo：https://github.com/eclipse-threadx/threadx/

Milk-V Duo repo： https://github.com/milkv-duo/duo-buildroot-sdk/

Milk-V Duo 在线Wiki：https://milkv.io/zh/docs/duo/overview