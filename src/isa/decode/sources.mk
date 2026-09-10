# Source ownership: src/isa/decode owns pure instruction classification and
# operand-field extraction.
ti_c54x_core_sources += \
    src/isa/decode/opcode_decode.c \
    src/isa/decode/operand_field.c \
    src/isa/decode/operand_layout.c \
    src/isa/decode/operand_layout_sets.c \
    src/isa/decode/trusted_execution_packet.c
