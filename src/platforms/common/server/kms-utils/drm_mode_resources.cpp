/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "drm_mode_resources.h"

#include <boost/throw_exception.hpp>
#include <system_error>

namespace mgk = mir::graphics::kms;

namespace
{

struct CrtcDeleter
{
    void operator()(drmModeCrtc* p) { if (p) drmModeFreeCrtc(p); }
};

struct EncoderDeleter
{
    void operator()(drmModeEncoder* p) { if (p) drmModeFreeEncoder(p); }
};

struct ConnectorDeleter
{
    void operator()(drmModeConnector* p) { if (p) drmModeFreeConnector(p); }
};

struct ResourcesDeleter
{
    void operator()(drmModeRes* p) { if (p) drmModeFreeResources(p); }
};

mgk::DRMModeResUPtr resources_for_drm_node(int drm_fd)
{
    errno = 0;
    mgk::DRMModeResUPtr resources{drmModeGetResources(drm_fd), ResourcesDeleter()};

    if (!resources)
    {
        if (errno == 0)
        {
            // drmModeGetResources either sets errno, or has failed in malloc()
            errno = ENOMEM;
        }
        BOOST_THROW_EXCEPTION((std::system_error{errno, std::system_category(), "Couldn't get DRM resources"}));
    }

    return resources;
}

}

mgk::DRMModeResources::DRMModeResources(int drm_fd)
    : drm_fd{drm_fd},
      resources{resources_for_drm_node(drm_fd)}
{
}

void mgk::DRMModeResources::for_each_connector(std::function<void(DRMModeConnectorUPtr)> const& f) const
{
    for (int i = 0; i < resources->count_connectors; i++)
    {
        f(get_connector(drm_fd, resources->connectors[i]));
    }
}

void mgk::DRMModeResources::for_each_encoder(std::function<void(DRMModeEncoderUPtr)> const& f) const
{
    for (int i = 0; i < resources->count_encoders; i++)
    {
        f(get_encoder(drm_fd, resources->encoders[i]));
    }
}

void mgk::DRMModeResources::for_each_crtc(std::function<void(DRMModeCrtcUPtr)> const& f) const
{
    for (int i = 0; i < resources->count_crtcs; i++)
    {
        f(get_crtc(drm_fd, resources->crtcs[i]));
    }
}

size_t mgk::DRMModeResources::num_connectors() const
{
    return resources->count_connectors;
}

size_t mgk::DRMModeResources::num_encoders() const
{
    return resources->count_encoders;
}

size_t mgk::DRMModeResources::num_crtcs() const
{
    return resources->count_crtcs;
}

mgk::DRMModeConnectorUPtr mgk::DRMModeResources::connector(uint32_t id) const
{
    return get_connector(drm_fd, id);
}

mgk::DRMModeEncoderUPtr mgk::DRMModeResources::encoder(uint32_t id) const
{
    return get_encoder(drm_fd, id);
}

mgk::DRMModeCrtcUPtr mgk::DRMModeResources::crtc(uint32_t id) const
{
    return get_crtc(drm_fd, id);
}

mgk::DRMModeConnectorUPtr mgk::get_connector(int drm_fd, uint32_t id)
{
    errno = 0;
    DRMModeConnectorUPtr connector{drmModeGetConnector(drm_fd, id), ConnectorDeleter()};

    if (!connector)
    {
        if (errno == 0)
        {
            // drmModeGetConnector either sets errno, or has failed in malloc()
            errno = ENOMEM;
        }
        BOOST_THROW_EXCEPTION((
            std::system_error{errno, std::system_category(), "Failed to get DRM connector"}));
    }
    return connector;
}

mgk::DRMModeEncoderUPtr mgk::get_encoder(int drm_fd, uint32_t id)
{
    errno = 0;
    DRMModeEncoderUPtr encoder{drmModeGetEncoder(drm_fd, id), EncoderDeleter()};

    if (!encoder)
    {
        if (errno == 0)
        {
            // drmModeGetEncoder either sets errno, or has failed in malloc()
            errno = ENOMEM;
        }
        BOOST_THROW_EXCEPTION((
            std::system_error{errno, std::system_category(), "Failed to get DRM encoder"}));
    }
    return encoder;
}

mgk::DRMModeCrtcUPtr mgk::get_crtc(int drm_fd, uint32_t id)
{
    errno = 0;
    DRMModeCrtcUPtr crtc{drmModeGetCrtc(drm_fd, id), CrtcDeleter()};

    if (!crtc)
    {
        if (errno == 0)
        {
            // drmModeGetCrtc either sets errno, or has failed in malloc()
            errno = ENOMEM;
        }
        BOOST_THROW_EXCEPTION((
            std::system_error{errno, std::system_category(), "Failed to get DRM crtc"}));
    }
    return crtc;
}

auto mgk::DRMModeResources::connectors() const -> Connectors
{
    return Connectors{drm_fd, resources->connectors, resources->connectors + resources->count_connectors};
}

mgk::DRMModeResources::Connectors::Connectors(int drm_fd, uint32_t* begin, uint32_t* end)
    : drm_fd{drm_fd},
      begin_{begin},
      end_{end}
{
}

auto mgk::DRMModeResources::Connectors::begin() -> iterator
{
    return iterator(drm_fd, begin_);
}

auto mgk::DRMModeResources::Connectors::end() -> iterator
{
    return iterator(drm_fd, end_);
}

mgk::DRMModeResources::Connectors::iterator::iterator(iterator const& from)
    : drm_fd{from.drm_fd},
      id_ptr{from.id_ptr}
{
}

auto mgk::DRMModeResources::Connectors::iterator::operator=(iterator const& rhs) -> iterator&
{
    drm_fd = rhs.drm_fd;
    id_ptr = rhs.id_ptr;
    current.reset();
    return *this;
}

mgk::DRMModeResources::Connectors::iterator::iterator(
    int drm_fd,
    uint32_t* id_ptr)
    : drm_fd{drm_fd},
      id_ptr{id_ptr}
{
}

auto mgk::DRMModeResources::Connectors::iterator::operator++() -> iterator&
{
    ++id_ptr;
    current.reset();
    return *this;
}

auto mgk::DRMModeResources::Connectors::iterator::operator++(int) -> iterator
{
    iterator copy(drm_fd, id_ptr);
    ++id_ptr;
    current.reset();
    return copy;
}

auto mgk::DRMModeResources::Connectors::iterator::operator*() const -> DRMModeConnectorUPtr&
{
    if (!current)
    {
        current = get_connector(drm_fd, *id_ptr);
    }
    return current;
}

auto mgk::DRMModeResources::Connectors::iterator::operator->() const -> DRMModeConnectorUPtr*
{
    if (!current)
    {
        current = get_connector(drm_fd, *id_ptr);
    }
    return &current;
}

bool mgk::DRMModeResources::Connectors::iterator::operator==(iterator const& rhs) const
{
    return rhs.id_ptr == id_ptr;
}

bool mgk::DRMModeResources::Connectors::iterator::operator!=(iterator const& rhs) const
{
    return !(*this == rhs);
}

auto mgk::DRMModeResources::encoders() const -> Encoders
{
    return Encoders{drm_fd, resources->encoders, resources->encoders + resources->count_connectors};
}

mgk::DRMModeResources::Encoders::Encoders(int drm_fd, uint32_t* begin, uint32_t* end)
    : drm_fd{drm_fd},
      begin_{begin},
      end_{end}
{
}

auto mgk::DRMModeResources::Encoders::begin() -> iterator
{
    return iterator(drm_fd, begin_);
}

auto mgk::DRMModeResources::Encoders::end() -> iterator
{
    return iterator(drm_fd, end_);
}

mgk::DRMModeResources::Encoders::iterator::iterator(
    int drm_fd,
    uint32_t* id_ptr)
    : drm_fd{drm_fd},
      id_ptr{id_ptr}
{
}

auto mgk::DRMModeResources::Encoders::iterator::operator++() -> iterator&
{
    ++id_ptr;
    current.reset();
    return *this;
}

auto mgk::DRMModeResources::Encoders::iterator::operator++(int) -> iterator
{
    iterator copy(drm_fd, id_ptr);
    ++id_ptr;
    current.reset();
    return copy;
}

auto mgk::DRMModeResources::Encoders::iterator::operator*() const -> DRMModeEncoderUPtr&
{
    if (!current)
    {
        current = get_encoder(drm_fd, *id_ptr);
    }
    return current;
}

auto mgk::DRMModeResources::Encoders::iterator::operator->() const -> DRMModeEncoderUPtr*
{
    if (!current)
    {
        current = get_encoder(drm_fd, *id_ptr);
    }
    return &current;
}

bool mgk::DRMModeResources::Encoders::iterator::operator==(iterator const& rhs) const
{
    return rhs.id_ptr == id_ptr;
}

bool mgk::DRMModeResources::Encoders::iterator::operator!=(iterator const& rhs) const
{
    return !(*this == rhs);
}

auto mgk::DRMModeResources::crtcs() const -> CRTCs
{
    return CRTCs{drm_fd, resources->crtcs, resources->crtcs + resources->count_crtcs};
}

mgk::DRMModeResources::CRTCs::CRTCs(int drm_fd, uint32_t* begin, uint32_t* end)
    : drm_fd{drm_fd},
      begin_{begin},
      end_{end}
{
}

auto mgk::DRMModeResources::CRTCs::begin() -> iterator
{
    return iterator(drm_fd, begin_);
}

auto mgk::DRMModeResources::CRTCs::end() -> iterator
{
    return iterator(drm_fd, end_);
}

mgk::DRMModeResources::CRTCs::iterator::iterator(
    int drm_fd,
    uint32_t* id_ptr)
    : drm_fd{drm_fd},
      id_ptr{id_ptr}
{
}

auto mgk::DRMModeResources::CRTCs::iterator::operator++() -> iterator&
{
    ++id_ptr;
    current.reset();
    return *this;
}

auto mgk::DRMModeResources::CRTCs::iterator::operator++(int) -> iterator
{
    iterator copy(drm_fd, id_ptr);
    ++id_ptr;
    current.reset();
    return copy;
}

auto mgk::DRMModeResources::CRTCs::iterator::operator*() const -> DRMModeCrtcUPtr&
{
    if (!current)
    {
        current = get_crtc(drm_fd, *id_ptr);
    }
    return current;
}

auto mgk::DRMModeResources::CRTCs::iterator::operator->() const -> DRMModeCrtcUPtr*
{
    if (!current)
    {
        current = get_crtc(drm_fd, *id_ptr);
    }
    return &current;
}

bool mgk::DRMModeResources::CRTCs::iterator::operator==(iterator const& rhs) const
{
    return rhs.id_ptr == id_ptr;
}

bool mgk::DRMModeResources::CRTCs::iterator::operator!=(iterator const& rhs) const
{
    return !(*this == rhs);
}