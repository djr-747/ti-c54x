# Source ownership: src/bus/byte_order owns private WORD16 byte-order codecs.
list(APPEND ti_c54x_core_sources
    "${CMAKE_CURRENT_LIST_DIR}/word16_codec.c"
)
