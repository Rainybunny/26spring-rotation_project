llvm::DITypeRefArray createParameterTypes(CanSILFunctionType FnTy) {
    auto params = FnTy->getParameters();
    SmallVector<llvm::Metadata *, 16> Parameters;
    // Reserve space for return type + all parameters
    Parameters.reserve(1 + params.size());

    GenericContextScope scope(IGM, FnTy->getInvocationGenericSignature());

    // The function return type is the first element in the list.
    createParameterType(Parameters, getResultTypeForDebugInfo(IGM, FnTy));

    for (auto Param : params)
      createParameterType(
          Parameters, IGM.silConv.getSILType(
                          Param, FnTy, IGM.getMaximalTypeExpansionContext()));

    return DBuilder.getOrCreateTypeArray(Parameters);
  }