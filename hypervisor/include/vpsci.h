#ifndef __VPSCI_H__
#define __VPSCI_H__

#include <types.h>
#include <vcpu.h>

/* https://developer.aliyun.com/article/1205031 */
#define PSCI_VERSION            0x84000000 //返回 PSCI 的主版本号和次版本号（32 位值）：
#define PSCI_MIGRATE_INFO_TYPE  0x84000006 //返回系统是否支持 CPU 迁移（migration），以及迁移的类型
#define PSCI_SYSTEM_OFF         0x84000008 //通知系统进入完全关闭状态（电源关闭）
#define PSCI_SYSTEM_RESET       0x84000009 //通知系统执行重启（软复位或硬复位）
#define PSCI_SYSTEM_CPUON       0xc4000003 //唤醒一个关闭或低功耗的 CPU，设置其执行入口地址和上下文
#define PSCI_FEATURE		    0x8400000a //检查特定 PSCI 功能是否可用。输入功能 ID，返回支持状态。

u64 vpsci_trap_smc(vcpu_t *vcpu, u64 funid, u64 target_cpu, u64 entry_addr);
u64 smc_call(u64 funid, u64 target_cpu, u64 entry_addr);


#endif