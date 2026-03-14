void Device::rasterize_triangle(const Triangle& triangle)
{
    INCREASE_STATISTICS_COUNTER(g_num_rasterized_triangles, 1);

    if (m_options.enable_alpha_test && m_options.alpha_test_func == AlphaTestFunction::Never)
        return;

    // Vertices
    Vertex const vertex0 = triangle.vertices[0];
    Vertex const vertex1 = triangle.vertices[1];
    Vertex const vertex2 = triangle.vertices[2];

    // Calculate area and precompute edge function values
    FloatVector2 const v0 = vertex0.window_coordinates.xy();
    FloatVector2 const v1 = vertex1.window_coordinates.xy();
    FloatVector2 const v2 = vertex2.window_coordinates.xy();

    // Precompute edge function values
    float const edge0_dx = v2.y() - v1.y();
    float const edge0_dy = v1.x() - v2.x();
    float const edge1_dx = v0.y() - v2.y();
    float const edge1_dy = v2.x() - v0.x();
    float const edge2_dx = v1.y() - v0.y();
    float const edge2_dy = v0.x() - v1.x();

    float const area = edge_function(v0, v1, v2);
    float const one_over_area = 1.0f / area;

    // Precompute zero thresholds for edge tests
    constexpr auto epsilon = NumericLimits<float>::epsilon();
    FloatVector3 zero { epsilon, epsilon, epsilon };
    if (v2.y() <= v1.y()) zero.set_x(0.f);
    if (v0.y() <= v2.y()) zero.set_y(0.f);
    if (v1.y() <= v0.y()) zero.set_z(0.f);

    // Precompute fog depths if needed
    f32x4 vertex0_fog_depth, vertex1_fog_depth, vertex2_fog_depth;
    if (m_options.fog_enabled) {
        vertex0_fog_depth = expand4(fabsf(vertex0.eye_coordinates.z()));
        vertex1_fog_depth = expand4(fabsf(vertex1.eye_coordinates.z()));
        vertex2_fog_depth = expand4(fabsf(vertex2.eye_coordinates.z()));
    }

    // Precompute w coordinates for perspective correction
    Vector3<f32x4> const w_coordinates = {
        expand4(vertex0.window_coordinates.w()),
        expand4(vertex1.window_coordinates.w()),
        expand4(vertex2.window_coordinates.w())
    };

    // Get render bounds
    auto render_bounds = m_options.scissor_enabled 
        ? m_frame_buffer->rect().intersected(m_options.scissor_box)
        : m_frame_buffer->rect();

    // Calculate block-based bounds
    int const bx0 = max(render_bounds.left(), min(min(v0.x(), v1.x()), v2.x())) & ~1;
    int const bx1 = (min(render_bounds.right(), max(max(v0.x(), v1.x()), v2.x())) & ~1) + 2;
    int const by0 = max(render_bounds.top(), min(min(v0.y(), v1.y()), v2.y())) & ~1;
    int const by1 = (min(render_bounds.bottom(), max(max(v0.y(), v1.y()), v2.y())) & ~1) + 2;

    // Get buffer pointers
    auto color_buffer = m_frame_buffer->color_buffer();
    auto depth_buffer = m_frame_buffer->depth_buffer();
    auto stencil_buffer = m_frame_buffer->stencil_buffer();

    // Stencil configuration
    auto const stencil_configuration = m_stencil_configuration[Face::Front];
    auto const stencil_reference_value = stencil_configuration.reference_value & stencil_configuration.test_mask;

    // Main rasterization loop
    for (int by = by0; by < by1; by += 2) {
        float const f_by = static_cast<float>(by);
        for (int bx = bx0; bx < bx1; bx += 2) {
            PixelQuad quad;
            float const f_bx = static_cast<float>(bx);

            // Calculate edge values using precomputed edge function coefficients
            Vector2<f32x4> const pixel_center = {
                {f_bx + 0.5f, f_bx + 1.5f, f_bx + 0.5f, f_bx + 1.5f},
                {f_by + 0.5f, f_by + 0.5f, f_by + 1.5f, f_by + 1.5f}
            };

            // Calculate edge values using precomputed coefficients
            Vector3<f32x4> edge_values = {
                (pixel_center.x() - v1.x()) * edge0_dx + (pixel_center.y() - v1.y()) * edge0_dy,
                (pixel_center.x() - v2.x()) * edge1_dx + (pixel_center.y() - v2.y()) * edge1_dy,
                (pixel_center.x() - v0.x()) * edge2_dx + (pixel_center.y() - v0.y()) * edge2_dy
            };

            // Generate coverage mask
            quad.mask = (edge_values.x() >= zero.x()) & 
                        (edge_values.y() >= zero.y()) & 
                        (edge_values.z() >= zero.z()) &
                        (pixel_center.x() >= render_bounds.left()) &
                        (pixel_center.x() <= render_bounds.right()) &
                        (pixel_center.y() >= render_bounds.top()) &
                        (pixel_center.y() <= render_bounds.bottom());

            if (none(quad.mask))
                continue;

            // Rest of the function remains the same...
            // [Previous implementation continues...]
        }
    }
}