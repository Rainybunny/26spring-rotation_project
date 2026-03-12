llvm::DITypeRefArray createParameterTypes(CanSILFunctionType FnTy) {
  // First check the cache
  static llvm::DenseMap<CanSILFunctionType, llvm::DITypeRefArray> ParamTypeCache;
  auto it = ParamTypeCache.find(FnTy);
  if (it != ParamTypeCache.end())
    return it->second;

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