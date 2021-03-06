
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
#include "generictilerenderer.h"

// appleseed.renderer headers.
#include "renderer/kernel/rendering/generic/pixelsampler.h"
#include "renderer/kernel/rendering/isamplerenderer.h"
#include "renderer/kernel/shading/shadingresult.h"
#include "renderer/modeling/aov/aovcollection.h"
#include "renderer/modeling/aov/aovimagecollection.h"
#include "renderer/modeling/frame/frame.h"

// appleseed.foundation headers.
#include "foundation/image/canvasproperties.h"
#include "foundation/image/image.h"
#include "foundation/image/tile.h"
#include "foundation/math/minmax.h"
#include "foundation/math/ordering.h"
#include "foundation/math/qmc.h"
#include "foundation/math/scalar.h"
#include "foundation/platform/breakpoint.h"
#include "foundation/utility/job.h"

// Standard headers.
#include <vector>

using namespace foundation;
using namespace std;

namespace renderer
{

namespace
{
    //
    // Generic tile renderer.
    //
    // Reference for composition algebra with alpha channels:
    //
    //   http://keithp.com/~keithp/porterduff/p253-porter.pdf
    //

#ifndef NDEBUG

    // Define this symbol to break execution into the debugger
    // when a specific pixel is about to be rendered.
    // #define DEBUG_BREAK_AT_PIXEL Vector<size_t, 2>(0, 0)

#endif

    class GenericTileRenderer
      : public ITileRenderer
    {
      public:
        GenericTileRenderer(
            const Frame&                frame,
            ISampleRendererFactory*     factory,
            const ParamArray&           params)
          : m_params(params)
          , m_sample_renderer(factory->create())
          , m_frame_properties(frame.image().properties())
          , m_lighting_conditions(frame.get_lighting_conditions())
          , m_aov_images(frame.aov_images())
        {
            // Retrieve frame properties.
            const CanvasProperties& properties = frame.image().properties();
            const size_t num_pixels = properties.m_tile_width * properties.m_tile_height;

            // Generate pixel ordering.
            vector<size_t> ordering;
            ordering.reserve(num_pixels);
            hilbert_ordering(
                ordering,
                properties.m_tile_width,
                properties.m_tile_height);
            assert(ordering.size() == num_pixels);

            // Convert pixel ordering to (x, y) representation.
            m_pixel_ordering.resize(num_pixels);
            for (size_t i = 0; i < num_pixels; ++i)
            {
                const size_t x = ordering[i] % properties.m_tile_width;
                const size_t y = ordering[i] / properties.m_tile_width;
                assert(x < properties.m_tile_width);
                assert(y < properties.m_tile_height);
                m_pixel_ordering[i].x = static_cast<uint16>(x);
                m_pixel_ordering[i].y = static_cast<uint16>(y);
            }

            // Compute the approximate size of one side of the subpixel grid inside a pixel.
            m_sqrt_max_samples =
                round<size_t>(sqrt(static_cast<double>(m_params.m_max_samples)));
            RENDERER_LOG_INFO(
                "effective subpixel grid size: " FMT_SIZE_T "x" FMT_SIZE_T,
                m_sqrt_max_samples,
                m_sqrt_max_samples);

            // Initialize the pixel sampler.
            m_pixel_sampler.initialize(m_sqrt_max_samples);

            // Precompute some stuff.
            m_aov_image_count = m_aov_images.size();
            m_rcp_sample_canvas_width = 1.0 / (properties.m_canvas_width * m_sqrt_max_samples);
            m_rcp_sample_canvas_height = 1.0 / (properties.m_canvas_height * m_sqrt_max_samples);
            m_rcp_sample_count = 1.0f / (m_sqrt_max_samples * m_sqrt_max_samples);
        }

        virtual void release()
        {
            delete this;
        }

        virtual void render_tile(
            const Frame&                frame,
            const size_t                tile_x,
            const size_t                tile_y,
            AbortSwitch&                abort_switch)
        {
            assert(tile_x < m_frame_properties.m_tile_count_x);
            assert(tile_y < m_frame_properties.m_tile_count_y);

            // Access the tile.
            Tile& tile = frame.image().tile(tile_x, tile_y);
            const size_t tile_width = tile.get_width();
            const size_t tile_height = tile.get_height();
            const size_t tile_origin_x = m_frame_properties.m_tile_width * tile_x;
            const size_t tile_origin_y = m_frame_properties.m_tile_height * tile_y;

            // Loop over tile pixels.
            const size_t num_pixels = m_pixel_ordering.size();
            for (size_t i = 0; i < num_pixels; ++i)
            {
                // Retrieve the coordinates of the pixel in the tile.
                const size_t tx = static_cast<size_t>(m_pixel_ordering[i].x);
                const size_t ty = static_cast<size_t>(m_pixel_ordering[i].y);

                // Skip pixels outside of the tile.
                if (tx >= tile_width || ty >= tile_height)
                    continue;

                // Initialize the pixel values.
                Color4f pixel_color(0.0f);
                AOVCollection pixel_aovs(m_aov_image_count);
                pixel_aovs.set(0.0f);

                // Compute the coordinates of the pixel in the image.
                const size_t ix = tile_origin_x + tx;
                const size_t iy = tile_origin_y + ty;

#ifdef DEBUG_BREAK_AT_PIXEL

                // Break in the debugger when this pixel is reached.
                if (Vector<size_t, 2>(ix, iy) == DEBUG_BREAK_AT_PIXEL)
                    BREAKPOINT();

#endif

                if (!abort_switch.is_aborted())
                {
                    // If cropping is enabled, skip pixels outside the crop window.
                    if (!m_params.m_crop || is_pixel_inside_crop_window(ix, iy))
                    {
                        // Render, filter and accumulate samples.
                        render_pixel(frame, ix, iy, pixel_color, pixel_aovs);
                    }
                }

                // Store the pixel values.
                tile.set_pixel(tx, ty, pixel_color);
                m_aov_images.set_pixel(ix, iy, pixel_aovs);
            }
        }

      private:
        struct Parameters
        {
            const size_t        m_min_samples;          // minimum number of samples per pixel
            const size_t        m_max_samples;          // maximum number of samples per pixel
            bool                m_crop;                 // is cropping enabled?
            Vector4i            m_crop_window;          // crop window

            // Constructor, extract parameters.
            explicit Parameters(const ParamArray& params)
              : m_min_samples   ( params.get_required<size_t>("min_samples", 1) )
              , m_max_samples   ( params.get_required<size_t>("max_samples", 1) )
            {
                // Retrieve crop window parameter.
                m_crop = params.strings().exist("crop_window");
                if (m_crop)
                {
                    m_crop_window =
                        params.get_required<Vector4i>(
                            "crop_window",
                            Vector4i(0, 0, 65535, 65535));
                }
            }
        };

        // Pixel coordinates in a tile; max tile size is 65536 x 65536 pixels.
        typedef Vector<uint16, 2> Pixel;

        const Parameters                    m_params;
        auto_release_ptr<ISampleRenderer>   m_sample_renderer;

        const CanvasProperties&             m_frame_properties;
        const LightingConditions&           m_lighting_conditions;
        const AOVImageCollection&           m_aov_images;
        size_t                              m_aov_image_count;

        vector<Pixel>                       m_pixel_ordering;
        PixelSampler                        m_pixel_sampler;

        size_t                              m_sqrt_max_samples;
        double                              m_rcp_sample_canvas_width;
        double                              m_rcp_sample_canvas_height;
        float                               m_rcp_sample_count;

        SamplingContext::RNGType            m_rng;

        bool is_pixel_inside_crop_window(
            const size_t                ix,
            const size_t                iy) const
        {
            return
                static_cast<int>(ix) >= m_params.m_crop_window[0] &&
                static_cast<int>(iy) >= m_params.m_crop_window[1] &&
                static_cast<int>(ix) <= m_params.m_crop_window[2] &&
                static_cast<int>(iy) <= m_params.m_crop_window[3];
        }

        void render_pixel(
            const Frame&                frame,
            const size_t                ix,
            const size_t                iy,
            Color4f&                    pixel_color,
            AOVCollection&              pixel_aovs)
        {
            const size_t base_sx = ix * m_sqrt_max_samples;
            const size_t base_sy = iy * m_sqrt_max_samples;

            for (size_t sy = 0; sy < m_sqrt_max_samples; ++sy)
            {
                for (size_t sx = 0; sx < m_sqrt_max_samples; ++sx)
                {
                    // Compute the sample position in sample space and the instance number.
                    Vector2d s;
                    size_t instance;
                    m_pixel_sampler.sample(
                        base_sx + sx,
                        base_sy + sy,
                        s,
                        instance);

                    // Compute the sample position in NDC.
                    const Vector2d sample_position =
                        frame.get_sample_position(s.x, s.y);

                    // Create a sampling context. We start with an initial dimension of 1,
                    // as this seems to give less correlation artifacts than when the
                    // initial dimension is set to 0 or 2.
                    SamplingContext sampling_context(
                        m_rng,
                        1,              // number of dimensions
                        instance,       // number of samples
                        instance);      // initial instance number

                    // Render the sample.
                    ShadingResult shading_result;
                    shading_result.m_aovs.set_size(pixel_aovs.size());
                    m_sample_renderer->render_sample(
                        sampling_context,
                        sample_position,
                        shading_result);

                    // todo: implement proper sample filtering.
                    // todo: detect invalid sample values (NaN, infinity, etc.), set
                    // them to black and mark them as faulty in the diagnostic map.

                    // Accumulate the sample.
                    assert(shading_result.m_color_space == ColorSpaceLinearRGB);
                    pixel_color[0] += shading_result.m_color[0];
                    pixel_color[1] += shading_result.m_color[1];
                    pixel_color[2] += shading_result.m_color[2];
                    pixel_color[3] += shading_result.m_alpha[0];
                    pixel_aovs += shading_result.m_aovs;
                }
            }

            // Finish computing the pixel values.
            pixel_color *= m_rcp_sample_count;
            pixel_aovs *= m_rcp_sample_count;
        }
    };
}


//
// GenericTileRendererFactory class implementation.
//

GenericTileRendererFactory::GenericTileRendererFactory(
    const Frame&                frame,
    ISampleRendererFactory*     factory,
    const ParamArray&           params)
  : m_frame(frame)
  , m_factory(factory)
  , m_params(params)
{
}

void GenericTileRendererFactory::release()
{
    delete this;
}

ITileRenderer* GenericTileRendererFactory::create()
{
    return
        new GenericTileRenderer(
            m_frame,
            m_factory,
            m_params);
}

ITileRenderer* GenericTileRendererFactory::create(
    const Frame&                frame,
    ISampleRendererFactory*     factory,
    const ParamArray&           params)
{
    return
        new GenericTileRenderer(
            frame,
            factory,
            params);
}

}   // namespace renderer
