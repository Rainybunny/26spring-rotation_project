llvm::DITypeRefArray createParameterTypes(CanSILFunctionType FnTy) {
  // Check cache first
  static llvm::DenseMap<CanSILFunctionType, llvm::DITypeRefArray> ParamTypeCache;
  auto It = ParamTypeCache.find(FnTy);
  if (It != ParamTypeCache.end())
    return It->second;

  SmallVector<llvm::Metadata *, 16> Parameters;

  GenericContextScope scope(IGM, FnTy->getInvocationGenericSignature());

  // The function return type is the first element in the list.
  createParameterType(Parameters, getResultTypeForDebugInfo(IGM, FnTy));

  for (auto Param : FnTy->getParameters())
    createParameterType(
        Parameters, IGM.silConv.getSILType(
                        Param, FnTy, IGM.getMaximalTypeExpansionContext()));

  // Cache the result before returning
  auto Result = DBuilder.getOrCreateTypeArray(Parameters);
  ParamTypeCache[FnTy] = Result;
  return Result;
}