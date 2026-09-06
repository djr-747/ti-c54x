#ifndef TI_C54X_STATE_REGISTER_LAYOUT_H
#define TI_C54X_STATE_REGISTER_LAYOUT_H

/* Module contract
 * Owns: the private fixed-width bit masks and knownness-bit positions for the
 * common C54x register state.
 * Does not own: state storage, reset mutation, CPU lifecycle, device pins, XPC,
 * or public ABI declarations.
 */

#include <stdint.h>

/* ST0: ARP[15:13], TC[12], C[11], OVA[10], OVB[9], DP[8:0]. */
#define TI_C54X_ST0_ARP_SHIFT 13U
#define TI_C54X_ST0_ARP_MASK UINT16_C(0xE000)
#define TI_C54X_ST0_TC_SHIFT 12U
#define TI_C54X_ST0_TC_MASK UINT16_C(0x1000)
#define TI_C54X_ST0_C_SHIFT 11U
#define TI_C54X_ST0_C_MASK UINT16_C(0x0800)
#define TI_C54X_ST0_OVA_MASK UINT16_C(0x0400)
#define TI_C54X_ST0_OVB_MASK UINT16_C(0x0200)
#define TI_C54X_ST0_DP_MASK UINT16_C(0x01FF)

/* ST1: bit 10 is reserved and is not a reset-state assertion. */
#define TI_C54X_ST1_BRAF_MASK UINT16_C(0x8000)
#define TI_C54X_ST1_CPL_SHIFT 14U
#define TI_C54X_ST1_CPL_MASK UINT16_C(0x4000)
#define TI_C54X_ST1_XF_MASK UINT16_C(0x2000)
#define TI_C54X_ST1_HM_MASK UINT16_C(0x1000)
#define TI_C54X_ST1_INTM_MASK UINT16_C(0x0800)
#define TI_C54X_ST1_RESERVED10_MASK UINT16_C(0x0400)
#define TI_C54X_ST1_OVM_MASK UINT16_C(0x0200)
#define TI_C54X_ST1_SXM_MASK UINT16_C(0x0100)
#define TI_C54X_ST1_C16_MASK UINT16_C(0x0080)
#define TI_C54X_ST1_FRCT_MASK UINT16_C(0x0040)
#define TI_C54X_ST1_CMPT_SHIFT 5U
#define TI_C54X_ST1_CMPT_MASK UINT16_C(0x0020)
#define TI_C54X_ST1_ASM_MASK UINT16_C(0x001F)

/* PMST: IPTR[15:7], MP/MC[6], OVLY[5], AVIS[4], DROM[3],
 * CLKOFF[2], SMUL[1], SST[0].  MP/MC is pin-sampled; SMUL/SST are
 * family/profile-dependent in the registered common evidence. */
#define TI_C54X_PMST_IPTR_MASK UINT16_C(0xFF80)
#define TI_C54X_PMST_MP_MC_MASK UINT16_C(0x0040)
#define TI_C54X_PMST_OVLY_MASK UINT16_C(0x0020)
#define TI_C54X_PMST_AVIS_MASK UINT16_C(0x0010)
#define TI_C54X_PMST_DROM_MASK UINT16_C(0x0008)
#define TI_C54X_PMST_CLKOFF_MASK UINT16_C(0x0004)
#define TI_C54X_PMST_SMUL_MASK UINT16_C(0x0002)
#define TI_C54X_PMST_SST_MASK UINT16_C(0x0001)

/* Scalar knownness bits.  A zero-filled value is not architectural unless its
 * corresponding bit is set. */
enum {
    TI_C54X_ARCH_STATE_KNOWN_PC = 1U << 0,
    TI_C54X_ARCH_STATE_KNOWN_IFR = 1U << 1,
    TI_C54X_ARCH_STATE_KNOWN_RC = 1U << 2,
    TI_C54X_ARCH_STATE_KNOWN_SP = 1U << 3,
    TI_C54X_ARCH_STATE_KNOWN_BK = 1U << 4
};

#endif /* TI_C54X_STATE_REGISTER_LAYOUT_H */
