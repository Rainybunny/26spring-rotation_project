static int gbDisass(RAsmOp *op, const ut8 *buf, ut64 len) {
	int foo = gbOpLength (gb_op[buf[0]].type);
	if (len < foo)
		return 0;
	switch (gb_op[buf[0]].type) {
	case GB_8BIT:
		snprintf (op->buf_asm, sizeof(op->buf_asm), "%s", gb_op[buf[0]].name);
		break;
	case GB_16BIT: {
		const char *op_str = cb_ops[buf[1]/8];
		const char *reg_str = cb_regs[buf[1]%8];
		snprintf (op->buf_asm, sizeof(op->buf_asm), "%s %s", op_str, reg_str);
		break;
	}
	case GB_8BIT+ARG_8:
		snprintf (op->buf_asm, sizeof(op->buf_asm), gb_op[buf[0]].name, buf[1]);
		break;
	case GB_8BIT+ARG_16: {
		ut16 val = buf[1] + 0x100 * buf[2];
		snprintf (op->buf_asm, sizeof(op->buf_asm), gb_op[buf[0]].name, val);
		break;
	}
	case GB_8BIT+ARG_8+GB_IO:
		snprintf (op->buf_asm, sizeof(op->buf_asm), gb_op[buf[0]].name, 0xff00 + buf[1]);
		break;
	}
	return foo;
}