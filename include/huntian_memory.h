/* ============================================================================
 * 三进制内存寻址系统 - HunTian Trit Address Space
 * 
 * 高维流形视角：
 * - 地址不是线性索引，是拓扑位置编码
 * - Trit地址空间：3^n 种拓扑位置
 * - 使用分层进制寻址（3-12-36）
 * ============================================================================ */

#ifndef HUNTIAN_MEMORY_H
#define HUNTIAN_MEMORY_H

#include "ternary_types.h"
#include "huntian_layered_base.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ══════════════════════════════════════════════════════════════════════
 * 1. Trit 地址定义
 * ══════════════════════════════════════════════════════════════════════ */

/* Trit地址：使用平衡三进制编码位置
 * 
 * 高维视角：
 * - 地址 = 拓扑位置的编码
 * - 不是线性索引，是流形上的坐标
 */

/* Trit地址结构 */
typedef struct {
    Trit    trits[16];        /* 16 Trit地址 = 3^16 种位置 */
    int64_t linear_offset;    /* 线性偏移（兼容传统内存） */
} TritAddress;

#define TRIT_ADDR_BITS 16     /* 16 Trit地址位 */
#define TRIT_ADDR_MAX   21523360  /* (3^16-1)/2 */

/* Trit地址初始化 */
static inline void trit_addr_init(TritAddress* addr, int64_t offset) {
    addr->linear_offset = offset;
    
    /* 将偏移转换为Trit地址 */
    int64_t remaining = offset;
    for (int i = 0; i < TRIT_ADDR_BITS; i++) {
        int remainder = remaining % 3;
        remaining /= 3;
        
        if (remainder == 2) {
            addr->trits[i] = TRIT_NEG;
            remaining += 1;
        } else if (remainder == -2) {
            addr->trits[i] = TRIT_POS;
            remaining -= 1;
        } else {
            addr->trits[i] = (Trit)remainder;
        }
    }
}

/* Trit地址转线性偏移 */
static inline int64_t trit_addr_to_offset(TritAddress* addr) {
    int64_t offset = 0;
    int64_t power = 1;
    
    for (int i = 0; i < TRIT_ADDR_BITS; i++) {
        offset += (int64_t)addr->trits[i] * power;
        power *= 3;
    }
    
    return offset;
}

/* Trit地址加法（地址偏移） */
static inline void trit_addr_add(TritAddress* addr, int64_t delta) {
    Trit carry = TRIT_ZERO;
    Trit delta_trits[TRIT_ADDR_BITS];
    
    /* 将delta转换为Trit */
    int64_t remaining = delta;
    for (int i = 0; i < TRIT_ADDR_BITS; i++) {
        int rem = remaining % 3;
        remaining /= 3;
        if (rem == 2) {
            delta_trits[i] = TRIT_NEG;
            remaining++;
        } else if (rem == -2) {
            delta_trits[i] = TRIT_POS;
            remaining--;
        } else {
            delta_trits[i] = (Trit)rem;
        }
    }
    
    /* Trit加法 */
    for (int i = 0; i < TRIT_ADDR_BITS; i++) {
        int sum = (int)addr->trits[i] + (int)delta_trits[i] + (int)carry;
        
        if (sum >= 2) {
            addr->trits[i] = TRIT_NEG;
            carry = TRIT_POS;
        } else if (sum <= -2) {
            addr->trits[i] = TRIT_POS;
            carry = TRIT_NEG;
        } else {
            addr->trits[i] = (Trit)sum;
            carry = TRIT_ZERO;
        }
    }
    
    addr->linear_offset = trit_addr_to_offset(addr);
}

/* ══════════════════════════════════════════════════════════════════════
 * 2. Trit 内存空间
 * ══════════════════════════════════════════════════════════════════════ */

/* Trit内存块：以Tryte为基本单元 */
typedef struct {
    Tryte*  data;            /* Tryte数据数组 */
    size_t  tryte_count;     /* Tryte数量 */
    TritAddress base_addr;   /* 基地址 */
} TritMemoryBlock;

/* Trit内存空间：完整的寻址空间 */
typedef struct {
    TritMemoryBlock* blocks;     /* 内存块数组 */
    size_t           block_count;/* 内存块数量 */
    size_t           total_trytes;/* 总Tryte数 */
    TritAddress      free_addr;  /* 下一个空闲地址 */
} TritMemorySpace;

/* 初始化Trit内存空间 */
static inline TritMemorySpace* trit_mem_space_create(size_t initial_trytes) {
    TritMemorySpace* space = (TritMemorySpace*)malloc(sizeof(TritMemorySpace));
    if (!space) return NULL;
    
    space->blocks = (TritMemoryBlock*)malloc(sizeof(TritMemoryBlock));
    if (!space->blocks) {
        free(space);
        return NULL;
    }
    
    space->blocks[0].data = (Tryte*)calloc(initial_trytes, sizeof(Tryte));
    if (!space->blocks[0].data) {
        free(space->blocks);
        free(space);
        return NULL;
    }
    
    space->blocks[0].tryte_count = initial_trytes;
    trit_addr_init(&space->blocks[0].base_addr, 0);
    
    space->block_count = 1;
    space->total_trytes = initial_trytes;
    trit_addr_init(&space->free_addr, 0);
    
    return space;
}

/* 释放Trit内存空间 */
static inline void trit_mem_space_destroy(TritMemorySpace* space) {
    if (!space) return;
    
    for (size_t i = 0; i < space->block_count; i++) {
        free(space->blocks[i].data);
    }
    free(space->blocks);
    free(space);
}

/* ══════════════════════════════════════════════════════════════════════
 * 3. Trit 内存访问
 * ══════════════════════════════════════════════════════════════════════ */

/* 通过Trit地址读取Tryte */
static inline Tryte trit_mem_read_tryte(TritMemorySpace* space, TritAddress* addr) {
    int64_t offset = trit_addr_to_offset(addr);
    
    /* 查找对应内存块 */
    for (size_t i = 0; i < space->block_count; i++) {
        int64_t base = trit_addr_to_offset(&space->blocks[i].base_addr);
        if (offset >= base && offset < base + space->blocks[i].tryte_count) {
            return space->blocks[i].data[offset - base];
        }
    }
    
    /* 地址无效，返回零 */
    Tryte zero;
    for (int i = 0; i < TRYTE_TRITS; i++) zero.trits[i] = TRIT_ZERO;
    return zero;
}

/* 通过Trit地址写入Tryte */
static inline int trit_mem_write_tryte(TritMemorySpace* space, 
                                        TritAddress* addr, Tryte value) {
    int64_t offset = trit_addr_to_offset(addr);
    
    /* 查找对应内存块 */
    for (size_t i = 0; i < space->block_count; i++) {
        int64_t base = trit_addr_to_offset(&space->blocks[i].base_addr);
        if (offset >= base && offset < base + space->blocks[i].tryte_count) {
            space->blocks[i].data[offset - base] = value;
            return 0;  /* 成功 */
        }
    }
    
    return -1;  /* 地址无效 */
}

/* 通过线性偏移读取Tryte */
static inline Tryte trit_mem_read_at(TritMemorySpace* space, int64_t offset) {
    TritAddress addr;
    trit_addr_init(&addr, offset);
    return trit_mem_read_tryte(space, &addr);
}

/* 通过线性偏移写入Tryte */
static inline int trit_mem_write_at(TritMemorySpace* space, 
                                     int64_t offset, Tryte value) {
    TritAddress addr;
    trit_addr_init(&addr, offset);
    return trit_mem_write_tryte(space, &addr, value);
}

/* ══════════════════════════════════════════════════════════════════════
 * 4. Trit 内存分配
 * ══════════════════════════════════════════════════════════════════════ */

/* Trit内存分配：返回分配的地址 */
static inline TritAddress trit_mem_alloc(TritMemorySpace* space, size_t trytes) {
    TritAddress alloc_addr = space->free_addr;
    
    /* 检查是否需要扩展 */
    int64_t needed_offset = trit_addr_to_offset(&space->free_addr) + trytes;
    int64_t current_max = 0;
    for (size_t i = 0; i < space->block_count; i++) {
        int64_t block_end = trit_addr_to_offset(&space->blocks[i].base_addr) 
                          + space->blocks[i].tryte_count;
        if (block_end > current_max) current_max = block_end;
    }
    
    if (needed_offset > current_max) {
        /* 扩展内存块 */
        size_t new_block_count = space->block_count + 1;
        TritMemoryBlock* new_blocks = (TritMemoryBlock*)realloc(
            space->blocks, new_block_count * sizeof(TritMemoryBlock));
        if (!new_blocks) {
            TritAddress invalid;
            trit_addr_init(&invalid, -1);
            return invalid;
        }
        
        space->blocks = new_blocks;
        space->block_count = new_block_count;
        
        size_t expansion = needed_offset - current_max;
        if (expansion < trytes) expansion = trytes;
        
        space->blocks[new_block_count - 1].data = (Tryte*)calloc(expansion, sizeof(Tryte));
        space->blocks[new_block_count - 1].tryte_count = expansion;
        trit_addr_init(&space->blocks[new_block_count - 1].base_addr, current_max);
        
        space->total_trytes += expansion;
    }
    
    /* 更新空闲地址 */
    trit_addr_add(&space->free_addr, trytes);
    
    return alloc_addr;
}

/* Trit内存释放 */
static inline void trit_mem_free(TritMemorySpace* space, 
                                  TritAddress* addr, size_t trytes) {
    /* 简化版本：标记为可用（不实际释放） */
    /* 高维视角：拓扑空间回收 */
    (void)space;
    (void)addr;
    (void)trytes;
}

/* ══════════════════════════════════════════════════════════════════════
 * 5. 分层进制寻址
 * ══════════════════════════════════════════════════════════════════════ */

/* 分层地址：使用3-12-36结构 */
typedef struct {
    TritAddress    trit_addr;   /* Trit地址 */
    Base12Number    spiral_addr; /* 12进制螺旋地址 */
    Base36Number    quantum_addr;/* 36进制量子态地址 */
} LayeredAddress;

/* Trit地址转分层地址 */
static inline void trit_addr_to_layered(TritAddress* addr, LayeredAddress* layered) {
    layered->trit_addr = *addr;
    
    /* 分解为12进制螺旋地址 */
    Trit spiral_trits[4];
    for (int i = 0; i < 4; i++) spiral_trits[i] = addr->trits[i];
    trits_to_base12(spiral_trits, 4, &layered->spiral_addr);
    
    /* 分解为36进制量子态地址 */
    Trit quantum_trits[8];
    for (int i = 0; i < 8; i++) quantum_trits[i] = addr->trits[i];
    trits_to_base36(quantum_trits, 8, &layered->quantum_addr);
}

/* 分层地址转Trit地址 */
static inline void layered_addr_to_trit(LayeredAddress* layered, TritAddress* addr) {
    /* 从36进制恢复Trit */
    base36_to_trits(&layered->quantum_addr, addr->trits);
    
    /* 补充高位Trit */
    for (int i = 8; i < TRIT_ADDR_BITS; i++) {
        addr->trits[i] = TRIT_ZERO;
    }
    
    addr->linear_offset = trit_addr_to_offset(addr);
}

/* ══════════════════════════════════════════════════════════════════════
 * 6. 拓扑寻址（测地线距离）
 * ══════════════════════════════════════════════════════════════════════ */

/* 计算两个Trit地址之间的拓扑距离 */
static inline double trit_addr_topological_distance(TritAddress* a, TritAddress* b) {
    /* 高维视角：不是线性距离，是拓扑距离 */
    double dist = 0;
    
    for (int i = 0; i < TRIT_ADDR_BITS; i++) {
        int diff = (int)a->trits[i] - (int)b->trits[i];
        /* 环面距离：考虑周期性 */
        if (diff > 1) diff = 2 - diff;
        if (diff < -1) diff = -2 - diff;
        dist += diff * diff;
    }
    
    return sqrt(dist);
}

/* Trit地址的黄金螺旋映射 */
static inline TritAddress trit_addr_spiral_transform(TritAddress* addr, int phase) {
    TritAddress result;
    
    /* 应用黄金角相位偏移 */
    for (int i = 0; i < TRIT_ADDR_BITS; i++) {
        int spiral_phase = (int)(i * PHI_GOLDEN * phase) % TRIT_ADDR_BITS;
        result.trits[spiral_phase] = addr->trits[i];
    }
    
    result.linear_offset = trit_addr_to_offset(&result);
    return result;
}

/* ══════════════════════════════════════════════════════════════════════
 * 7. Trit 指针类型
 * ══════════════════════════════════════════════════════════════════════ */

/* Trit指针：指向Trit内存空间的地址 */
typedef struct {
    TritMemorySpace* space;   /* 所属内存空间 */
    TritAddress      addr;    /* Trit地址 */
    Trit*            direct;  /* 直接指针（可选） */
} TritPointer;

/* Trit指针初始化 */
static inline void trit_ptr_init(TritPointer* ptr, TritMemorySpace* space, 
                                  int64_t offset) {
    ptr->space = space;
    trit_addr_init(&ptr->addr, offset);
    ptr->direct = NULL;
}

/* Trit指针读取 */
static inline Tryte trit_ptr_read(TritPointer* ptr) {
    if (ptr->direct) {
        return *ptr->direct;
    }
    return trit_mem_read_tryte(ptr->space, &ptr->addr);
}

/* Trit指针写入 */
static inline int trit_ptr_write(TritPointer* ptr, Tryte value) {
    if (ptr->direct) {
        *ptr->direct = value;
        return 0;
    }
    return trit_mem_write_tryte(ptr->space, &ptr->addr, value);
}

/* Trit指针偏移 */
static inline void trit_ptr_offset(TritPointer* ptr, int64_t delta) {
    trit_addr_add(&ptr->addr, delta);
    if (ptr->direct) {
        ptr->direct += delta;
    }
}

/* ══════════════════════════════════════════════════════════════════════
 * 8. Trit 数组操作
 * ══════════════════════════════════════════════════════════════════════ */

/* Trit数组复制 */
static inline void trit_memcpy(TritPointer* dst, TritPointer* src, size_t count) {
    for (size_t i = 0; i < count; i++) {
        Tryte val = trit_ptr_read(src);
        trit_ptr_write(dst, val);
        trit_ptr_offset(dst, 1);
        trit_ptr_offset(src, 1);
    }
}

/* Trit数组设置 */
static inline void trit_memset(TritPointer* ptr, Trit value, size_t count) {
    Tryte fill;
    for (int i = 0; i < TRYTE_TRITS; i++) fill.trits[i] = value;
    
    for (size_t i = 0; i < count; i++) {
        trit_ptr_write(ptr, fill);
        trit_ptr_offset(ptr, 1);
    }
}

/* Trit数组比较 */
static inline int trit_memcmp(TritPointer* a, TritPointer* b, size_t count) {
    for (size_t i = 0; i < count; i++) {
        Tryte va = trit_ptr_read(a);
        Tryte vb = trit_ptr_read(b);
        
        int32_t ia = tryte_to_int(va);
        int32_t ib = tryte_to_int(vb);
        
        if (ia < ib) return -1;
        if (ia > ib) return 1;
        
        trit_ptr_offset(a, 1);
        trit_ptr_offset(b, 1);
    }
    
    return 0;
}

/* ══════════════════════════════════════════════════════════════════════
 * 9. Trit 堆栈实现
 * ══════════════════════════════════════════════════════════════════════ */

/* Trit堆栈 */
typedef struct {
    TritMemorySpace* space;
    TritAddress      base;
    TritAddress      top;
    size_t           capacity;
} TritStack;

/* 创建Trit堆栈 */
static inline TritStack* trit_stack_create(TritMemorySpace* space, size_t capacity) {
    TritStack* stack = (TritStack*)malloc(sizeof(TritStack));
    if (!stack) return NULL;
    
    stack->space = space;
    stack->base = trit_mem_alloc(space, capacity);
    stack->top = stack->base;
    stack->capacity = capacity;
    
    return stack;
}

/* Trit压栈 */
static inline int trit_stack_push(TritStack* stack, Tryte value) {
    int64_t top_offset = trit_addr_to_offset(&stack->top);
    int64_t base_offset = trit_addr_to_offset(&stack->base);
    
    if (top_offset - base_offset >= stack->capacity) {
        return -1;  /* 堆栈满 */
    }
    
    trit_mem_write_tryte(stack->space, &stack->top, value);
    trit_addr_add(&stack->top, 1);
    
    return 0;
}

/* Trit弹栈 */
static inline Tryte trit_stack_pop(TritStack* stack) {
    int64_t top_offset = trit_addr_to_offset(&stack->top);
    int64_t base_offset = trit_addr_to_offset(&stack->base);
    
    if (top_offset <= base_offset) {
        Tryte empty;
        for (int i = 0; i < TRYTE_TRITS; i++) empty.trits[i] = TRIT_ZERO;
        return empty;  /* 堆栈空 */
    }
    
    trit_addr_add(&stack->top, -1);
    return trit_mem_read_tryte(stack->space, &stack->top);
}

/* 释放Trit堆栈 */
static inline void trit_stack_destroy(TritStack* stack) {
    free(stack);
}

#endif /* HUNTIAN_MEMORY_H */