constexpr static f32x4 edge_function4(const FloatVector2& a, const FloatVector2& b, const Vector2<f32x4>& c)
{
#ifdef __SSE2__
    // Load a and b into SSE registers
    __m128 a_x = _mm_set1_ps(a.x());
    __m128 a_y = _mm_set1_ps(a.y());
    __m128 b_x = _mm_set1_ps(b.x());
    __m128 b_y = _mm_set1_ps(b.y());
    
    // Calculate (c.x - a.x) and (b.y - a.y)
    __m128 cx_minus_ax = _mm_sub_ps(c.x().v, a_x);
    __m128 by_minus_ay = _mm_sub_ps(b_y, a_y);
    
    // Calculate (c.y - a.y) and (b.x - a.x)
    __m128 cy_minus_ay = _mm_sub_ps(c.y().v, a_y);
    __m128 bx_minus_ax = _mm_sub_ps(b_x, a_x);
    
    // Multiply components
    __m128 term1 = _mm_mul_ps(cx_minus_ax, by_minus_ay);
    __m128 term2 = _mm_mul_ps(cy_minus_ay, bx_minus_ax);
    
    // Subtract and return
    return f32x4(_mm_sub_ps(term1, term2));
#else
    // Fallback to original implementation if no SSE2
    return (c.x() - a.x()) * (b.y() - a.y()) - (c.y() - a.y()) * (b.x() - a.x());
#endif
}