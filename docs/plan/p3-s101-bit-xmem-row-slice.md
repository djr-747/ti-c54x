### P3-S101: BIT Xmem dual row and leaf (implemented; local verification green)

This immutable anchor heads the P3-S101 evidence freeze. One
single-X dual-16-bit row lands on the established dual-XMOD seam:
`BIT Xmem,BITC` (`0x9600`/`0xFF00`, TI SPRU172C section BIT
p. 4-21) with the new `DUAL_XMOD_XAR_BITC` layout (XMOD bits 7:6,
raw XAR bits 5:4, BITC bits 3:0), `AUXILIARY_REGISTERS` input,
and ELIGIBLE repeatability (Table 6-18 omits BIT). The
`op_bit_xmem` leaf stages one X-only dual-indirect WORD16 DATA
READ (Y self-aliases X, as in the ADD Xmem sibling) whose
read-value completion stages TC = Xmem(15 - BITC) as a single
STATUS_BIT_WRITE to ST0 bit 12 with no other status mutation.
All four XMOD codes stage, including MOD3 indexed and circular
commits owned by the shared X-only dual resolver, exactly as in
the LD Xmem SHIFT sibling; checked, trusted, and completion
entries re-validate layout, word count, and the 0x9600 prefix.
No dispatch, sequencing, or CPU change beyond the automatic
X-macro arms. The T2-11-01 form flips to `COMMON_CANDIDATE` /
`COMPLETE` with one binding under the ALL addressing token.
Projected closure adds one TI section record, two
ARCHITECTURAL requirements, and two COMMON test records on the
op and CPU suites: 501 evidence records, 823 requirements, and
677 tests (677 `PASSING`, 0 `PLANNED`); 374 bindings, 179/205
forms (87.317%), 350 rows, 351 IDs, 43352 lookup slots. These
numbers are projections until the slice lands.
