void ViewShadowNode::initialize() noexcept {
  auto &viewProps = static_cast<ViewProps const &>(*props_);

  bool formsStackingContext = 
      // Simple and likely conditions first
      !viewProps.collapsable ||
      viewProps.opacity != 1.0 ||
      viewProps.elevation != 0 ||
      viewProps.yogaStyle.display() == YGDisplayNone ||
      viewProps.getClipsContentToBounds() ||
      viewProps.removeClippedSubviews ||
      // Then more complex conditions
      viewProps.pointerEvents == PointerEventsMode::None ||
      !viewProps.nativeId.empty() || 
      viewProps.accessible ||
      viewProps.transform != Transform{} ||
      (viewProps.zIndex.has_value() &&
       viewProps.yogaStyle.positionType() != YGPositionTypeStatic) ||
      isColorMeaningful(viewProps.shadowColor) ||
      viewProps.accessibilityElementsHidden ||
      viewProps.accessibilityViewIsModal ||
      viewProps.importantForAccessibility != ImportantForAccessibility::Auto;

  bool formsView = 
      // Simple checks first
      !viewProps.testId.empty() ||
      !(viewProps.yogaStyle.border() == YGStyle::Edges{}) ||
      // More expensive checks last
      isColorMeaningful(viewProps.backgroundColor) ||
      isColorMeaningful(viewProps.foregroundColor);

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