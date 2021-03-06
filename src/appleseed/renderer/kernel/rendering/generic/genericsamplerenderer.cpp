
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
#include "genericsamplerenderer.h"

// appleseed.renderer headers.
#include "renderer/kernel/intersection/intersector.h"
#include "renderer/kernel/intersection/tracecontext.h"
#include "renderer/kernel/lighting/ilightingengine.h"
#include "renderer/kernel/shading/shadingcontext.h"
#include "renderer/kernel/shading/shadingengine.h"
#include "renderer/kernel/shading/shadingpoint.h"
#include "renderer/kernel/shading/shadingray.h"
#include "renderer/kernel/shading/shadingresult.h"
#include "renderer/kernel/texturing/texturecache.h"
#include "renderer/modeling/camera/camera.h"
#include "renderer/modeling/frame/frame.h"
#include "renderer/modeling/scene/scene.h"

using namespace foundation;
using namespace std;

namespace renderer
{

namespace
{
    //
    // Generic sample renderer.
    //

    // If defined, the texture cache returns solid tiles whose color depends on whether
    // the requested tile could be found in the cache or not, and if it was found, at
    // which level it was found.
    #undef DEBUG_DISPLAY_TEXTURE_CACHE_PERFORMANCES

    class GenericSampleRenderer
      : public ISampleRenderer
    {
      public:
        GenericSampleRenderer(
            const Scene&            scene,
            const Frame&            frame,
            const TraceContext&     trace_context,
            TextureStore&           texture_store,
            ILightingEngineFactory* lighting_engine_factory,
            ShadingEngine&          shading_engine,
            const ParamArray&       params)
          : m_params(params)
          , m_scene(scene)
          , m_lighting_conditions(frame.get_lighting_conditions())
          , m_texture_cache(texture_store)
          , m_intersector(trace_context, m_texture_cache, true, m_params.m_report_self_intersections)
          , m_lighting_engine(lighting_engine_factory->create())
          , m_shading_engine(shading_engine)
        {
        }

        ~GenericSampleRenderer()
        {
            m_lighting_engine->release();
        }

        virtual void release()
        {
            delete this;
        }

        virtual void render_sample(
            SamplingContext&        sampling_context,
            const Vector2d&         image_point,        // point in image plane, in NDC
            ShadingResult&          shading_result)
        {
#ifdef DEBUG_DISPLAY_TEXTURE_CACHE_PERFORMANCES

            const uint64 initial_texcache_s0_hit_count = m_texture_cache.get_stage0_hit_count();
            const uint64 initial_texcache_s1_hit_count = m_texture_cache.get_stage1_hit_count();
            const uint64 initial_texcache_s1_miss_count = m_texture_cache.get_stage1_miss_count();

#endif

            // Construct a shading context.
            ShadingContext shading_context(
                m_intersector,
                m_texture_cache,
                m_lighting_engine);

            // Construct a primary ray.
            ShadingRay primary_ray;
            m_scene.get_camera()->generate_ray(
                sampling_context,
                image_point,
                primary_ray);

            // Initialize the result to linear RGB transparent black.
            shading_result.clear();

            ShadingPoint shading_points[2];
            size_t shading_point_index = 0;
            const ShadingPoint* shading_point_ptr = 0;
            size_t iterations = 0;

            while (true)
            {
                // Put a hard limit on the number of iterations.
                const size_t MaxIterations = 10000;
                if (++iterations >= MaxIterations)
                {
                    RENDERER_LOG_WARNING(
                        "reached hard iteration limit (%s), breaking primary ray trace loop.",
                        pretty_int(MaxIterations).c_str());
                    break;
                }

                // Trace the ray.
                shading_points[shading_point_index].clear();
                m_intersector.trace(
                    primary_ray,
                    shading_points[shading_point_index],
                    shading_point_ptr);

                // Update the pointers to the shading points.
                shading_point_ptr = &shading_points[shading_point_index];
                shading_point_index = 1 - shading_point_index;

                // Shade the intersection point.
                ShadingResult local_result;
                local_result.m_aovs.set_size(shading_result.m_aovs.size());
                m_shading_engine.shade(
                    sampling_context,
                    shading_context,
                    *shading_point_ptr,
                    local_result);

                // Transform the result to the linear RGB color space.
                local_result.transform_to_linear_rgb(m_lighting_conditions);

                // "Over" alpha compositing.
                shading_result.composite_over(local_result);

                // Stop once we hit the environment.
                if (!shading_point_ptr->hit())
                    break;

                // Stop once we hit full opacity.
                if (max_value(shading_result.m_alpha) > m_params.m_transparency_threshold)
                    break;

                // Move the ray origin to the intersection point.
                primary_ray.m_org = shading_point_ptr->get_point();
                primary_ray.m_tmax = numeric_limits<double>::max();
            }

#ifdef DEBUG_DISPLAY_TEXTURE_CACHE_PERFORMANCES

            const Vector<uint64, 3> texcache_stats(
                m_texture_cache.get_stage0_hit_count() - initial_texcache_s0_hit_count,
                m_texture_cache.get_stage1_hit_count() - initial_texcache_s1_hit_count,
                m_texture_cache.get_stage1_miss_count() - initial_texcache_s1_miss_count);

            if (texcache_stats == Vector<uint64, 3>(0, 0, 0))
            {
                // In black: no access to the texture cache.
                shading_result.set_to_linear_rgb(Color3f(0.0f, 0.0f, 0.0f));
            }
            else
            {
                switch (max_index(texcache_stats))
                {
                  // In green: a majority of stage-0 cache hits.
                  case 0:
                    shading_result.set_to_linear_rgb(Color3f(0.0f, 1.0f, 0.0f));
                    break;

                  // In blue: a majority of stage-0 cache misses and stage-1 cache hits.
                  case 1:
                    shading_result.set_to_linear_rgb(Color3f(0.0f, 0.0f, 1.0f));
                    break;

                  // In red: a majority of stage-0 and stage-1 cache misses.
                  case 2:
                    shading_result.set_to_linear_rgb(Color3f(1.0f, 0.0f, 0.0f));
                    break;
                }
            }

#endif
        }

      private:
        struct Parameters
        {
            const float     m_transparency_threshold;
            const bool      m_report_self_intersections;

            explicit Parameters(const ParamArray& params)
              : m_transparency_threshold(1.0f - params.get_optional<float>("opacity_threshold", 1.0e-5f))
              , m_report_self_intersections(params.get_optional<bool>("report_self_intersections", false))
            {
            }
        };

        const Parameters            m_params;
        const Scene&                m_scene;
        const LightingConditions&   m_lighting_conditions;

        TextureCache                m_texture_cache;
        Intersector                 m_intersector;
        ILightingEngine*            m_lighting_engine;
        ShadingEngine&              m_shading_engine;
    };
}


//
// GenericSampleRendererFactory class implementation.
//

GenericSampleRendererFactory::GenericSampleRendererFactory(
    const Scene&            scene,
    const Frame&            frame,
    const TraceContext&     trace_context,
    TextureStore&           texture_store,
    ILightingEngineFactory* lighting_engine_factory,
    ShadingEngine&          shading_engine,
    const ParamArray&       params)
  : m_scene(scene)
  , m_frame(frame)
  , m_trace_context(trace_context)
  , m_texture_store(texture_store)
  , m_lighting_engine_factory(lighting_engine_factory)
  , m_shading_engine(shading_engine)
  , m_params(params)
{
}

void GenericSampleRendererFactory::release()
{
    delete this;
}

ISampleRenderer* GenericSampleRendererFactory::create()
{
    return
        new GenericSampleRenderer(
            m_scene,
            m_frame,
            m_trace_context,
            m_texture_store,
            m_lighting_engine_factory,
            m_shading_engine,
            m_params);
}

ISampleRenderer* GenericSampleRendererFactory::create(
    const Scene&            scene,
    const Frame&            frame,
    const TraceContext&     trace_context,
    TextureStore&           texture_store,
    ILightingEngineFactory* lighting_engine_factory,
    ShadingEngine&          shading_engine,
    const ParamArray&       params)
{
    return
        new GenericSampleRenderer(
            scene,
            frame,
            trace_context,
            texture_store,
            lighting_engine_factory,
            shading_engine,
            params);
}

}   // namespace renderer
