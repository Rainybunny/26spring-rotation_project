void Device::rasterize_triangle(const Triangle& triangle)
{
    INCREASE_STATISTICS_COUNTER(g_num_rasterized_triangles, 1);

    // Return if alpha testing is a no-op
    if (m_options.enable_alpha_test && m_options.alpha_test_func == AlphaTestFunction::Never)
        return;

    // Vertices
    Vertex const vertex0 = triangle.vertices[0];
    Vertex const vertex1 = triangle.vertices[1];
    Vertex const vertex2 = triangle.vertices[2];

    // Calculate area of the triangle for later tests
    FloatVector2 const v0 = vertex0.window_coordinates.xy();
    FloatVector2 const v1 = vertex1.window_coordinates.xy();
    FloatVector2 const v2 = vertex2.window_coordinates.xy();

    auto const area = edge_function(v0, v1, v2);
    auto const one_over_area = 1.0f / area;

    // Precompute edge coefficients
    auto const v1y_minus_v2y = v1.y() - v2.y();
    auto const v2x_minus_v1x = v2.x() - v1.x();
    auto const v2y_minus_v0y = v2.y() - v0.y();
    auto const v0x_minus_v2x = v0.x() - v2.x();
    auto const v0y_minus_v1y = v0.y() - v1.y();
    auto const v1x_minus_v0x = v1.x() - v0.x();

    auto render_bounds = m_frame_buffer->rect();
    if (m_options.scissor_enabled)
        render_bounds.intersect(m_options.scissor_box);

    // Optimized edge function calculation using precomputed coefficients
    auto calculate_edge_values4 = [=](Vector2<f32x4> const& p) -> Vector3<f32x4> {
        return {
            (p.x() - v2.x()) * v1y_minus_v2y - (p.y() - v2.y()) * v2x_minus_v1x,
            (p.x() - v0.x()) * v2y_minus_v0y - (p.y() - v0.y()) * v0x_minus_v2x,
            (p.x() - v1.x()) * v0y_minus_v1y - (p.y() - v1.y()) * v1x_minus_v0x,
        };
    };

    // Zero is used in testing against edge values below, applying the "top-left rule". If a pixel
    // lies exactly on an edge shared by two triangles, we only render that pixel if the edge in
    // question is a "top" or "left" edge. We can detect those easily by testing for Y2 <= Y1,
    // since we know our vertices are in CCW order. By changing a float epsilon to 0, we
    // effectively change the comparisons against the edge values below from "> 0" into ">= 0".
    constexpr auto epsilon = NumericLimits<float>::epsilon();
    FloatVector3 zero { epsilon, epsilon, epsilon };
    if (v2.y() <= v1.y())
        zero.set_x(0.f);
    if (v0.y() <= v2.y())
        zero.set_y(0.f);
    if (v1.y() <= v0.y())
        zero.set_z(0.f);

    // This function tests whether a point as identified by its 3 edge values lies within the triangle
    auto test_point4 = [zero](Vector3<f32x4> const& edges) -> i32x4 {
        return edges.x() >= zero.x()
            && edges.y() >= zero.y()
            && edges.z() >= zero.z();
    };

    // [Rest of the function remains exactly the same...]
    // Calculate block-based bounds
    // clang-format off
    int const bx0 =  max(render_bounds.left(),   min(min(v0.x(), v1.x()), v2.x())) & ~1;
    int const bx1 = (min(render_bounds.right(),  max(max(v0.x(), v1.x()), v2.x())) & ~1) + 2;
    int const by0 =  max(render_bounds.top(),    min(min(v0.y(), v1.y()), v2.y())) & ~1;
    int const by1 = (min(render_bounds.bottom(), max(max(v0.y(), v1.y()), v2.y())) & ~1) + 2;
    // clang-format on

    // [Rest of the original function implementation...]
}