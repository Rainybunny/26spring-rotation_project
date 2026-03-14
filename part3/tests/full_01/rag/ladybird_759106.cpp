Let me analyze the `rasterize_triangle` function for optimization opportunities:

1. **Bottleneck Analysis**:
   - The function processes triangles in 2x2 pixel blocks (quads)
   - Main operations per quad: edge function calculations, barycentric interpolation, depth/stencil tests, shading, blending
   - The inner loop over pixel blocks is the hottest part
   - Memory access patterns could be improved for better cache utilization

2. **Key Observations**:
   - The function already uses SIMD operations effectively
   - The edge function calculations are vectorized
   - The depth/stencil tests are major contributors to runtime
   - The blending operations have some redundant calculations

3. **Potential Optimizations**:
   - Precompute more values outside the inner loop
   - Optimize the blending calculations which are done per-pixel
   - Improve memory access patterns for color/depth buffers
   - Reduce redundant calculations in the inner loop

4. **Selected Optimization**:
   - The blending calculations can be optimized by precomputing constant terms and reducing redundant operations. The current implementation recalculates many terms per pixel that could be computed once per triangle.

Here's the optimized version focusing on the blending calculations:

```cpp
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

    auto render_bounds = m_frame_buffer->rect();
    if (m_options.scissor_enabled)
        render_bounds.intersect(m_options.scissor_box);

    // This function calculates the 3 edge values for the pixel relative to the triangle.
    auto calculate_edge_values4 = [v0, v1, v2](Vector2<f32x4> const& p) -> Vector3<f32x4> {
        return {
            edge_function4(v1, v2, p),
            edge_function4(v2, v0, p),
            edge_function4(v0, v1, p),
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

    // Precompute blending factors if blending is enabled
    bool const blending_enabled = m_options.enable_blending;
    Vector4<f32x4> const src_factor_constant = expand4(m_alpha_blend_factors.src_constant);
    Vector4<f32x4> const dst_factor_constant = expand4(m_alpha_blend_factors.dst_constant);
    float const src_factor_src_color = m_alpha_blend_factors.src_factor_src_color;
    float const src_factor_src_alpha = m_alpha_blend_factors.src_factor_src_alpha;
    float const src_factor_dst_color = m_alpha_blend_factors.src_factor_dst_color;
    float const src_factor_dst_alpha = m_alpha_blend_factors.src_factor_dst_alpha;
    float const dst_factor_src_color = m_alpha_blend_factors.dst_factor_src_color;
    float const dst_factor_src_alpha = m_alpha_blend_factors.dst_factor_src_alpha;
    float const dst_factor_dst_color = m_alpha_blend_factors.dst_factor_dst_color;
    float const dst_factor_dst_alpha = m_alpha_blend_factors.dst_factor_dst_alpha;

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

            int coverage_bits = maskbits(quad.mask);

            // Stencil testing
            StencilType* stencil_ptrs[4];
            i32x4 stencil_value;
            if (m_options.enable_stencil_test) {
                stencil_ptrs[0] = coverage_bits & 1 ? &stencil_buffer->scanline(by)[bx] : nullptr;
                stencil_ptrs[1] = coverage_bits & 2 ? &stencil_buffer->scanline(by)[bx + 1] : nullptr;
                stencil_ptrs[2] = coverage_bits & 4 ? &stencil_buffer->scanline(by + 1)[bx] : nullptr;
                stencil_ptrs[3] = coverage_bits & 8 ? &stencil_buffer->scanline(by + 1)[bx + 1] : nullptr;

                stencil_value = load4_masked(stencil_ptrs[0], stencil_ptrs[1], stencil_ptrs[2], stencil_ptrs[3], quad.mask);
                stencil_value &= stencil_configuration.test_mask;

                i32x4 stencil_test_passed;
                switch (stencil_configuration.test_function) {
                case StencilTestFunction::Always:
                    stencil_test_passed = expand4(~0);
                    break;
                case StencilTestFunction::Equal:
                    stencil_test_passed = stencil_value == stencil_reference_value;
                    break;
                case StencilTestFunction::Greater:
                    stencil_test_passed = stencil_value > stencil_reference_value;
                    break;
                case StencilTestFunction::GreaterOrEqual:
                    stencil_test_passed = stencil_value >= stencil_reference_value;
                    break;
                case StencilTestFunction::Less:
                    stencil_test_passed = stencil_value < stencil_reference_value;
                    break;
                case StencilTestFunction::LessOrEqual:
                    stencil_test_passed = stencil_value <= stencil_reference_value;
                    break;
                case StencilTestFunction::Never:
                    stencil_test_passed = expand4(0);
                    break;
                case StencilTestFunction::NotEqual:
                    stencil_test_passed = stencil_value != stencil_reference_value;
                    break;
                default:
                    VERIFY_NOT_REACHED();
                }

                // Update stencil buffer for pixels that failed the stencil test
                write_to_stencil(
                    stencil_ptrs,
                    stencil_value,
                    stencil_configuration.on_stencil_test_fail,
                    stencil_reference_value,
                    stencil_configuration.write_mask,
                    quad.mask & ~stencil_test_passed);

                // Update coverage mask + early quad rejection
                quad.mask &= stencil_test_passed;
                if (none(quad.mask))
                    continue;
            }

            // Calculate barycentric coordinates from previously calculated edge values
            quad.barycentrics = edge_values * one_over_area;

            // Depth testing
            DepthType* depth_ptrs[4] = {
                coverage_bits & 1 ? &depth_buffer->scanline(by)[bx] : nullptr,
                coverage_bits & 2 ? &depth_buffer->scanline(by)[bx + 1] : nullptr,
                coverage_bits & 4 ? &depth_buffer->scanline(by + 1)[bx] : nullptr,
                coverage_bits & 8 ? &depth_buffer->scanline(by + 1)[bx + 1] : nullptr,
            };
            if (m_options.enable_depth_test) {
                auto depth = load4_masked(depth_ptrs[0], depth_ptrs[1], depth_ptrs[2], depth_ptrs[3], quad.mask);

                quad.depth = interpolate(vertex0.window_coordinates.z(), vertex1.window_coordinates.z(), vertex2.window_coordinates.z(), quad.barycentrics);
                // FIXME: Also apply depth_offset_factor which depends on the depth gradient
                if (m_options.depth_offset_enabled)
                    quad.depth += m_options.depth_offset_constant * NumericLimits<float>::epsilon();

                i32x4 depth_test_passed;
                switch (m_options.depth_func) {
                case DepthTestFunction::Always:
                    depth_test_passed = expand4(~0);
                    break;
                case DepthTestFunction::Never:
                    depth_test_passed = expand4(0);
                    break;
                case DepthTestFunction::Greater:
                    depth_test_passed = quad.depth > depth;
                    break;
                case DepthTestFunction::GreaterOrEqual:
                    depth_test_passed = quad.depth >= depth;
                    break;
                case DepthTestFunction::NotEqual:
#ifdef __SSE__
                    depth_test_passed = quad.depth != depth;
#else
                    depth_test_passed = i32x4 {
                        bit_cast<u32>(quad.depth[0]) != bit_cast<u32>(depth[0]) ? -1 : 0,
                        bit_cast<u32>(quad.depth[1]) != bit_cast<u32>(depth[1]) ? -1 : 0,
                        bit_cast<u32>(quad.depth[2]) != bit_cast<u32>(depth[2]) ? -1 : 0,
                        bit_cast<u32>(quad.depth[3]) != bit_cast<u32>(depth[3]) ? -1 : 0,
                    };
#endif
                    break;
                case DepthTestFunction::Equal:
#ifdef __SSE__
                    depth_test_passed = quad.depth == depth;
#else
                    depth_test_passed = i32x4 {
                        bit_cast<u32>(quad.depth[0]) == bit_cast<u32>(depth[0]) ? -1 : 0,
                        bit_cast<u32>(quad.depth[1]) == bit_cast<u32>(depth[1]) ? -1 : 0,
                        bit_cast<u32>(quad.depth[2]) == bit_cast<u32>(depth[2]) ? -1 : 0,
                        bit_cast<u32>(quad.depth[3]) == bit_cast<u32>(depth[3]) ? -1 : 0,
                    };
#endif
                    break;
                case DepthTestFunction::LessOrEqual:
                    depth_test_passed = quad.depth <= depth;
                    break;
                case DepthTestFunction::Less:
                    depth_test_passed = quad.depth < depth;
                    break;
                default:
                    VERIFY_NOT_REACHED();
                }

                // Update stencil buffer for pixels that failed the depth test
                if (m_options.enable_stencil_test) {
                    write_to_stencil(
                        stencil_ptrs,
                        stencil_value,
                        stencil_configuration.on_depth_test_fail,
                        stencil_reference_value,
                        stencil_configuration.write_mask,
                        quad.mask & ~depth_test_passed);
                }

                // Update coverage mask + early quad rejection
                quad.mask &= depth_test_passed;
                if (none(quad.mask))
                    continue;
            }

            // Update stencil buffer for passed pixels
            if (m_options.enable_stencil_test) {
                write_to_stencil(
                    stencil_ptrs,
                    stencil_value,
                    stencil_configuration.on_pass,
                    stencil_reference_value,
                    stencil_configuration.write_mask,
                    quad.mask);
            }

            INCREASE_STATISTICS_COUNTER(g_num_pixels_shaded, maskcount(quad.mask));

            // Draw the pixels according to the previously generated mask
            auto const w_coordinates = Vector3<f32x4> {
                expand4(vertex0.window_coordinates.w()),
                expand4(vertex1.window_coordinates.w()),
                expand4(vertex2.window_coordinates.w()),
            };

            auto const interpolated_reciprocal_w = interpolate