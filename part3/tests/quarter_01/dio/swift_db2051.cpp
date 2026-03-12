llvm::DITypeRefArray createParameterTypes(CanSILFunctionType FnTy) {
    // Pre-allocate vector with exact size needed
    size_t numParams = FnTy->getParameters().size() + 1; // +1 for return type
    SmallVector<llvm::Metadata *, 16> Parameters;
    Parameters.reserve(numParams);

    GenericContextScope scope(IGM, FnTy->getInvocationGenericSignature());

    // The function return type is the first element in the list.
    createParameterType(Parameters, getResultTypeForDebugInfo(IGM, FnTy));

    // Add parameter types
    for (auto Param : FnTy->getParameters()) {
      createParameterType(
          Parameters, IGM.silConv.getSILType(
                          Param, FnTy, IGM.getMaximalTypeExpansionContext()));
    }

    return DBuilder.getOrCreateTypeArray(Parameters);
}