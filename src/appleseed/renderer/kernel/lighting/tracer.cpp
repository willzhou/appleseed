
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2010-2012 Francois Beaune, Jupiter Jazz Limited
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

// Interface header.
#include "tracer.h"

// appleseed.renderer headers.
#include "renderer/kernel/intersection/intersector.h"
#include "renderer/kernel/shading/shadingray.h"
#include "renderer/modeling/material/material.h"
#include "renderer/modeling/surfaceshader/surfaceshader.h"

// Standard headers.
#include <cstddef>

using namespace foundation;
using namespace std;

namespace renderer
{

const ShadingPoint& Tracer::trace(
    SamplingContext&        sampling_context,
    const Vector3d&         origin,
    const Vector3d&         direction,
    const double            time,
    double&                 transmission,
    const ShadingPoint*     parent_shading_point)
{
    transmission = 1.0;

    const ShadingPoint* shading_point_ptr = parent_shading_point;
    size_t shading_point_index = 0;
    Vector3d point = origin;
    size_t iterations = 0;

    while (true)
    {
        // Put a hard limit on the number of iterations.
        if (++iterations >= m_max_iterations)
        {
            RENDERER_LOG_WARNING(
                "reached hard iteration limit (%s), breaking trace loop.",
                pretty_int(m_max_iterations).c_str());
            break;
        }

        // Construct the visibility ray.
        const ShadingRay ray(
            point,
            direction,
            time,
            ~0);            // ray flags

        // Trace the ray.
        m_shading_points[shading_point_index].clear();
        m_intersector.trace(
            ray,
            m_shading_points[shading_point_index],
            shading_point_ptr);

        // Update the pointers to the shading points.
        shading_point_ptr = &m_shading_points[shading_point_index];
        shading_point_index = 1 - shading_point_index;

        // Stop if the ray escaped the scene.
        if (!shading_point_ptr->hit())
            break;

        // Retrieve the material at the shading point.
        const Material* material = shading_point_ptr->get_material();
        if (material == 0)
            break;

        // Retrieve the surface shader.
        const SurfaceShader* surface_shader = material->get_surface_shader();
        if (surface_shader == 0)
            break;

        // Evaluate the alpha mask at the shading point.
        Alpha alpha_mask;
        surface_shader->evaluate_alpha_mask(
            sampling_context,
            m_texture_cache,
            *shading_point_ptr,
            alpha_mask);

        // Stop as soon as we reach a fully opaque occluder.
        if (alpha_mask[0] == 1.0f)
            break;

        // Update the transmission factor.
        transmission *= 1.0 - static_cast<double>(alpha_mask[0]);

        // Move past this partial occluder.
        point = shading_point_ptr->get_point();
    }

    return *shading_point_ptr;
}

const ShadingPoint& Tracer::trace_between(
    SamplingContext&        sampling_context,
    const Vector3d&         origin,
    const Vector3d&         target,
    const double            time,
    double&                 transmission,
    const ShadingPoint*     parent_shading_point)
{
    // todo: get rid of this epsilon.
    const double Eps = 1.0e-6;
    const double SafeMaxDistance = 1.0 - Eps;

    transmission = 1.0;

    const ShadingPoint* shading_point_ptr = parent_shading_point;
    size_t shading_point_index = 0;
    Vector3d point = origin;
    size_t iterations = 0;

    while (true)
    {
        // Put a hard limit on the number of iterations.
        if (++iterations >= m_max_iterations)
        {
            RENDERER_LOG_WARNING(
                "reached hard iteration limit (%s), breaking trace_between loop.",
                pretty_int(m_max_iterations).c_str());
            break;
        }

        // Construct the visibility ray.
        const ShadingRay ray(
            point,
            target - point,
            0.0,                    // ray tmin
            SafeMaxDistance,        // ray tmax
            time,
            ~0);                    // ray flags

        // Trace the ray.
        m_shading_points[shading_point_index].clear();
        m_intersector.trace(
            ray,
            m_shading_points[shading_point_index],
            shading_point_ptr);

        // Update the pointers to the shading points.
        shading_point_ptr = &m_shading_points[shading_point_index];
        shading_point_index = 1 - shading_point_index;

        // Stop if the target point was reached.
        if (!shading_point_ptr->hit())
            break;

        // Retrieve the material at the shading point.
        const Material* material = shading_point_ptr->get_material();
        if (material == 0)
            break;

        // Retrieve the surface shader.
        const SurfaceShader* surface_shader = material->get_surface_shader();
        if (surface_shader == 0)
            break;

        // Evaluate the alpha mask at the shading point.
        Alpha alpha_mask;
        surface_shader->evaluate_alpha_mask(
            sampling_context,
            m_texture_cache,
            *shading_point_ptr,
            alpha_mask);

        // Stop as soon as we reach a fully opaque occluder.
        if (alpha_mask[0] == 1.0f)
            break;

        // Update the transmission factor.
        transmission *= 1.0 - static_cast<double>(alpha_mask[0]);

        // Move past this partial occluder.
        point = shading_point_ptr->get_point();
    }

    return *shading_point_ptr;
}

}   // namespace renderer
