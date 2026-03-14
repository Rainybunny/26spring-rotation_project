void ViewShadowNode::initialize() noexcept {
  auto &viewProps = static_cast<ViewProps const &>(*props_);

  // Cache expensive isColorMeaningful() results
  const bool shadowColorMeaningful = isColorMeaningful(viewProps.shadowColor);
  const bool backgroundColorMeaningful = isColorMeaningful(viewProps.backgroundColor);
  const bool foregroundColorMeaningful = isColorMeaningful(viewProps.foregroundColor);

  bool formsStackingContext = !viewProps.collapsable ||
      viewProps.pointerEvents == PointerEventsMode::None ||
      !viewProps.nativeId.empty() || viewProps.accessible ||
      viewProps.opacity != 1.0 || viewProps.transform != Transform{} ||
      viewProps.elevation != 0 ||
      (viewProps.zIndex.has_value() &&
       viewProps.yogaStyle.positionType() != YGPositionTypeStatic) ||
      viewProps.yogaStyle.display() == YGDisplayNone ||
      viewProps.getClipsContentToBounds() ||
      shadowColorMeaningful ||
      viewProps.accessibilityElementsHidden ||
      viewProps.accessibilityViewIsModal ||
      viewProps.importantForAccessibility != ImportantForAccessibility::Auto ||
      viewProps.removeClippedSubviews;

  bool formsView = backgroundColorMeaningful ||
      foregroundColorMeaningful ||
      !(viewProps.yogaStyle.border() == YGStyle::Edges{}) ||
      !viewProps.testId.empty();

  formsView = formsView || formsStackingContext;

  if (formsView) {
    traits_.set(ShadowNodeTraits::Trait::FormsView);
  } else {
    traits_.unset(ShadowNodeTraits::Trait::FormsView);
  }

  if (formsStackingContext) {
    traits_.set(ShadowNodeTraits::Trait::FormsStackingContext);
  } else {
    traits_.unset(ShadowNodeTraits::Trait::FormsStackingContext);
  }
}