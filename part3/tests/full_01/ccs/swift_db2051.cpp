llvm::DITypeRefArray createParameterTypes(CanSILFunctionType FnTy) {
    // Calculate exact capacity needed: parameters + return type
    unsigned NumParams = FnTy->getParameters().size() + 1;
    SmallVector<llvm::Metadata *, 16> Parameters;
    Parameters.reserve(NumParams);  // Pre-allocate exact capacity

    GenericContextScope scope(IGM, FnTy->getInvocationGenericSignature());

    // The function return type is the first element in the list.
    createParameterType(Parameters, getResultTypeForDebugInfo(IGM, FnTy));

    for (auto Param : FnTy->getParameters())
      createParameterType(
          Parameters, IGM.silConv.getSILType(
                          Param, FnTy, IGM.getMaximalTypeExpansionContext()));

    return DBuilder.getOrCreateTypeArray(Parameters);
}