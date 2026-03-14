void ViewShadowNode::initialize() noexcept {
  auto &viewProps = static_cast<ViewProps const &>(*props_);

  // Reordered conditions for better short-circuiting
  bool formsStackingContext = 
      viewProps.opacity != 1.0 ||
      viewProps.transform != Transform{} ||
      !viewProps.collapsable ||
      viewProps.pointerEvents == PointerEventsMode::None ||
      viewProps.elevation != 0 ||
      (viewProps.zIndex.has_value() &&
       viewProps.yogaStyle.positionType() != YGPositionTypeStatic) ||
      viewProps.yogaStyle.display() == YGDisplayNone ||
      viewProps.getClipsContentToBounds() ||
      isColorMeaningful(viewProps.shadowColor) ||
      !viewProps.nativeId.empty() ||
      viewProps.accessible ||
      viewProps.accessibilityElementsHidden ||
      viewProps.accessibilityViewIsModal ||
      viewProps.importantForAccessibility != ImportantForAccessibility::Auto ||
      viewProps.removeClippedSubviews;

  // Group color-related checks together
  bool formsView = isColorMeaningful(viewProps.backgroundColor) ||
      isColorMeaningful(viewProps.foregroundColor) ||
      !viewProps.testId.empty() ||
      !(viewProps.yogaStyle.border() == YGStyle::Edges{});

  formsView = formsView || formsStackingContext;

  // Use direct assignment since set/unset are just setting bits
  traits_.set(ShadowNodeTraits::Trait::FormsView, formsView);
  traits_.set(ShadowNodeTraits::Trait::FormsStackingContext, formsStackingContext);
}