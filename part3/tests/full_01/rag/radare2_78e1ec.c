R_API const char* r_reg_32_to_64(RReg* reg, const char* rreg32) {
	int i;
	RListIter *iter;
	RRegItem *item;
	
	for (i = 0; i < R_REG_TYPE_LAST; ++i) {
		r_list_foreach (reg->regset[i].regs, iter, item) {
			if (!r_str_casecmp (rreg32, item->name)) {
				if (item->size == 32) {
					// Found 32-bit version, look for 64-bit at same offset
					int offset = item->offset;
					RListIter *iter2;
					RRegItem *item2;
					for (int j = 0; j < R_REG_TYPE_LAST; ++j) {
						r_list_foreach (reg->regset[j].regs, iter2, item2) {
							if (item2->offset == offset && item2->size == 64) {
								return item2->name;
							}
						}
					}
					return NULL; // No 64-bit version found
				} else if (item->size == 64) {
					// Found 64-bit version first, check if there's a 32-bit at same offset
					int offset = item->offset;
					RListIter *iter2;
					RRegItem *item2;
					for (int j = 0; j < R_REG_TYPE_LAST; ++j) {
						r_list_foreach (reg->regset[j].regs, iter2, item2) {
							if (item2->offset == offset && item2->size == 32 && 
								!r_str_casecmp (rreg32, item2->name)) {
								return item->name;
							}
						}
					}
				}
			}
		}
	}
	return NULL;
}