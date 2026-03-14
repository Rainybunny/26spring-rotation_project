void ViewShadowNode::initialize() noexcept {
  auto &viewProps = static_cast<ViewProps const &>(*props_);

  // Reordered conditions with most likely true first
  bool formsStackingContext = !viewProps.collapsable ||
      viewProps.opacity != 1.0 ||
      viewProps.transform != Transform{} ||
      viewProps.elevation != 0 ||
      (viewProps.zIndex.has_value() &&
       viewProps.yogaStyle.positionType() != YGPositionTypeStatic) ||
      viewProps.yogaStyle.display() == YGDisplayNone ||
      viewProps.getClipsContentToBounds() ||
      viewProps.pointerEvents == PointerEventsMode::None ||
      !viewProps.nativeId.empty() ||
      viewProps.accessible ||
      viewProps.accessibilityElementsHidden ||
      viewProps.accessibilityViewIsModal ||
      viewProps.importantForAccessibility != ImportantForAccessibility::Auto ||
      viewProps.removeClippedSubviews ||
      isColorMeaningful(viewProps.shadowColor);

  bool formsView = !viewProps.testId.empty() ||
      !(viewProps.yogaStyle.border() == YGStyle::Edges{}) ||
      isColorMeaningful(viewProps.backgroundColor) ||
      isColorMeaningful(viewProps.foregroundColor) ||
      formsStackingContext;

  traits_.set(ShadowNodeTraits::Trait::FormsView, formsView);
  traits_.set(ShadowNodeTraits::Trait::FormsStackingContext, formsStackingContext);
}