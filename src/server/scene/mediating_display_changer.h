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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_SCENE_MEDIATING_DISPLAY_CHANGER_H_
#define MIR_SCENE_MEDIATING_DISPLAY_CHANGER_H_

#include "mir/frontend/display_changer.h"
#include "mir/display_changer.h"
#include "mir/shell/display_configuration_controller.h"
#include "mir/input/input_region.h"

#include <mutex>
#include <map>
#include <mir/graphics/display_configuration.h>

namespace mir
{
class ServerActionQueue;

namespace time
{
class Alarm;
class AlarmFactory;
}

namespace graphics
{
    class Display;
    class DisplayConfigurationPolicy;
    class DisplayConfigurationReport;
}
namespace compositor { class Compositor; }
namespace input
{
class InputRegion;
}
namespace scene
{
class SessionEventHandlerRegister;
class SessionContainer;
class Session;

class MediatingDisplayChanger : public frontend::DisplayChanger,
                                public mir::DisplayChanger,
                                public shell::DisplayConfigurationController
{
public:
    MediatingDisplayChanger(
        std::shared_ptr<graphics::Display> const& display,
        std::shared_ptr<compositor::Compositor> const& compositor,
        std::shared_ptr<graphics::DisplayConfigurationPolicy> const& display_configuration_policy,
        std::shared_ptr<SessionContainer> const& session_container,
        std::shared_ptr<SessionEventHandlerRegister> const& session_event_handler_register,
        std::shared_ptr<ServerActionQueue> const& server_action_queue,
        std::shared_ptr<graphics::DisplayConfigurationReport> const& report,
        std::shared_ptr<input::InputRegion> const& region,
        std::shared_ptr<time::AlarmFactory> const& alarm_factory);

    /* From mir::frontend::DisplayChanger */
    std::shared_ptr<graphics::DisplayConfiguration> base_configuration() override;
    void configure(std::shared_ptr<frontend::Session> const& session,
                   std::shared_ptr<graphics::DisplayConfiguration> const& conf) override;
    void preview_base_configuration(
        std::weak_ptr<frontend::Session> const& session,
        std::shared_ptr<graphics::DisplayConfiguration> const& conf,
        std::chrono::seconds timeout) override;
    void confirm_base_configuration(
        std::shared_ptr<frontend::Session> const& session,
        std::shared_ptr<graphics::DisplayConfiguration> const& confirmed_conf) override;

    void cancel_base_configuration_preview(
        std::shared_ptr<frontend::Session> const& session) override;

    /* From mir::DisplayChanger */
    void configure_for_hardware_change(
        std::shared_ptr<graphics::DisplayConfiguration> const& conf,
        SystemStateHandling pause_resume_system) override;

    void pause_display_config_processing() override;
    void resume_display_config_processing() override;

    /* From shell::DisplayConfigurationController */
    void set_base_configuration(std::shared_ptr<graphics::DisplayConfiguration> const &conf) override;

private:
    void focus_change_handler(std::shared_ptr<Session> const& session);
    void no_focus_handler();
    void session_stopping_handler(std::shared_ptr<Session> const& session);

    void apply_config(std::shared_ptr<graphics::DisplayConfiguration> const& conf,
                      SystemStateHandling pause_resume_system);
    void apply_base_config(SystemStateHandling pause_resume_system);
    void send_config_to_all_sessions(
        std::shared_ptr<graphics::DisplayConfiguration> const& conf);
    void update_input_rectangles(graphics::DisplayConfiguration const& conf);

    std::shared_ptr<graphics::Display> const display;
    std::shared_ptr<compositor::Compositor> const compositor;
    std::shared_ptr<graphics::DisplayConfigurationPolicy> const display_configuration_policy;
    std::shared_ptr<SessionContainer> const session_container;
    std::shared_ptr<SessionEventHandlerRegister> const session_event_handler_register;
    std::shared_ptr<ServerActionQueue> const server_action_queue;
    std::shared_ptr<graphics::DisplayConfigurationReport> const report;
    std::mutex configuration_mutex;
    std::map<std::weak_ptr<frontend::Session>,
             std::shared_ptr<graphics::DisplayConfiguration>,
             std::owner_less<std::weak_ptr<frontend::Session>>> config_map;
    std::weak_ptr<frontend::Session> focused_session;
    std::shared_ptr<graphics::DisplayConfiguration> base_configuration_;
    bool base_configuration_applied;
    std::shared_ptr<input::InputRegion> const region;
    std::shared_ptr<time::AlarmFactory> const alarm_factory;
    std::unique_ptr<time::Alarm> preview_configuration_timeout;
    std::weak_ptr<frontend::Session> currently_previewing_session;
};

}
}

#endif /* MIR_SCENE_MEDIATING_DISPLAY_CHANGER_H_ */
