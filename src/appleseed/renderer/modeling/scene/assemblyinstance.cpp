
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
#include "assemblyinstance.h"

// appleseed.renderer headers.
#include "renderer/modeling/scene/assembly.h"
#include "renderer/modeling/scene/containers.h"
#include "renderer/modeling/scene/objectinstance.h"
#include "renderer/utility/bbox.h"

using namespace foundation;
using namespace std;

namespace renderer
{

//
// AssemblyInstance class implementation.
//

struct AssemblyInstance::Impl
{
    // Order of data members impacts performance, preserve it.
    Transformd  m_transform;
};

namespace
{
    const UniqueID g_class_uid = new_guid();
}

AssemblyInstance::AssemblyInstance(
    const char*         name,
    const ParamArray&   params,
    const Assembly&     assembly,
    const Transformd&   transform)
  : Entity(g_class_uid, params)
  , impl(new Impl())
  , m_assembly(assembly)
  , m_assembly_uid(assembly.get_uid())
{
    set_name(name);

    impl->m_transform = transform;
}

AssemblyInstance::~AssemblyInstance()
{
    delete impl;
}

void AssemblyInstance::release()
{
    delete this;
}

const Transformd& AssemblyInstance::get_transform() const
{
    return impl->m_transform;
}

GAABB3 AssemblyInstance::compute_parent_bbox() const
{
    return
        impl->m_transform.transform_to_parent(
            get_parent_bbox<GAABB3>(
                m_assembly.object_instances().begin(),
                m_assembly.object_instances().end()));
}


//
// AssemblyInstanceFactory class implementation.
//

auto_release_ptr<AssemblyInstance> AssemblyInstanceFactory::create(
    const char*         name,
    const ParamArray&   params,
    const Assembly&     assembly,
    const Transformd&   transform)
{
    return
        auto_release_ptr<AssemblyInstance>(
            new AssemblyInstance(
                name,
                params,
                assembly,
                transform));
}

}   // namespace renderer
