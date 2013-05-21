/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "android_input_registrar.h"

#include "android_input_window_handle.h"
#include "android_input_application_handle.h"

#include "mir/input/android/android_input_configuration.h"

#include <InputDispatcher.h>

#include <boost/throw_exception.hpp>

#include <stdexcept>
#include <mutex>

namespace mi = mir::input;
namespace mia = mi::android;

mia::InputRegistrar::InputRegistrar(std::shared_ptr<mia::InputConfiguration> const& config) :
    input_dispatcher(config->the_dispatcher())
{
}

void mia::InputRegistrar::input_surface_opened(std::shared_ptr<input::SurfaceTarget> const& opened_surface)
{
    std::unique_lock<std::mutex> lock(handles_mutex);

    // TODO: We don't have much use for InputApplicationHandle so we simply use one per surface.
    // it is only used in droidinput for logging and determining application not responding (ANR),
    // we determine ANR on a per surface basis. ~racarr
    auto application_handle = new mia::InputApplicationHandle(opened_surface);
    if (window_handles.find(opened_surface) != window_handles.end())
        BOOST_THROW_EXCEPTION(std::logic_error("A surface was opened twice"));

    droidinput::sp<droidinput::InputWindowHandle> window_handle = new mia::InputWindowHandle(application_handle, opened_surface);
    input_dispatcher->registerInputChannel(window_handle->getInfo()->inputChannel, window_handle, false);
    
    window_handles[opened_surface] = window_handle;
}

void mia::InputRegistrar::input_surface_closed(std::shared_ptr<input::SurfaceTarget> const& closed_surface)
{
    std::unique_lock<std::mutex> lock(handles_mutex);
    auto it = window_handles.find(closed_surface);
    if (it == window_handles.end())
        BOOST_THROW_EXCEPTION(std::logic_error("A surface was closed twice"));

    input_dispatcher->unregisterInputChannel(it->second->getInfo()->inputChannel);
    window_handles.erase(it);
}

// TODO: Test behavior ~racarr
droidinput::sp<droidinput::InputWindowHandle> mia::InputRegistrar::handle_for_surface(std::shared_ptr<input::SurfaceTarget const> const& surface)
{
    if (window_handles.find(surface) == window_handles.end())
        BOOST_THROW_EXCEPTION(std::logic_error("Requesting handle for an unregistered surface"));
    return window_handles[surface];
}
