llvm::DITypeRefArray createParameterTypes(CanSILFunctionType FnTy) {
  // Cache lookup - functions with identical signatures will reuse the same debug info
  static llvm::DenseMap<CanSILFunctionType, llvm::DITypeRefArray> TypeCache;
  auto It = TypeCache.find(FnTy);
  if (It != TypeCache.end())
    return It->second;

  SmallVector<llvm::Metadata *, 16> Parameters;
  Parameters.reserve(FnTy->getParameters().size() + 1); // +1 for return type

  {
    GenericContextScope scope(IGM, FnTy->getInvocationGenericSignature());
    
    // Process return type
    createParameterType(Parameters, getResultTypeForDebugInfo(IGM, FnTy));

    // Process parameters
    for (auto Param : FnTy->getParameters()) {
      createParameterType(
          Parameters, 
          IGM.silConv.getSILType(Param, FnTy, IGM.getMaximalTypeExpansionContext())
      );
    }
  }

  // Cache the result
  auto Result = DBuilder.getOrCreateTypeArray(Parameters);
  TypeCache[FnTy] = Result;
  return Result;
}