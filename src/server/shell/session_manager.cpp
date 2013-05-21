/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#include "mir/shell/session_manager.h"
#include "mir/shell/application_session.h"
#include "mir/shell/session_container.h"
#include "mir/shell/surface_factory.h"
#include "mir/shell/focus_sequence.h"
#include "mir/shell/focus_setter.h"
#include "mir/shell/session.h"
#include "mir/shell/surface.h"
#include "mir/shell/input_targeter.h"
#include "mir/shell/session_listener.h"

#include <memory>
#include <cassert>
#include <algorithm>

namespace mf = mir::frontend;
namespace msh = mir::shell;

msh::SessionManager::SessionManager(std::shared_ptr<msh::SurfaceFactory> const& surface_factory,
    std::shared_ptr<msh::SessionContainer> const& container,
    std::shared_ptr<msh::FocusSequence> const& sequence,
    std::shared_ptr<msh::FocusSetter> const& focus_setter,
    std::shared_ptr<msh::InputTargeter> const& input_targeter,
    std::shared_ptr<msh::SessionListener> const& session_listener) :
    surface_factory(surface_factory),
    app_container(container),
    focus_sequence(sequence),
    focus_setter(focus_setter),
    input_targeter(input_targeter),
    session_listener(session_listener)
{
    assert(surface_factory);
    assert(sequence);
    assert(container);
    assert(focus_setter);
    assert(input_targeter);
    assert(session_listener);
}

msh::SessionManager::~SessionManager()
{
    /*
     * Close all open sessions. We need to do this manually here
     * to break the cyclic dependency between msh::Session
     * and msh::InputTargeter, since our implementations
     * of these interfaces keep strong references to each other.
     * TODO: Investigate other solutions (e.g. weak_ptr)
     */
    std::vector<std::shared_ptr<msh::Session>> sessions;

    app_container->for_each([&](std::shared_ptr<Session> const& session)
    {
        sessions.push_back(session);
    });

    for (auto& session : sessions)
        close_session(session);
}

std::shared_ptr<mf::Session> msh::SessionManager::open_session(
    std::string const& name,
    std::shared_ptr<events::EventSink> const& sink)
{
    std::shared_ptr<msh::Session> new_session = std::make_shared<msh::ApplicationSession>(surface_factory, name, sink);

    app_container->insert_session(new_session);
    
    session_listener->starting(new_session);

    set_focus_to_locked(std::unique_lock<std::mutex>(mutex), new_session);

    return new_session;
}

inline void msh::SessionManager::set_focus_to_locked(std::unique_lock<std::mutex> const&, std::shared_ptr<Session> const& shell_session)
{
    auto old_focus = focus_application.lock();

    focus_application = shell_session;
    focus_setter->set_focus_to(shell_session);

    if (shell_session && shell_session->default_surface())
        input_targeter->focus_changed(shell_session->default_surface()->internal_surface());
    else if (shell_session == old_focus || !shell_session)
        input_targeter->focus_cleared();
}

void msh::SessionManager::close_session(std::shared_ptr<mf::Session> const& session)
{
    auto shell_session = std::dynamic_pointer_cast<Session>(session);

    session_listener->stopping(shell_session);

    app_container->remove_session(shell_session);

    std::unique_lock<std::mutex> lock(mutex);
    set_focus_to_locked(lock, focus_sequence->default_focus());

    typedef Tags::value_type Pair;

    auto remove = std::remove_if(tags.begin(), tags.end(),
        [&](Pair const& v) { return v.second == shell_session;});

    tags.erase(remove, tags.end());
}

void msh::SessionManager::focus_next()
{
    std::unique_lock<std::mutex> lock(mutex);
    auto focus = focus_application.lock();
    if (!focus)
    {
        focus = focus_sequence->default_focus();
    }
    else
    {
        focus = focus_sequence->successor_of(focus);
    }
    set_focus_to_locked(lock, focus);
}

mf::SurfaceId msh::SessionManager::create_surface_for(std::shared_ptr<mf::Session> const& session,
    msh::SurfaceCreationParameters const& params)
{
    auto shell_session = std::dynamic_pointer_cast<Session>(session);
    auto id = shell_session->create_surface(params);

    set_focus_to_locked(std::unique_lock<std::mutex>(mutex), shell_session);

    return id;
}
