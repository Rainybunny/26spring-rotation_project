folly::Optional<ModuleConfig> ModuleRegistry::getConfig(
    const std::string &name) {
  SystraceSection s("ModuleRegistry::getConfig", "module", name);
  const char* name_cstr = name.c_str(); // Cache the C-string version

  // Initialize modulesByName_
  if (modulesByName_.empty() && !modules_.empty()) {
    moduleNames();
  }

  auto it = modulesByName_.find(name);

  if (it == modulesByName_.end()) {
    if (unknownModules_.find(name) != unknownModules_.end()) {
      BridgeNativeModulePerfLogger::moduleJSRequireBeginningFail(name_cstr);
      BridgeNativeModulePerfLogger::moduleJSRequireEndingStart(name_cstr);
      return folly::none;
    }

    if (!moduleNotFoundCallback_) {
      unknownModules_.insert(name);
      BridgeNativeModulePerfLogger::moduleJSRequireBeginningFail(name_cstr);
      BridgeNativeModulePerfLogger::moduleJSRequireEndingStart(name_cstr);
      return folly::none;
    }

    BridgeNativeModulePerfLogger::moduleJSRequireBeginningEnd(name_cstr);

    bool wasModuleLazilyLoaded = moduleNotFoundCallback_(name);
    it = modulesByName_.find(name);

    bool wasModuleRegisteredWithRegistry =
        wasModuleLazilyLoaded && it != modulesByName_.end();

    if (!wasModuleRegisteredWithRegistry) {
      BridgeNativeModulePerfLogger::moduleJSRequireEndingStart(name_cstr);
      unknownModules_.insert(name);
      return folly::none;
    }
  } else {
    BridgeNativeModulePerfLogger::moduleJSRequireBeginningEnd(name_cstr);
  }

  // If we've gotten this far, then we've signaled moduleJSRequireBeginningEnd

  size_t index = it->second;

  CHECK(index < modules_.size());
  NativeModule *module = modules_[index].get();

  // string name, object constants, array methodNames (methodId is index),
  // [array promiseMethodIds], [array syncMethodIds]
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

    for (auto &descriptor : methods) {
      methodNames.push_back(std::move(descriptor.name));
      if (descriptor.type == "promise") {
        promiseMethodIds.push_back(methodNames.size() - 1);
      } else if (descriptor.type == "sync") {
        syncMethodIds.push_back(methodNames.size() - 1);
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
  } else {
    return ModuleConfig{index, config};
  }
}