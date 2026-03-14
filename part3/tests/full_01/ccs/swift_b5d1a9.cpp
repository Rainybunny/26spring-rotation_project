llvm::DITypeRefArray createParameterTypes(CanSILFunctionType FnTy) {
    auto params = FnTy->getParameters();
    SmallVector<llvm::Metadata *, 8> Parameters;
    Parameters.reserve(params.size() + 1); // +1 for return type

    GenericContextScope scope(IGM, FnTy->getInvocationGenericSignature());

    // The function return type is the first element in the list.
    createParameterType(Parameters, getResultTypeForDebugInfo(IGM, FnTy));

    for (auto Param : params)
      createParameterType(
          Parameters, IGM.silConv.getSILType(
                          Param, FnTy, IGM.getMaximalTypeExpansionContext()));

    return DBuilder.getOrCreateTypeArray(Parameters);
  }