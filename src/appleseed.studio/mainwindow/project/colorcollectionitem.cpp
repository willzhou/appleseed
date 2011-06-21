
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2010-2011 Francois Beaune
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
#include "colorcollectionitem.h"

// appleseed.studio headers.
#include "mainwindow/project/coloritem.h"

// appleseed.foundation headers.
#include "foundation/utility/uid.h"

using namespace foundation;
using namespace renderer;

namespace appleseed {
namespace studio {

namespace
{
    const UniqueID g_class_uid = new_guid();
}

SceneColorCollectionItem::SceneColorCollectionItem(
    Scene&              scene,
    ColorContainer&     colors,
    ProjectBuilder&     project_builder)
  : CollectionItem(scene, project_builder)
  , ItemBase(g_class_uid, "Colors")
  , m_scene(scene)
  , m_project_builder(project_builder)
{
    add_items(m_scene.colors());
}

void SceneColorCollectionItem::add_item(ColorEntity* color)
{
    addChild(new SceneColorItem(color, m_scene, m_project_builder));
}

AssemblyColorCollectionItem::AssemblyColorCollectionItem(
    Assembly&           assembly,
    ColorContainer&     colors,
    ProjectBuilder&     project_builder)
  : CollectionItem(assembly, project_builder)
  , ItemBase(g_class_uid, "Colors")
  , m_assembly(assembly)
  , m_project_builder(project_builder)
{
    add_items(m_assembly.colors());
}

void AssemblyColorCollectionItem::add_item(ColorEntity* color)
{
    addChild(new AssemblyColorItem(color, m_assembly, m_project_builder));
}

}   // namespace studio
}   // namespace appleseed
