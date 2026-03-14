void Device::rasterize_triangle(const Triangle& triangle)
{
    INCREASE_STATISTICS_COUNTER(g_num_rasterized_triangles, 1);

    if (m_options.enable_alpha_test && m_options.alpha_test_func == AlphaTestFunction::Never)
        return;

    // Vertices and edge calculations (unchanged)
    Vertex const vertex0 = triangle.vertices[0];
    Vertex const vertex1 = triangle.vertices[1];
    Vertex const vertex2 = triangle.vertices[2];

    FloatVector2 const v0 = vertex0.window_coordinates.xy();
    FloatVector2 const v1 = vertex1.window_coordinates.xy();
    FloatVector2 const v2 = vertex2.window_coordinates.xy();

    auto const area = edge_function(v0, v1, v2);
    auto const one_over_area = 1.0f / area;

    auto render_bounds = m_frame_buffer->rect();
    if (m_options.scissor_enabled)
        render_bounds.intersect(m_options.scissor_box);

    // Precompute edge function and test functions (unchanged)
    auto calculate_edge_values4 = [v0, v1, v2](Vector2<f32x4> const& p) -> Vector3<f32x4> {
        return {
            edge_function4(v1, v2, p),
            edge_function4(v2, v0, p),
            edge_function4(v0, v1, p),
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

    // Calculate bounds (unchanged)
    int const bx0 =  max(render_bounds.left(),   min(min(v0.x(), v1.x()), v2.x())) & ~1;
    int const bx1 = (min(render_bounds.right(),  max(max(v0.x(), v1.x()), v2.x())) & ~1) + 2;
    int const by0 =  max(render_bounds.top(),    min(min(v0.y(), v1.y()), v2.y())) & ~1;
    int const by1 = (min(render_bounds.bottom(), max(max(v0.y(), v1.y()), v2.y())) & ~1) + 2;

    // Fog calculations (unchanged)
    f32x4 vertex0_fog_depth;
    f32x4 vertex1_fog_depth;
    f32x4 vertex2_fog_depth;
    if (m_options.fog_enabled) {
        vertex0_fog_depth = expand4(fabsf(vertex0.eye_coordinates.z()));
        vertex1_fog_depth = expand4(fabsf(vertex1.eye_coordinates.z()));
        vertex2_fog_depth = expand4(fabsf(vertex2.eye_coordinates.z()));
    }

    // Get buffer pointers once outside the loops
    auto color_buffer = m_frame_buffer->color_buffer();
    auto depth_buffer = m_frame_buffer->depth_buffer();
    auto stencil_buffer = m_frame_buffer->stencil_buffer();
    
    // Precompute buffer row strides
    const int color_stride = color_buffer->pitch() / sizeof(ColorType);
    const int depth_stride = depth_buffer->pitch() / sizeof(DepthType);
    const int stencil_stride = stencil_buffer->pitch() / sizeof(StencilType);

    // Stencil config (unchanged)
    auto const stencil_configuration = m_stencil_configuration[Face::Front];
    auto const stencil_reference_value = stencil_configuration.reference_value & stencil_configuration.test_mask;

    // Optimized write_to_stencil using precomputed pointers
    auto write_to_stencil = [](StencilType* base_ptr, int stride, int x, int y, i32x4 stencil_value, StencilOperation op, StencilType reference_value, StencilType write_mask, i32x4 pixel_mask) {
        if (write_mask == 0 || op == StencilOperation::Keep)
            return;

        // Calculate offsets once
        StencilType* ptrs[4] = {
            pixel_mask[0] ? &base_ptr[(y) * stride + x] : nullptr,
            pixel_mask[1] ? &base_ptr[(y) * stride + x + 1] : nullptr,
            pixel_mask[2] ? &base_ptr[(y + 1) * stride + x] : nullptr,
            pixel_mask[3] ? &base_ptr[(y + 1) * stride + x + 1] : nullptr
        };

        switch (op) {
        case StencilOperation::Decrement:
            stencil_value = (stencil_value & ~write_mask) | (max(stencil_value - 1, expand4(0)) & write_mask);
            break;
        // ... other cases unchanged ...
        }

        INCREASE_STATISTICS_COUNTER(g_num_stencil_writes, maskcount(pixel_mask));
        store4_masked(stencil_value, ptrs[0], ptrs[1], ptrs[2], ptrs[3], pixel_mask);
    };

    // Main loop - optimized buffer access
    for (int by = by0; by < by1; by += 2) {
        auto const f_by = static_cast<float>(by);
        
        // Precompute buffer row pointers
        ColorType* color_row = color_buffer->scanline(by);
        ColorType* color_row_next = color_buffer->scanline(by + 1);
        DepthType* depth_row = depth_buffer->scanline(by);
        DepthType* depth_row_next = depth_buffer->scanline(by + 1);
        StencilType* stencil_row = stencil_buffer->scanline(by);
        StencilType* stencil_row_next = stencil_buffer->scanline(by + 1);

        for (int bx = bx0; bx < bx1; bx += 2) {
            PixelQuad quad;
            auto const f_bx = static_cast<float>(bx);
            quad.screen_coordinates = {
                f32x4 { f_bx, f_bx + 1, f_bx, f_bx + 1 },
                f32x4 { f_by, f_by, f_by + 1, f_by + 1 },
            };

            // Edge tests and mask generation (unchanged)
            auto edge_values = calculate_edge_values4(quad.screen_coordinates + Vector2<f32x4>{expand4(.5f), expand4(.5f)});
            quad.mask = test_point4(edge_values);
            quad.mask &= quad.screen_coordinates.x() >= render_bounds.left()
                && quad.screen_coordinates.x() <= render_bounds.right()
                && quad.screen_coordinates.y() >= render_bounds.top()
                && quad.screen_coordinates.y() <= render_bounds.bottom();

            if (none(quad.mask))
                continue;

            INCREASE_STATISTICS_COUNTER(g_num_quads, 1);
            INCREASE_STATISTICS_COUNTER(g_num_pixels, maskcount(quad.mask));

            int coverage_bits = maskbits(quad.mask);

            // Stencil testing with optimized pointer access
            i32x4 stencil_value;
            if (m_options.enable_stencil_test) {
                StencilType* stencil_ptrs[4] = {
                    coverage_bits & 1 ? &stencil_row[bx] : nullptr,
                    coverage_bits & 2 ? &stencil_row[bx + 1] : nullptr,
                    coverage_bits & 4 ? &stencil_row_next[bx] : nullptr,
                    coverage_bits & 8 ? &stencil_row_next[bx + 1] : nullptr
                };

                stencil_value = load4_masked(stencil_ptrs[0], stencil_ptrs[1], stencil_ptrs[2], stencil_ptrs[3], quad.mask);
                stencil_value &= stencil_configuration.test_mask;

                // ... rest of stencil test unchanged ...
            }

            // Depth testing with optimized pointer access
            DepthType* depth_ptrs[4] = {
                coverage_bits & 1 ? &depth_row[bx] : nullptr,
                coverage_bits & 2 ? &depth_row[bx + 1] : nullptr,
                coverage_bits & 4 ? &depth_row_next[bx] : nullptr,
                coverage_bits & 8 ? &depth_row_next[bx + 1] : nullptr
            };

            if (m_options.enable_depth_test) {
                auto depth = load4_masked(depth_ptrs[0], depth_ptrs[1], depth_ptrs[2], depth_ptrs[3], quad.mask);
                // ... rest of depth test unchanged ...
            }

            // Shading and fragment operations (unchanged)
            // ...

            // Color buffer writes with optimized pointer access
            ColorType* color_ptrs[4] = {
                coverage_bits & 1 ? &color_row[bx] : nullptr,
                coverage_bits & 2 ? &color_row[bx + 1] : nullptr,
                coverage_bits & 4 ? &color_row_next[bx] : nullptr,
                coverage_bits & 8 ? &color_row_next[bx + 1] : nullptr
            };

            if ((m_options.color_mask != 0) && m_options.enable_color_write) {
                u32x4 dst_u32;
                if (m_options.enable_blending || m_options.color_mask != 0xffffffff)
                    dst_u32 = load4_masked(color_ptrs[0], color_ptrs[1], color_ptrs[2], color_ptrs[3], quad.mask);

                if (m_options.enable_blending) {
                    // ... blending calculations unchanged ...
                }

                if (m_options.color_mask == 0xffffffff)
                    store4_masked(to_bgra32(quad.out_color), color_ptrs[0], color_ptrs[1], color_ptrs[2], color_ptrs[3], quad.mask);
                else
                    store4_masked((to_bgra32(quad.out_color) & m_options.color_mask) | (dst_u32 & ~m_options.color_mask), color_ptrs[0], color_ptrs[1], color_ptrs[2], color_ptrs[3], quad.mask);
            }
        }
    }
}