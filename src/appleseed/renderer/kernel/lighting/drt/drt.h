
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

#ifndef APPLESEED_RENDERER_KERNEL_LIGHTING_DRT_DRT_H
#define APPLESEED_RENDERER_KERNEL_LIGHTING_DRT_DRT_H

// appleseed.renderer headers.
#include "renderer/global/global.h"
#include "renderer/kernel/lighting/ilightingengine.h"

// Forward declarations.
namespace renderer  { class LightSampler; }

namespace renderer
{

//
// Distribution Ray Tracing (DRT) lighting engine factory.
//

class RENDERERDLL DRTLightingEngineFactory
  : public ILightingEngineFactory
{
  public:
    // Constructor.
    DRTLightingEngineFactory(
        const LightSampler& light_sampler,
        const ParamArray&   params);

    // Delete this instance.
    virtual void release();

    // Return a new DRT lighting engine instance.
    virtual ILightingEngine* create();

    // Return a new DRT lighting engine instance.
    static ILightingEngine* create(
        const LightSampler& light_sampler,
        const ParamArray&   params);

  private:
    const LightSampler&     m_light_sampler;
    ParamArray              m_params;
};

}       // namespace renderer

#endif  // !APPLESEED_RENDERER_KERNEL_LIGHTING_DRT_DRT_H
