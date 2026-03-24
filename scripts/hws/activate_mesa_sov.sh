#!/bin/bash
# ==============================================================================
# 🌌 [ACTIVATE-MESA-SOV] 浑天主权驱动激活脚本
# 目标：劫持系统图形/计算链路，指向 /opt/mesa
# 平台：RX 580 (gfx803)
# ==============================================================================

export OPT_MESA="/opt/mesa"

# 1. 劫持动态链接器
export LD_LIBRARY_PATH="${OPT_MESA}/lib:${LD_LIBRARY_PATH}"
export LIBGL_DRIVERS_PATH="${OPT_MESA}/lib/dri"

# 2. 劫持 OpenCL ICD (屏蔽系统自带，仅使用主权版)
export OCL_ICD_VENDORS="${OPT_MESA}/etc/OpenCL/vendors"

# 3. 劫持 Vulkan ICD
export VK_ICD_FILENAMES="${OPT_MESA}/share/vulkan/icd.d/radeon_icd.x86_64.json"

# 4. 开启物理特性
export RUSTICL_ENABLE=radeonsi
export ROC_ENABLE_PRE_VEGA=1
export HSA_OVERRIDE_GFX_VERSION=8.0.3

echo "=============================================================================="
echo "   HUNTIAN SOVEREIGN MESA DRIVER ACTIVATED [v26.1.0-devel]                  "
echo "   Physical Path: /opt/mesa | Status: Superfluid Mode (2.1K)                "
echo "=============================================================================="
