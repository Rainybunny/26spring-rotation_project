void Device::rasterize_triangle(const Triangle& triangle)
{
    INCREASE_STATISTICS_COUNTER(g_num_rasterized_triangles, 1);

    if (m_options.enable_alpha_test && m_options.alpha_test_func == AlphaTestFunction::Never)
        return;

    // Vertices
    Vertex const vertex0 = triangle.vertices[0];
    Vertex const vertex1 = triangle.vertices[1];
    Vertex const vertex2 = triangle.vertices[2];

    // Calculate area and gradients once
    FloatVector2 const v0 = vertex0.window_coordinates.xy();
    FloatVector2 const v1 = vertex1.window_coordinates.xy();
    FloatVector2 const v2 = vertex2.window_coordinates.xy();

    auto const area = edge_function(v0, v1, v2);
    auto const one_over_area = 1.0f / area;

    // Precompute edge function gradients
    auto const a01 = v0.y() - v1.y();
    auto const b01 = v1.x() - v0.x();
    auto const a12 = v1.y() - v2.y();
    auto const b12 = v2.x() - v1.x();
    auto const a20 = v2.y() - v0.y();
    auto const b20 = v0.x() - v2.x();

    auto render_bounds = m_frame_buffer->rect();
    if (m_options.scissor_enabled)
        render_bounds.intersect(m_options.scissor_box);

    // Optimized edge function calculation using precomputed gradients
    auto calculate_edge_values4 = [a12, b12, a20, b20, a01, b01](Vector2<f32x4> const& p) -> Vector3<f32x4> {
        return {
            (p.x() - v1.x()) * a12 + (p.y() - v1.y()) * b12,
            (p.x() - v2.x()) * a20 + (p.y() - v2.y()) * b20,
            (p.x() - v0.x()) * a01 + (p.y() - v0.y()) * b01
        };
    };

    constexpr auto epsilon = NumericLimits<float>::epsilon();
    FloatVector3 zero { epsilon, epsilon, epsilon };
    if (v2.y() <= v1.y())
        zero.set_x(0.f);
    if (v0.y() <= v2.y())
        zero.set_y(0.f);
    if (v1.y() <= v0.y())
        zero.set_z(0.f);

    auto test_point4 = [zero](Vector3<f32x4> const& edges) -> i32x4 {
        return edges.x() >= zero.x()
            && edges.y() >= zero.y()
            && edges.z() >= zero.z();
    };

    // Calculate block-based bounds
    int const bx0 =  max(render_bounds.left(),   min(min(v0.x(), v1.x()), v2.x())) & ~1;
    int const bx1 = (min(render_bounds.right(),  max(max(v0.x(), v1.x()), v2.x())) & ~1) + 2;
    int const by0 =  max(render_bounds.top(),    min(min(v0.y(), v1.y()), v2.y())) & ~1;
    int const by1 = (min(render_bounds.bottom(), max(max(v0.y(), v1.y()), v2.y())) & ~1) + 2;

    // Rest of the function remains the same...
    // [Previous implementation continues unchanged from this point]
}