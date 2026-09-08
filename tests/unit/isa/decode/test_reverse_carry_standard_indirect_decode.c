/*
 * Module contract
 *
 * Owns: reverse-carry MOD4/MOD7 decode classification.
 * Does not own: catalog generation, handlers, effects, CPU, or inventory.
 */
#include "../../../../src/isa/catalog/opcode_catalog.h"
#include "../../../../src/isa/decode/instruction_bundle.h"
#include "../../../../src/isa/decode/opcode_decode.h"
#include "../../../../src/isa/decode/operand_layout.h"
#include "../../../support/test_harness.h"
#include <stddef.h>
#include <stdint.h>
typedef struct { ti_c54x_opcode_id id4,id7; uint16_t base4,base7; ti_c54x_operand_layout_id layout; uint8_t arf,mode,ext; } rc_case;
#define PAIR(n,b4,b7,l,a,m,e) {TI_C54X_OPCODE_ID_##n##_MOD4,TI_C54X_OPCODE_ID_##n##_MOD7,UINT16_C(b4),UINT16_C(b7),l,a,m,e}
static const rc_case cases[]={
PAIR(LD_SMEM,0x10A0,0x10B8,TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4,1,2,0),
PAIR(LD_SMEM_TS,0x14A0,0x14B8,TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4,1,2,0),
PAIR(STL_SMEM,0x80A0,0x80B8,TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4,1,2,0),
PAIR(LDU_SMEM,0x12A0,0x12B8,TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4,1,2,0),
PAIR(LD_SMEM_SHIFT16,0x44A0,0x44B8,TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4,1,2,0),
PAIR(LD_T_SMEM,0x30A0,0x30B8,TI_C54X_OPERAND_LAYOUT_ARF_MOD4,0,1,0),
PAIR(ST_IMMEDIATE_SMEM,0x76A0,0x76B8,TI_C54X_OPERAND_LAYOUT_ARF_EXTENSION16_MOD4,0,1,1),
PAIR(LD_DP_SMEM,0x46A0,0x46B8,TI_C54X_OPERAND_LAYOUT_ARF_MOD4,0,1,0),
PAIR(LD_ASM_SMEM,0x32A0,0x32B8,TI_C54X_OPERAND_LAYOUT_ARF_MOD4,0,1,0),
PAIR(ST_T_SMEM,0x8CA0,0x8CB8,TI_C54X_OPERAND_LAYOUT_ARF_MOD4,0,1,0)};
static void check(const rc_case*c,uint16_t w,ti_c54x_opcode_id id,uint8_t mode){ti_c54x_decode_result d=ti_c54x_decode_first_word(w);TI_C54X_TEST_CHECK(d.status==TI_C54X_DECODE_STATUS_OK);TI_C54X_TEST_CHECK(d.id==id);ti_c54x_instruction_word_bundle b={{w,UINT16_C(0xC53A)},c->ext?2U:1U};ti_c54x_decoded_instruction ins={0};TI_C54X_TEST_CHECK(ti_c54x_decode_complete_instruction(&d,&b,&ins)==TI_C54X_INSTRUCTION_COMPLETION_STATUS_OK);TI_C54X_TEST_CHECK(ins.operand_layout==c->layout);(void)mode;}
static void test_rows(void){for(size_t i=0;i<sizeof(cases)/sizeof(cases[0]);++i){const rc_case*c=&cases[i];uint16_t d=(c->layout==TI_C54X_OPERAND_LAYOUT_ACCUMULATOR_ARF_MOD4)?UINT16_C(0x0100):0U;check(c,(uint16_t)(c->base4|UINT16_C(5)|d),c->id4,4U);check(c,(uint16_t)(c->base7|UINT16_C(5)|d),c->id7,7U);/* ARF0 decodes; handlers own rejection. */check(c,(uint16_t)(c->base4|d),c->id4,4U);}}
int main(void){test_rows();return 0;}
