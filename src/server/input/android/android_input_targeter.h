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

#ifndef MIR_INPUT_ANDROID_TARGET_H_
#define MIR_INPUT_ANDROID_TARGET_H_

#include "mir/shell/input_targeter.h"

#include <utils/StrongPointer.h>

#include <map>
#include <mutex>

namespace android
{
class InputDispatcherInterface;
class InputWindowHandle;
}

namespace droidinput = android;

namespace mir
{
namespace shell
{
class InputRegistrar;
}
namespace input
{
namespace android
{
class InputConfiguration;
class InputRegistrar;

class InputTargeter : public shell::InputTargeter
{
public:
    explicit InputTargeter(std::shared_ptr<InputConfiguration> const& input_configuration,
                           std::shared_ptr<shell::InputRegistrar> const& input_registrar);
    virtual ~InputTargeter() noexcept(true) {}
    
    void focus_changed(std::shared_ptr<input::SurfaceTarget const> const& focus_surface);
    void focus_cleared();

protected:
    InputTargeter(const InputTargeter&) = delete;
    InputTargeter& operator=(const InputTargeter&) = delete;

private:
    droidinput::sp<droidinput::InputDispatcherInterface> input_dispatcher;

    std::shared_ptr<InputRegistrar> const input_registrar;
};

}
}
} // namespace mir

#endif // MIR_INPUT_ANDROID_TARGET_H_
