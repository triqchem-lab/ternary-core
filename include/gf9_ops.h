/* ══════════════════════════════════════════════════════════════════════
 * gf9_ops.h — GF(9) 域扩展指令 (标准三进制 {0,1,2}, α² = −1 ≡ 2)
 * HunTian 三进制指令核心 · 主权标准三进制 {0,1,2}
 *
 * GF(9) = GF(3)²: 元素 (a,b) = a + bα, a,b ∈ {0,1,2} (标准三进制)。
 * 与 Agda Sovereign/Algebra/GF9.agda 逐字段对齐 (无同构映射):
 *   (a,b) + (c,d) = (a⊕c, b⊕d)                  [⊕ = mod 3 加]
 *   (a,b) · (c,d) = (a⊗c ⊕ ⊖(b⊗d), a⊗d ⊕ b⊗c)   [⊖ = 取负, α² = −1 = 2]
 *   α = (0,1),  α·α = (2,0)  (Agda: alpha-squared : α*α ≡ (T₂,T₀))
 *
 * BitNet 权重映射 (pyBitNet _gf3_weight 前向, 精确整数减法零浮点):
 *   标准 {0,1,2} → 计算域平衡 {−1,0,+1}:  bal = std − 1
 *   平衡 {−1,0,+1} → 标准 {0,1,2}:        std = bal + 1
 *   即: 0→−1 (阴), 1→0 (中性/跳过), 2→+1 (阳)
 *
 * 平衡三进制 (ternary-core 的 Trit {−1,0,+1}) 用于数域转换层 (LCM 桥),
 * 本头文件的域运算一律采用标准 {0,1,2}, 与 Agda/数学库一致。
 * 交叉验证: /tmp/xval (Python 精确算术, 2026-08-16)。
 * ══════════════════════════════════════════════════════════════════════ */

#ifndef GF9_OPS_H
#define GF9_OPS_H

#include <stdint.h>

/* ── 标准三进制 {0,1,2} 基本运算 (mod 3) ── */
static inline uint8_t sov3_add(uint8_t a, uint8_t b) { return (uint8_t)((a + b) % 3); }
static inline uint8_t sov3_mul(uint8_t a, uint8_t b) { return (uint8_t)((a * b) % 3); }
static inline uint8_t sov3_neg(uint8_t a) { return (uint8_t)((3 - a) % 3); } /* 0→0, 1→2, 2→1 */

/* GF(9) 元素: 2 trit 打包 (a + bα), a,b ∈ {0,1,2} */
typedef struct {
    uint8_t a;  /* 实部 */
    uint8_t b;  /* α 系数 */
} Gf9El;

/* ── 常量构造 ── */
static inline Gf9El vavx3_gf9_zero(void)  { Gf9El z = {0, 0}; return z; }
static inline Gf9El vavx3_gf9_one(void)   { Gf9El o = {1, 0}; return o; }
static inline Gf9El vavx3_gf9_alpha(void) { Gf9El a = {0, 1}; return a; }

/* ── 加法: (a,b)+(c,d) = (a⊕c, b⊕d) ── */
static inline Gf9El vavx3_gf9_add(Gf9El x, Gf9El y) {
    Gf9El r;
    r.a = sov3_add(x.a, y.a);
    r.b = sov3_add(x.b, y.b);
    return r;
}

/* ── 取负 ── */
static inline Gf9El vavx3_gf9_neg(Gf9El x) {
    Gf9El r = {sov3_neg(x.a), sov3_neg(x.b)};
    return r;
}

/* ── 减法: x − y = x + (−y) ── */
static inline Gf9El vavx3_gf9_sub(Gf9El x, Gf9El y) {
    return vavx3_gf9_add(x, vavx3_gf9_neg(y));
}

/* ── 共轭 (Frobenius): σ(a+bα) = a + (⊖b)α ── */
static inline Gf9El vavx3_gf9_conj(Gf9El x) {
    Gf9El r = {x.a, sov3_neg(x.b)};
    return r;
}

/* ── 乘法: (a+bα)(c+dα) = (a⊗c ⊕ ⊖(b⊗d), a⊗d ⊕ b⊗c) ── */
static inline Gf9El vavx3_gf9_mul(Gf9El x, Gf9El y) {
    Gf9El r;
    r.a = sov3_add(sov3_mul(x.a, y.a), sov3_neg(sov3_mul(x.b, y.b)));
    r.b = sov3_add(sov3_mul(x.a, y.b), sov3_mul(x.b, y.a));
    return r;
}

/* ── 范数 N(a+bα) = a⊗a ⊕ b⊗b (Agda: galoisNorm) ── */
static inline uint8_t vavx3_gf9_norm(Gf9El x) {
    return sov3_add(sov3_mul(x.a, x.a), sov3_mul(x.b, x.b));
}

/* ── 迹 Tr(a+bα) = a⊕a = 2a (Agda: galoisTrace = 2a) ── */
static inline uint8_t vavx3_gf9_trace(Gf9El x) {
    return sov3_add(x.a, x.a);
}

/* ══════════════════════════════════════════════════════════════════════
 * BitNet 权重域映射 (标准 ↔ 计算域平衡) — 精确整数 ±1, 零浮点
 * ══════════════════════════════════════════════════════════════════════ */

/* 标准 {0,1,2} → 平衡 {−1,0,+1}: bal = std − 1 (pyBitNet: w_gf3 − 1) */
static inline int8_t sov3_to_balanced(uint8_t std) { return (int8_t)((int)std - 1); }

/* 平衡 {−1,0,+1} → 标准 {0,1,2}: std = bal + 1 */
static inline uint8_t balanced_to_sov3(int8_t bal) { return (uint8_t)((int)bal + 1); }

/* GF(9) 元素 标准 → 平衡 (两个分量) */
static inline void gf9_to_balanced(Gf9El x, int8_t out[2]) {
    out[0] = sov3_to_balanced(x.a);
    out[1] = sov3_to_balanced(x.b);
}

/* GF(9) 元素 平衡 → 标准 */
static inline Gf9El gf9_from_balanced(const int8_t bal[2]) {
    Gf9El r = {balanced_to_sov3(bal[0]), balanced_to_sov3(bal[1])};
    return r;
}

#endif /* GF9_OPS_H */
