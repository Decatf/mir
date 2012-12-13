/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/frontend/application_mediator.h"
#include "mir/frontend/application_listener.h"
#include "mir/frontend/session_store.h"
#include "mir/frontend/session.h"
#include "mir/frontend/surface_organiser.h"
#include "mir/frontend/resource_cache.h"

#include "mir/compositor/buffer_ipc_package.h"
#include "mir/compositor/buffer_id.h"
#include "mir/compositor/buffer.h"
#include "mir/compositor/buffer_bundle.h"
#include "mir/geometry/dimensions.h"
#include "mir/graphics/platform.h"
#include "mir/graphics/display.h"
#include "mir/graphics/platform_ipc_package.h"
#include "mir/surfaces/surface.h"
#include "mir/exception.h"

mir::frontend::ApplicationMediator::ApplicationMediator(
    std::shared_ptr<SessionStore> const& session_store,
    std::shared_ptr<graphics::Platform> const & graphics_platform,
    std::shared_ptr<graphics::Display> const& graphics_display,
    std::shared_ptr<ApplicationListener> const& listener,
    std::shared_ptr<ResourceCache> const& resource_cache) :
    session_store(session_store),
    graphics_platform(graphics_platform),
    graphics_display(graphics_display),
    listener(listener),
    resource_cache(resource_cache)
{
}

void mir::frontend::ApplicationMediator::connect(
    ::google::protobuf::RpcController*,
    const ::mir::protobuf::ConnectParameters* request,
    ::mir::protobuf::Connection* response,
    ::google::protobuf::Closure* done)
{
    listener->application_connect_called(request->application_name());

    application_session = session_store->open_session(request->application_name());

    auto ipc_package = graphics_platform->get_ipc_package();
    auto platform = response->mutable_platform();
    auto display_info = response->mutable_display_info();

    for (auto p = ipc_package->ipc_data.begin(); p != ipc_package->ipc_data.end(); ++p)
        platform->add_data(*p);

    for (auto p = ipc_package->ipc_fds.begin(); p != ipc_package->ipc_fds.end(); ++p)
        platform->add_fd(*p);

    auto view_area = graphics_display->view_area();
    display_info->set_width(view_area.size.width.as_uint32_t());
    display_info->set_height(view_area.size.height.as_uint32_t());

    resource_cache->save_resource(response, ipc_package);
    done->Run();
}

void mir::frontend::ApplicationMediator::create_surface(
    google::protobuf::RpcController* /*controller*/,
    const mir::protobuf::SurfaceParameters* request,
    mir::protobuf::Surface* response,
    google::protobuf::Closure* done)
{
    if (application_session.get() == nullptr)
        BOOST_THROW_EXCEPTION(std::runtime_error("Invalid application session"));

    listener->application_create_surface_called(application_session->get_name());

    auto const id = application_session->create_surface(
        surfaces::SurfaceCreationParameters()
        .of_name(request->surface_name())
        .of_size(request->width(), request->height())
        .of_buffer_usage(static_cast<compositor::BufferUsage>(request->buffer_usage()))
        .of_pixel_format(static_cast<geometry::PixelFormat>(request->pixel_format()))
        );

    {
        auto surface = application_session->get_surface(id);
        response->mutable_id()->set_value(id.as_value());
        response->set_width(surface->size().width.as_uint32_t());
        response->set_height(surface->size().height.as_uint32_t());
        response->set_pixel_format((int)surface->pixel_format());
        response->set_buffer_usage(request->buffer_usage());


        surface->advance_client_buffer();
        auto const& client_resource = surface->get_buffer_ipc_package();
        auto const& id = client_resource->id;
        auto ipc_package = client_resource->buffer.lock()->get_ipc_package();


        auto buffer = response->mutable_buffer();

        buffer->set_buffer_id(id.as_uint32_t());
        for (auto p = ipc_package->ipc_data.begin(); p != ipc_package->ipc_data.end(); ++p)
            buffer->add_data(*p);

        for (auto p = ipc_package->ipc_fds.begin(); p != ipc_package->ipc_fds.end(); ++p)
            buffer->add_fd(*p);

        buffer->set_stride(ipc_package->stride);

        resource_cache->save_resource(response, ipc_package);
    }

    done->Run();
}

void mir::frontend::ApplicationMediator::next_buffer(
    ::google::protobuf::RpcController* /*controller*/,
    ::mir::protobuf::SurfaceId const* request,
    ::mir::protobuf::Buffer* response,
    ::google::protobuf::Closure* done)
{
    if (application_session.get() == nullptr)
        BOOST_THROW_EXCEPTION(std::runtime_error("Invalid application session"));

    listener->application_next_buffer_called(application_session->get_name());

    auto surface = application_session->get_surface(SurfaceId(request->value()));

    surface->advance_client_buffer();
    auto const& client_resource = surface->get_buffer_ipc_package();
    auto const& id = client_resource->id;
    auto ipc_package = client_resource->buffer.lock()->get_ipc_package();

    response->set_buffer_id(id.as_uint32_t());
    for (auto p = ipc_package->ipc_data.begin(); p != ipc_package->ipc_data.end(); ++p)
        response->add_data(*p);

    for (auto p = ipc_package->ipc_fds.begin(); p != ipc_package->ipc_fds.end(); ++p)
        response->add_fd(*p);

    response->set_stride(ipc_package->stride);

    resource_cache->save_resource(response, ipc_package);
    done->Run();
}


void mir::frontend::ApplicationMediator::release_surface(
    google::protobuf::RpcController* /*controller*/,
    const mir::protobuf::SurfaceId* request,
    mir::protobuf::Void*,
    google::protobuf::Closure* done)
{
    if (application_session.get() == nullptr)
        BOOST_THROW_EXCEPTION(std::runtime_error("Invalid application session"));

    listener->application_release_surface_called(application_session->get_name());

    auto const id = SurfaceId(request->value());

    application_session->destroy_surface(id);

    done->Run();
}

void mir::frontend::ApplicationMediator::disconnect(
    google::protobuf::RpcController* /*controller*/,
    const mir::protobuf::Void* /*request*/,
    mir::protobuf::Void* /*response*/,
    google::protobuf::Closure* done)
{
    if (application_session.get() == nullptr)
        BOOST_THROW_EXCEPTION(std::runtime_error("Invalid application session"));

    listener->application_disconnect_called(application_session->get_name());

    session_store->close_session(application_session);
    application_session.reset();

    done->Run();
}

