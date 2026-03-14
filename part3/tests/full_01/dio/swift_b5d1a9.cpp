llvm::DITypeRefArray createParameterTypes(CanSILFunctionType FnTy) {
  ParametersStorage.clear();

  GenericContextScope scope(IGM, FnTy->getInvocationGenericSignature());

  // The function return type is the first element in the list.
  createParameterType(ParametersStorage, getResultTypeForDebugInfo(IGM, FnTy));

  for (auto Param : FnTy->getParameters())
    createParameterType(
        ParametersStorage, IGM.silConv.getSILType(
                            Param, FnTy, IGM.getMaximalTypeExpansionContext()));

  return DBuilder.getOrCreateTypeArray(ParametersStorage);
}