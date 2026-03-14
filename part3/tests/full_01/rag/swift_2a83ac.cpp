/// Emit a boolean expression as a control-flow condition.
///
/// \param hasFalseCode - true if the false branch doesn't just lead
/// to the fallthrough.
/// \param invertValue - true if this routine should invert the value before
/// testing true/false.
Condition IRGenFunction::emitCondition(Expr *E, bool hasFalseCode,
                                       bool invertValue) {
  assert(Builder.hasValidIP() && "emitting condition at unreachable point");

  // Sema forces conditions to have Builtin.i1 type, which guarantees this.
  llvm::Value *V = nullptr;
  llvm::ConstantInt *C = nullptr;

  // First try to evaluate as constant if possible
  if (E->isConstant()) {
    {
      FullExpr Scope(*this);
      V = emitAsPrimitiveScalar(E);
    }
    C = dyn_cast<llvm::ConstantInt>(V);
  }

  // If not constant, emit normally
  if (!C) {
    FullExpr Scope(*this);
    V = emitAsPrimitiveScalar(E);
    assert(V->getType()->isIntegerTy(1));
  }

  llvm::BasicBlock *trueBB = nullptr;
  llvm::BasicBlock *falseBB = nullptr;
  llvm::BasicBlock *contBB = nullptr;

  // Handle constant case if we have it
  if (C) {
    bool constVal = C->isZero() ? !invertValue : invertValue;
    if (constVal) {
      trueBB = Builder.GetInsertBlock();
      falseBB = (hasFalseCode ? Builder.GetInsertBlock() : nullptr);
    } else {
      trueBB = nullptr;
      falseBB = (hasFalseCode ? Builder.GetInsertBlock() : nullptr);
    }
    contBB = nullptr;
  } else {
    // Handle non-constant case
    if (invertValue) {
      if (llvm::ConstantInt *VC = dyn_cast<llvm::ConstantInt>(V)) {
        // Optimize inversion of constant values
        V = llvm::ConstantInt::get(IGM.Int1Ty, !VC->getZExtValue());
      } else {
        V = Builder.CreateXor(V, llvm::ConstantInt::get(IGM.Int1Ty, 1));
      }
    }
    
    contBB = createBasicBlock("condition.cont");
    trueBB = createBasicBlock("if.true");

    llvm::BasicBlock *falseDestBB;
    if (hasFalseCode) {
      falseBB = falseDestBB = createBasicBlock("if.false");
    } else {
      falseBB = nullptr;
      falseDestBB = contBB;
    }

    Builder.CreateCondBr(V, trueBB, falseDestBB);
  }

  return Condition(trueBB, falseBB, contBB);
}