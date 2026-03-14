local_ref<JMountItem::javaobject> createRemoveAndDeleteMultiMountItem(
  const jni::global_ref<jobject> &javaUIManager,
  std::vector<RemoveDeleteMetadata> metadata) {

  auto env = Environment::current();
  auto removeAndDeleteArray = env->NewIntArray(metadata.size()*4);
  jint* arrayElements = env->GetIntArrayElements(removeAndDeleteArray, nullptr);
  int position = 0;
  for (const auto& x : metadata) {
    arrayElements[position++] = x.tag;
    arrayElements[position++] = x.parentTag;
    arrayElements[position++] = x.index;
    arrayElements[position++] = (x.shouldRemove ? 1 : 0) | (x.shouldDelete ? 2 : 0);
  }
  env->ReleaseIntArrayElements(removeAndDeleteArray, arrayElements, 0);

  static auto removeDeleteMultiInstruction =
    jni::findClassStatic(UIManagerJavaDescriptor)
      ->getMethod<alias_ref<JMountItem>(jintArray)>("removeDeleteMultiMountItem");

  auto ret = removeDeleteMultiInstruction(javaUIManager, removeAndDeleteArray);
  env->DeleteLocalRef(removeAndDeleteArray);
  return ret;
}