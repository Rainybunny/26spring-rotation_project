R_API const char* r_reg_32_to_64(RReg* reg, const char* rreg32) {
	int i;
	RListIter *iter, *iter2;
	RRegItem *item32, *item64;
	
	for (i = 0; i < R_REG_TYPE_LAST; ++i) {
		r_list_foreach (reg->regset[i].regs, iter, item32) {
			if (!r_str_casecmp (rreg32, item32->name) && item32->size == 32) {
				// Found 32-bit register, look for 64-bit in same regset
				r_list_foreach (reg->regset[i].regs, iter2, item64) {
					if (item64->offset == item32->offset && item64->size == 64) {
						return item64->name;
					}
				}
				break; // No need to check other registers in this type
			}
		}
	}
	return NULL;
}