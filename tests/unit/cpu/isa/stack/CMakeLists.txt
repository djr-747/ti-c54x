# Test ownership: this leaf owns only public-CPU FRAME proofs.
ti_c54x_add_unit_test(cpu_frame_k test_cpu_frame_k.c)
# Test ownership: this leaf owns only public-CPU PSHD/POPD/PSHM/POPM proofs.
ti_c54x_add_unit_test(cpu_pshd_popd_direct test_cpu_pshd_popd_direct.c)
