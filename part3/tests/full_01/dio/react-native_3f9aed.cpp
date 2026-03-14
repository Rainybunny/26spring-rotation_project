folly::Optional<ModuleConfig> ModuleRegistry::getConfig(
    const std::string &name) {
  SystraceSection s("ModuleRegistry::getConfig", "module", name);

  // Initialize modulesByName_
  if (modulesByName_.empty() && !modules_.empty()) {
    moduleNames();
  }

  auto it = modulesByName_.find(name);

  if (it == modulesByName_.end()) {
    if (unknownModules_.find(name) != unknownModules_.end()) {
      BridgeNativeModulePerfLogger::moduleJSRequireBeginningFail(name.c_str());
      BridgeNativeModulePerfLogger::moduleJSRequireEndingStart(name.c_str());
      return folly::none;
    }

    if (!moduleNotFoundCallback_) {
      unknownModules_.insert(name);
      BridgeNativeModulePerfLogger::moduleJSRequireBeginningFail(name.c_str());
      BridgeNativeModulePerfLogger::moduleJSRequireEndingStart(name.c_str());
      return folly::none;
    }

    BridgeNativeModulePerfLogger::moduleJSRequireBeginningEnd(name.c_str());

    bool wasModuleLazilyLoaded = moduleNotFoundCallback_(name);
    it = modulesByName_.find(name);

    bool wasModuleRegisteredWithRegistry =
        wasModuleLazilyLoaded && it != modulesByName_.end();

    if (!wasModuleRegisteredWithRegistry) {
      BridgeNativeModulePerfLogger::moduleJSRequireEndingStart(name.c_str());
      unknownModules_.insert(name);
      return folly::none;
    }
  } else {
    BridgeNativeModulePerfLogger::moduleJSRequireBeginningEnd(name.c_str());
  }

  size_t index = it->second;
  CHECK(index < modules_.size());
  NativeModule *module = modules_[index].get();

  folly::dynamic config = folly::dynamic::array(name);

  {
    SystraceSection s_("ModuleRegistry::getConstants", "module", name);
    config.push_back(module->getConstants());
  }

  {
    SystraceSection s_("ModuleRegistry::getMethods", "module", name);
    std::vector<MethodDescriptor> methods = module->getMethods();

    folly::dynamic methodNames = folly::dynamic::array;
    folly::dynamic promiseMethodIds = folly::dynamic::array;
    folly::dynamic syncMethodIds = folly::dynamic::array;

    methodNames.reserve(methods.size());
    for (size_t i = 0; i < methods.size(); ++i) {
      auto& descriptor = methods[i];
      methodNames.push_back(descriptor.name);
      if (descriptor.type == "promise") {
        promiseMethodIds.push_back(i);
      } else if (descriptor.type == "sync") {
        syncMethodIds.push_back(i);
      }
    }

    if (!methodNames.empty()) {
      config.push_back(std::move(methodNames));
      if (!promiseMethodIds.empty() || !syncMethodIds.empty()) {
        config.push_back(std::move(promiseMethodIds));
        if (!syncMethodIds.empty()) {
          config.push_back(std::move(syncMethodIds));
        }
      }
    }
  }

  if (config.size() == 2 && config[1].empty()) {
    return folly::none;
  }
  return ModuleConfig{index, config};
}