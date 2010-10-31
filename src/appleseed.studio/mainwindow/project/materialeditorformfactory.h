
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2010 Francois Beaune
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

#ifndef APPLESEED_STUDIO_MAINWINDOW_PROJECT_MATERIALEDITORFORMFACTORY_H
#define APPLESEED_STUDIO_MAINWINDOW_PROJECT_MATERIALEDITORFORMFACTORY_H

// appleseed.studio headers.
#include "mainwindow/project/entityeditorwindow.h"

// Forward declarations.
namespace foundation    { class Dictionary; }
namespace renderer      { class Assembly; }

namespace appleseed {
namespace studio {

class MaterialEditorFormFactory
  : public EntityEditorWindow::IFormFactory
{
  public:
    typedef EntityEditorWindow::WidgetDefinitionCollection WidgetDefinitionCollection;

    explicit MaterialEditorFormFactory(const renderer::Assembly& assembly);

    virtual void update(
        const foundation::Dictionary&   values,
        WidgetDefinitionCollection&     definitions) const;

  private:
    const renderer::Assembly& m_assembly;
};

}       // namespace studio
}       // namespace appleseed

#endif  // !APPLESEED_STUDIO_MAINWINDOW_PROJECT_MATERIALEDITORFORMFACTORY_H