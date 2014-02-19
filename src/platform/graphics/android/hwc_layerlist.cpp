/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/graphics/renderable.h"
#include "mir/graphics/buffer.h"
#include "mir/graphics/android/sync_fence.h"
#include "mir/graphics/android/native_buffer.h"
#include "hwc_layerlist.h"

#include <cstring>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace geom=mir::geometry;

namespace
{
std::shared_ptr<hwc_display_contents_1_t> generate_hwc_list(size_t needed_size)
{
    /* hwc layer list uses hwLayers[0] at the end of the struct */
    auto struct_size = sizeof(hwc_display_contents_1_t) + sizeof(hwc_layer_1_t)*(needed_size);
    auto new_hwc_representation = std::shared_ptr<hwc_display_contents_1_t>(
        static_cast<hwc_display_contents_1_t*>( ::operator new(struct_size)));

    new_hwc_representation->numHwLayers = needed_size;
    new_hwc_representation->retireFenceFd = -1;
    new_hwc_representation->flags = HWC_GEOMETRY_CHANGED;

    //aosp exynos hwc in particular, checks that these fields are non-null in hwc1.1, although
    //these fields are deprecated in hwc1.1 and later.
    static int fake_egl_values = 0;
    new_hwc_representation->dpy = &fake_egl_values;
    new_hwc_representation->sur = &fake_egl_values;

    return new_hwc_representation;
}
}

void mga::LayerListBase::update_representation(size_t needed_size)
{
    if (hwc_representation->numHwLayers != needed_size)
    {
        hwc_representation = generate_hwc_list(needed_size);
    }

    if (layers.size() != needed_size)
    {
        std::list<HWCLayer> new_layers;
        for (auto i = 0u; i < needed_size; i++)
        {
            new_layers.emplace_back(mga::HWCLayer(hwc_representation, i));
        }
        layers = std::move(new_layers);
    }
}

std::weak_ptr<hwc_display_contents_1_t> mga::LayerListBase::native_list()
{
    return hwc_representation;
}

mga::NativeFence mga::LayerListBase::retirement_fence()
{
    return hwc_representation->retireFenceFd;
}

mga::LayerListBase::LayerListBase(size_t initial_list_size)
    : hwc_representation{generate_hwc_list(initial_list_size)}
{
    update_representation(initial_list_size);
}

mga::LayerList::LayerList()
    : LayerListBase{1}
{
    layers.back().set_layer_type(mga::LayerType::skip);
}

mga::FBTargetLayerList::FBTargetLayerList()
    : LayerListBase{2},
      needs_gl_draw{true}
{
    layers.front().set_layer_type(mga::LayerType::skip);
    layers.back().set_layer_type(mga::LayerType::framebuffer_target);
}

void mga::FBTargetLayerList::prepare_default_layers( 
        std::function<void(hwc_display_contents_1_t&)> const& prepare_fn)
{
    update_representation(2);

    layers.front().set_layer_type(mga::LayerType::skip);
    layers.back().set_layer_type(mga::LayerType::framebuffer_target);

    skip_layers_present = true;

    prepare_fn(*native_list().lock());
    needs_gl_draw = true;
}

void mga::FBTargetLayerList::prepare_composition_layers(
    std::function<void(hwc_display_contents_1_t&)> const& prepare_fn,
    std::list<std::shared_ptr<graphics::Renderable>> const& list,
    std::function<void(Renderable const&)> const& render_fn)
{
    auto const needed_size = list.size() + 1;
    update_representation(needed_size);

    //pack layer list from renderables
    auto layers_it = layers.begin();
    for(auto const& renderable : list)
    {
        layers_it->set_layer_type(mga::LayerType::gl_rendered);
        layers_it->set_render_parameters(renderable->screen_position(), renderable->alpha_enabled());
        layers_it->set_buffer(renderable->buffer()->native_buffer_handle());
        layers_it++;
    }
    layers_it->set_layer_type(mga::LayerType::framebuffer_target);
    skip_layers_present = false;

    prepare_fn(*native_list().lock());

    //if a layer cannot be drawn, draw with GL here
    layers_it = layers.begin();
    bool gl_render_needed = false;
    for(auto const& renderable : list)
    {
        if ((layers_it++)->needs_gl_render())
        {
            gl_render_needed = true;
            render_fn(*renderable);
        }
    }

    needs_gl_draw = gl_render_needed;
}

bool mga::FBTargetLayerList::needs_swapbuffers() const
{
    return needs_gl_draw;
}

void mga::FBTargetLayerList::set_fb_target(mg::Buffer const& buffer)
{
    geom::Rectangle const disp_frame{{0,0}, {buffer.size()}};
    auto buf = buffer.native_buffer_handle();
    if (skip_layers_present)
    {
        layers.front().set_render_parameters(disp_frame, false);
        layers.front().set_buffer(buf);
    }

    layers.back().set_render_parameters(disp_frame, false);
    layers.back().set_buffer(buf);
}

void mga::FBTargetLayerList::update_fences()
{
    for(auto& layer : layers)
    {
        layer.update_fence_and_release_buffer();
    }
}
