llvm::DITypeRefArray createParameterTypes(CanSILFunctionType FnTy) {
    // Pre-allocate vector with exact size needed
    size_t numParams = FnTy->getParameters().size() + 1; // +1 for return type
    SmallVector<llvm::Metadata *, 16> Parameters;
    Parameters.reserve(numParams);

    GenericContextScope scope(IGM, FnTy->getInvocationGenericSignature());

    // The function return type is the first element in the list.
    createParameterType(Parameters, getResultTypeForDebugInfo(IGM, FnTy));

    // Process parameters
    auto params = FnTy->getParameters();
    for (auto Param : params) {
      createParameterType(
          Parameters, IGM.silConv.getSILType(
                          Param, FnTy, IGM.getMaximalTypeExpansionContext()));
    }

    return DBuilder.getOrCreateTypeArray(Parameters);
}