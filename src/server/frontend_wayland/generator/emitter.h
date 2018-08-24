/*
 * Copyright © 2018 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
 * Authored By: William Wold <william.wold@canonical.com>
 */

#ifndef MIR_WAYLAND_GENERATOR_EMITTER_H
#define MIR_WAYLAND_GENERATOR_EMITTER_H

#include <ostream>
#include <initializer_list>
#include <vector>
#include <memory>

class Emitter;

// Simply a newline
// implicitly convertible to an Emitter
struct EmptyLine
{
};

// a line that is at the same indentation level as surrounding block
// implicitly convertible to an Emitter
struct Line
{
    Line(std::initializer_list<Emitter> const& emitters) : emitters{emitters} {}
    Line(std::vector<Emitter> const& emitters) : emitters{emitters} {}

    std::vector<Emitter> emitters;
};

// a series of lines that is at the same indentation level as surrounding block
// implicitly convertible to an Emitter
struct Lines
{
    Lines(std::initializer_list<Emitter> const& emitters) : emitters{emitters} {}
    Lines(std::vector<Emitter> const& emitters) : emitters{emitters} {}

    std::vector<Emitter> emitters;
};

// an indented curly brace surrounded block
// implicitly convertible to an Emitter
struct Block
{
    Block(std::initializer_list<Emitter> const& emitters) : emitters{emitters} {}
    Block(std::vector<Emitter> const& emitters) : emitters{emitters} {}

    std::vector<Emitter> emitters;
};

// a list of items with delimiters (such as commas) between them
// implicitly convertible to an Emitter
struct List
{
    List(std::initializer_list<Emitter> const& items, Emitter const& delimiter) : items{items}, delimiter{delimiter} {}
    List(std::vector<Emitter> const& items, Emitter const& delimiter) : items{items}, delimiter{delimiter} {}

    std::vector<Emitter> items;
    Emitter const& delimiter;
};

class Emitter
{
public:
    struct State
    {
        std::ostream& out;
        std::shared_ptr<bool> const on_fresh_line;
        std::string indent;
    };

    Emitter() = delete;

    // constructors for simple emitters
    Emitter(std::string const& text);
    Emitter(const char* text);
    Emitter(std::initializer_list<Emitter> const& emitters);
    Emitter(std::vector<Emitter> const& emitters);

    // constructors for complex emitters
    Emitter(EmptyLine);
    Emitter(Line && line);
    Emitter(Lines && lines);
    Emitter(Block && block);
    Emitter(List && list);

    void emit(State state) const;
    bool is_valid() const { return impl != nullptr; }

    class Impl;

private:
    Emitter(std::shared_ptr<Impl const> impl);

    std::shared_ptr<Impl const> impl;

    static std::string const single_indent;
};

#endif // MIR_WAYLAND_GENERATOR_EMITTER_H