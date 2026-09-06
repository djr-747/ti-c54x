# Source ownership: src/isa/ops/logical/immediate/or owns the OR immediate
# family leaf and delegates shared semantics to the common logical-immediate
# module.
list(APPEND ti_c54x_core_sources
    "${CMAKE_CURRENT_LIST_DIR}/op_or.c")
