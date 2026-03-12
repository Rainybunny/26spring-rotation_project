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

    // Precompute edge function denominators
    auto const area = edge_function(v0, v1, v2);
    auto const one_over_area = 1.0f / area;
    float const denom0 = (v1.y() - v2.y()) * one_over_area;
    float const denom1 = (v2.y() - v0.y()) * one_over_area;
    float const denom2 = (v0.y() - v1.y()) * one_over_area;
    float const numer0 = (v2.x() - v1.x()) * one_over_area;
    float const numer1 = (v0.x() - v2.x()) * one_over_area;
    float const numer2 = (v1.x() - v0.x()) * one_over_area;

    auto render_bounds = m_frame_buffer->rect();
    if (m_options.scissor_enabled)
        render_bounds.intersect(m_options.scissor_box);

    // Optimized edge function calculation using precomputed denominators
    auto calculate_edge_values4 = [v0, v1, v2, denom0, denom1, denom2, numer0, numer1, numer2](Vector2<f32x4> const& p) -> Vector3<f32x4> {
        auto const px = p.x();
        auto const py = p.y();
        return {
            (px - v1.x()) * denom0 + (py - v2.y()) * numer0,
            (px - v2.x()) * denom1 + (py - v0.y()) * numer1,
            (px - v0.x()) * denom2 + (py - v1.y()) * numer2
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

    // Calculate block-based bounds
    // clang-format off
    int const bx0 =  max(render_bounds.left(),   min(min(v0.x(), v1.x()), v2.x())) & ~1;
    int const bx1 = (min(render_bounds.right(),  max(max(v0.x(), v1.x()), v2.x())) & ~1) + 2;
    int const by0 =  max(render_bounds.top(),    min(min(v0.y(), v1.y()), v2.y())) & ~1;
    int const by1 = (min(render_bounds.bottom(), max(max(v0.y(), v1.y()), v2.y())) & ~1) + 2;
    // clang-format on

    // Calculate depth of fragment for fog;
    // OpenGL 1.5 spec chapter 3.10: "An implementation may choose to approximate the
    // eye-coordinate distance from the eye to each fragment center by |Ze|."
    f32x4 vertex0_fog_depth;
    f32x4 vertex1_fog_depth;
    f32x4 vertex2_fog_depth;
    if (m_options.fog_enabled) {
        vertex0_fog_depth = expand4(fabsf(vertex0.eye_coordinates.z()));
        vertex1_fog_depth = expand4(fabsf(vertex1.eye_coordinates.z()));
        vertex2_fog_depth = expand4(fabsf(vertex2.eye_coordinates.z()));
    }

    float const render_bounds_left = render_bounds.left();
    float const render_bounds_right = render_bounds.right();
    float const render_bounds_top = render_bounds.top();
    float const render_bounds_bottom = render_bounds.bottom();

    auto const half_pixel_offset = Vector2<f32x4> { expand4(.5f), expand4(.5f) };

    auto color_buffer = m_frame_buffer->color_buffer();
    auto depth_buffer = m_frame_buffer->depth_buffer();
    auto stencil_buffer = m_frame_buffer->stencil_buffer();

    // Stencil configuration and writing
    auto const stencil_configuration = m_stencil_configuration[Face::Front];
    auto const stencil_reference_value = stencil_configuration.reference_value & stencil_configuration.test_mask;

    auto write_to_stencil = [](StencilType* stencil_ptrs[4], i32x4 stencil_value, StencilOperation op, StencilType reference_value, StencilType write_mask, i32x4 pixel_mask) {
        if (write_mask == 0 || op == StencilOperation::Keep)
            return;

        switch (op) {
        case StencilOperation::Decrement:
            stencil_value = (stencil_value & ~write_mask) | (max(stencil_value - 1, expand4(0)) & write_mask);
            break;
        case StencilOperation::DecrementWrap:
            stencil_value = (stencil_value & ~write_mask) | (((stencil_value - 1) & 0xFF) & write_mask);
            break;
        case StencilOperation::Increment:
            stencil_value = (stencil_value & ~write_mask) | (min(stencil_value + 1, expand4(0xFF)) & write_mask);
            break;
        case StencilOperation::IncrementWrap:
            stencil_value = (stencil_value & ~write_mask) | (((stencil_value + 1) & 0xFF) & write_mask);
            break;
        case StencilOperation::Invert:
            stencil_value ^= write_mask;
            break;
        case StencilOperation::Replace:
            stencil_value = (stencil_value & ~write_mask) | (reference_value & write_mask);
            break;
        case StencilOperation::Zero:
            stencil_value &= ~write_mask;
            break;
        default:
            VERIFY_NOT_REACHED();
        }

        INCREASE_STATISTICS_COUNTER(g_num_stencil_writes, maskcount(pixel_mask));
        store4_masked(stencil_value, stencil_ptrs[0], stencil_ptrs[1], stencil_ptrs[2], stencil_ptrs[3], pixel_mask);
    };

    // Iterate over all blocks within the bounds of the triangle
    for (int by = by0; by < by1; by += 2) {
        auto const f_by = static_cast<float>(by);
        for (int bx = bx0; bx < bx1; bx += 2) {
            PixelQuad quad;

            auto const f_bx = static_cast<float>(bx);
            quad.screen_coordinates = {
                f32x4 { f_bx, f_bx + 1, f_bx, f_bx + 1 },
                f32x4 { f_by, f_by, f_by + 1, f_by + 1 },
            };

            auto edge_values = calculate_edge_values4(quad.screen_coordinates + half_pixel_offset);

            // Generate triangle coverage mask
            quad.mask = test_point4(edge_values);

            // Test quad against intersection of render target size and scissor rect
            quad.mask &= quad.screen_coordinates.x() >= render_bounds_left
                && quad.screen_coordinates.x() <= render_bounds_right
                && quad.screen_coordinates.y() >= render_bounds_top
                && quad.screen_coordinates.y() <= render_bounds_bottom;

            if (none(quad.mask))
                continue;

            INCREASE_STATISTICS_COUNTER(g_num_quads, 1);
            INCREASE_STATISTICS_COUNTER(g_num_pixels, maskcount(quad.mask));

            // ... rest of the function remains the same ...
            [Rest of the function implementation remains exactly the same]
        }
    }
}