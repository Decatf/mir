/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Brandon Schaefer <brandon.schaefer@canonical.com>
 */

#include <boost/throw_exception.hpp>

#include "mir/events/motion_event.h"

MirMotionEvent::MirMotionEvent() :
    MirInputEvent(mir_event_type_motion)
{
}

int32_t MirMotionEvent::device_id() const
{
    return device_id_;
}

void MirMotionEvent::set_device_id(int32_t id)
{
    device_id_ = id;
}

int32_t MirMotionEvent::source_id() const
{
    return source_id_;
}

void MirMotionEvent::set_source_id(int32_t id)
{
    source_id_ = id;
}

MirInputEventModifiers MirMotionEvent::modifiers() const
{
    return modifiers_;
}

void MirMotionEvent::set_modifiers(MirInputEventModifiers modifiers)
{
    modifiers_ = modifiers;
}

MirPointerButtons MirMotionEvent::buttons() const
{
    return buttons_;
}

void MirMotionEvent::set_buttons(MirPointerButtons buttons)
{
    buttons_ = buttons;
}

std::chrono::nanoseconds MirMotionEvent::event_time() const
{
    return event_time_;
}

void MirMotionEvent::set_event_time(std::chrono::nanoseconds const& event_time)
{
    event_time_ = event_time;
}

mir::cookie::Blob MirMotionEvent::cookie() const
{
    return cookie_;
}

void MirMotionEvent::set_cookie(mir::cookie::Blob const& blob)
{
    cookie_ = blob;
}

size_t MirMotionEvent::pointer_count() const
{
    return pointer_count_;
}

void MirMotionEvent::set_pointer_count(size_t count)
{
    pointer_count_ = count;
}

MirMotionPointer MirMotionEvent::pointer_coordinates(size_t index) const
{
    if (index > pointer_count_)
         BOOST_THROW_EXCEPTION(std::out_of_range("Out of bounds index in pointer coordinates"));

    return pointer_coordinates_[index];
}

MirMotionPointer& MirMotionEvent::pointer_coordinates(size_t index)
{
    if (index > pointer_count_)
         BOOST_THROW_EXCEPTION(std::out_of_range("Out of bounds index in pointer coordinates"));

    return pointer_coordinates_[index];
}

MirTouchEvent* MirMotionEvent::to_touch()
{
    return static_cast<MirTouchEvent*>(this);
}

MirTouchEvent const* MirMotionEvent::to_touch() const
{
    return static_cast<MirTouchEvent const*>(this);
}

MirPointerEvent* MirMotionEvent::to_pointer()
{
    return static_cast<MirPointerEvent*>(this);
}

MirPointerEvent const* MirMotionEvent::to_pointer() const
{
    return static_cast<MirPointerEvent const*>(this);
}

int MirMotionPointer::id() const
{
    return id_;
}

void MirMotionPointer::set_id(int id)
{
    id_ = id;
}

float MirMotionPointer::x() const
{
    return x_;
}

void MirMotionPointer::set_x(float x)
{
    x_ = x;
}

float MirMotionPointer::y() const
{
    return y_;
}

void MirMotionPointer::set_y(float y)
{
    y_ = y;
}

float MirMotionPointer::dx() const
{
    return dx_;
}

void MirMotionPointer::set_dx(float dx)
{
    dx_ = dx;
}

float MirMotionPointer::dy() const
{
    return dy_;
}

void MirMotionPointer::set_dy(float dy)
{
    dy_ = dy;
}

float MirMotionPointer::touch_major() const
{
    return touch_major_;
}

void MirMotionPointer::set_touch_major(float major)
{
    touch_major_ = major;
}

float MirMotionPointer::touch_minor() const
{
    return touch_minor_;
}

void MirMotionPointer::set_touch_minor(float minor)
{
    touch_minor_ = minor;
}

float MirMotionPointer::size() const
{
    return size_;
}

void MirMotionPointer::set_size(float size)
{
    size_ = size;
}

float MirMotionPointer::pressure() const
{
    return pressure_;
}

void MirMotionPointer::set_pressure(float pressure)
{
    pressure_ = pressure;
}

float MirMotionPointer::orientation() const
{
    return orientation_;
}

void MirMotionPointer::set_orientation(float orientation)
{
    orientation_ = orientation;
}

float MirMotionPointer::vscroll() const
{
    return vscroll_;
}

void MirMotionPointer::set_vscroll(float vscroll)
{
    vscroll_ = vscroll;
}

float MirMotionPointer::hscroll() const
{
    return hscroll_;
}

void MirMotionPointer::set_hscroll(float hscroll)
{
    hscroll_ = hscroll;
}

MirTouchTooltype MirMotionPointer::tool_type() const
{
    return tool_type_;
}

void MirMotionPointer::set_tool_type(MirTouchTooltype tool_type)
{
    tool_type_ = tool_type;
}

int MirMotionPointer::action() const
{
    return action_;
}

void MirMotionPointer::set_action(int action)
{
    action_ = action;
}
