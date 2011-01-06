
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

#ifndef APPLESEED_STUDIO_MAINWINDOW_PROJECT_MULTIMODELENTITYEDITORFORMFACTORY_H
#define APPLESEED_STUDIO_MAINWINDOW_PROJECT_MULTIMODELENTITYEDITORFORMFACTORY_H

// appleseed.studio headers.
#include "mainwindow/project/entityeditorformfactorybase.h"

// appleseed.foundation headers.
#include "foundation/utility/containers/dictionary.h"

// Standard headers.
#include <cstddef>
#include <string>

namespace appleseed {
namespace studio {

template <typename FactoryRegistrar>
class MultiModelEntityEditorFormFactory
  : public EntityEditorFormFactoryBase
{
  public:
    MultiModelEntityEditorFormFactory(
        const FactoryRegistrar&         factory_registrar,
        const std::string&              entity_name);

    virtual void update(
        const foundation::Dictionary&   values,
        WidgetDefinitionCollection&     definitions) const;

  private:
    const FactoryRegistrar&     m_factory_registrar;

    std::string add_model_widget_definition(
        const foundation::Dictionary&   values,
        WidgetDefinitionCollection&     definitions) const;
};


//
// MultiModelEntityEditorFormFactory class implementation.
//

template <typename FactoryRegistrar>
MultiModelEntityEditorFormFactory<FactoryRegistrar>::MultiModelEntityEditorFormFactory(
    const FactoryRegistrar&             factory_registrar,
    const std::string&                  entity_name)
  : EntityEditorFormFactoryBase(entity_name)
  , m_factory_registrar(factory_registrar)
{
}

template <typename FactoryRegistrar>
void MultiModelEntityEditorFormFactory<FactoryRegistrar>::update(
    const foundation::Dictionary&       values,
    WidgetDefinitionCollection&         definitions) const
{
    definitions.clear();

    add_name_widget_definition(values, definitions);

    const std::string model =
        add_model_widget_definition(values, definitions);

    if (!model.empty())
    {
        const typename FactoryRegistrar::FactoryType* factory =
            m_factory_registrar.lookup(model.c_str());

        add_widget_definitions(
            factory->get_widget_definitions(),
            values,
            definitions);
    }
}

template <typename FactoryRegistrar>
std::string MultiModelEntityEditorFormFactory<FactoryRegistrar>::add_model_widget_definition(
    const foundation::Dictionary&       values,
    WidgetDefinitionCollection&         definitions) const
{
    const typename FactoryRegistrar::FactoryArrayType factories =
        m_factory_registrar.get_factories();

    foundation::Dictionary model_items;
    for (size_t i = 0; i < factories.size(); ++i)
    {
        model_items.insert(
            factories[i]->get_human_readable_model(),
            factories[i]->get_model());
    }

    const std::string model =
        get_value(
            values,
            "model",
            factories.empty() ? "" : factories[0]->get_model());

    definitions.push_back(
        foundation::Dictionary()
            .insert("name", "model")
            .insert("label", "Model")
            .insert("widget", "dropdown_list")
            .insert("dropdown_items", model_items)
            .insert("use", "required")
            .insert("default", model)
            .insert("on_change", "rebuild_form"));

    return model;
}

}       // namespace studio
}       // namespace appleseed

#endif  // !APPLESEED_STUDIO_MAINWINDOW_PROJECT_MULTIMODELENTITYEDITORFORMFACTORY_H