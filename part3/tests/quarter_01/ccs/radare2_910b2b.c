static int gbDisass(RAsmOp *op, const ut8 *buf, ut64 len) {
    const ut8 opcode = buf[0];
    const GB_op_def *op_def = &gb_op[opcode];
    int foo = gbOpLength(op_def->type);
    if (len < foo)
        return 0;
    switch (op_def->type) {
    case GB_8BIT:
        sprintf(op->buf_asm, "%s", op_def->name);
        break;
    case GB_16BIT:
        sprintf(op->buf_asm, "%s %s", cb_ops[buf[1]/8], cb_regs[buf[1]%8]);
        break;
    case GB_8BIT+ARG_8:
        sprintf(op->buf_asm, op_def->name, buf[1]);
        break;
    case GB_8BIT+ARG_16:
        sprintf(op->buf_asm, op_def->name, buf[1]+0x100*buf[2]);
        break;
    case GB_8BIT+ARG_8+GB_IO:
        sprintf(op->buf_asm, op_def->name, 0xff00+buf[1]);
        break;
    }
    return foo;
}