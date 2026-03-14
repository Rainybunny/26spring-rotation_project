void ViewShadowNode::initialize() noexcept {
  auto &viewProps = static_cast<ViewProps const &>(*props_);

  // Check cheaper conditions first for early termination
  bool formsStackingContext = !viewProps.collapsable ||
      !viewProps.nativeId.empty() || viewProps.accessible ||
      viewProps.opacity != 1.0 || viewProps.elevation != 0 ||
      viewProps.pointerEvents == PointerEventsMode::None ||
      viewProps.transform != Transform{} ||
      viewProps.accessibilityElementsHidden ||
      viewProps.accessibilityViewIsModal ||
      viewProps.removeClippedSubviews ||
      viewProps.importantForAccessibility != ImportantForAccessibility::Auto ||
      (viewProps.zIndex.has_value() &&
       viewProps.yogaStyle.positionType() != YGPositionTypeStatic) ||
      viewProps.yogaStyle.display() == YGDisplayNone ||
      viewProps.getClipsContentToBounds() ||
      isColorMeaningful(viewProps.shadowColor);

  bool formsView = formsStackingContext || !viewProps.testId.empty() ||
      !(viewProps.yogaStyle.border() == YGStyle::Edges{}) ||
      isColorMeaningful(viewProps.backgroundColor) ||
      isColorMeaningful(viewProps.foregroundColor);

  traits_.set(ShadowNodeTraits::Trait::FormsView, formsView);
  traits_.set(ShadowNodeTraits::Trait::FormsStackingContext, formsStackingContext);
}