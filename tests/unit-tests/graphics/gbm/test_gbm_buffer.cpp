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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "mir_test/egl_mock.h"
#include "mir_test/gl_mock.h"
#include "mir_test_doubles/mock_drm.h"
#include "mir_test_doubles/mock_gbm.h"

#include "src/server/graphics/gbm/gbm_platform.h"
#include "src/server/graphics/gbm/gbm_buffer.h"
#include "src/server/graphics/gbm/gbm_buffer_allocator.h"
#include "mir/graphics/buffer_initializer.h"
#include "mir/compositor/buffer_ipc_package.h"
#include "mir/compositor/buffer_properties.h"
#include "mir_test_doubles/null_virtual_terminal.h"

#include "mir/graphics/null_display_report.h"

#include <gbm.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdint>
#include <stdexcept>

namespace mc=mir::compositor;
namespace mg=mir::graphics;
namespace mgg=mir::graphics::gbm;
namespace geom=mir::geometry;
namespace mtd=mir::test::doubles;

class GBMGraphicBufferBasic : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        using namespace testing;

        size = geom::Size{geom::Width{300}, geom::Height{200}};
        pf = geom::PixelFormat::argb_8888;
        stride = geom::Stride{4 * size.width.as_uint32_t()};
        usage = mc::BufferUsage::hardware;
        buffer_properties = mc::BufferProperties{size, pf, usage};

        ON_CALL(mock_gbm, gbm_bo_get_width(_))
        .WillByDefault(Return(size.width.as_uint32_t()));

        ON_CALL(mock_gbm, gbm_bo_get_height(_))
        .WillByDefault(Return(size.height.as_uint32_t()));

        ON_CALL(mock_gbm, gbm_bo_get_format(_))
        .WillByDefault(Return(GBM_BO_FORMAT_ARGB8888));

        ON_CALL(mock_gbm, gbm_bo_get_stride(_))
        .WillByDefault(Return(stride.as_uint32_t()));

        typedef mir::EglMock::generic_function_pointer_t func_ptr_t;

        ON_CALL(mock_egl, eglGetProcAddress(StrEq("eglCreateImageKHR")))
            .WillByDefault(Return(reinterpret_cast<func_ptr_t>(eglCreateImageKHR)));
        ON_CALL(mock_egl, eglGetProcAddress(StrEq("eglDestroyImageKHR")))
            .WillByDefault(Return(reinterpret_cast<func_ptr_t>(eglDestroyImageKHR)));
        ON_CALL(mock_egl, eglGetProcAddress(StrEq("glEGLImageTargetTexture2DOES")))
            .WillByDefault(Return(reinterpret_cast<func_ptr_t>(glEGLImageTargetTexture2DOES)));

        platform = std::make_shared<mgg::GBMPlatform>(std::make_shared<mg::NullDisplayReport>(),
                                                      std::make_shared<mtd::NullVirtualTerminal>());
        null_init = std::make_shared<mg::NullBufferInitializer>();
        allocator.reset(new mgg::GBMBufferAllocator(platform, null_init));
    }

    ::testing::NiceMock<mtd::MockDRM> mock_drm;
    ::testing::NiceMock<mtd::MockGBM> mock_gbm;
    ::testing::NiceMock<mir::EglMock> mock_egl;
    ::testing::NiceMock<mir::GLMock>  mock_gl;
    std::shared_ptr<mgg::GBMPlatform> platform;
    std::shared_ptr<mg::NullBufferInitializer> null_init;
    std::unique_ptr<mgg::GBMBufferAllocator> allocator;

    // Defaults
    geom::PixelFormat pf;
    geom::Size size;
    geom::Stride stride;
    mc::BufferUsage usage;
    mc::BufferProperties buffer_properties;
};

TEST_F(GBMGraphicBufferBasic, dimensions_test)
{
    using namespace testing;

    EXPECT_CALL(mock_gbm, gbm_bo_create(_,_,_,_,_));
    EXPECT_CALL(mock_gbm, gbm_bo_destroy(_));

    auto buffer = allocator->alloc_buffer(buffer_properties);
    ASSERT_EQ(size, buffer->size());
}

TEST_F(GBMGraphicBufferBasic, buffer_has_expected_pixel_format)
{
    using namespace testing;

    EXPECT_CALL(mock_gbm, gbm_bo_create(_,_,_,_,_));
    EXPECT_CALL(mock_gbm, gbm_bo_destroy(_));

    auto buffer(allocator->alloc_buffer(buffer_properties));
    ASSERT_EQ(pf, buffer->pixel_format());
}

TEST_F(GBMGraphicBufferBasic, stride_has_sane_value)
{
    using namespace testing;

    EXPECT_CALL(mock_gbm, gbm_bo_create(_,_,_,_,_));
    EXPECT_CALL(mock_gbm, gbm_bo_destroy(_));

    // RGBA 8888 cannot take less than 4 bytes
    // TODO: is there a *maximum* sane value for stride?
    geom::Stride minimum(size.width.as_uint32_t() * 4);

    auto buffer(allocator->alloc_buffer(buffer_properties));

    ASSERT_LE(minimum, buffer->stride());
}

TEST_F(GBMGraphicBufferBasic, buffer_native_handle_has_correct_size)
{
    using namespace testing;

    auto buffer = allocator->alloc_buffer(buffer_properties);
    auto native_handle = buffer->native_buffer_handle();
    EXPECT_EQ(1, ipc_package->fd_items);
    EXPECT_EQ(0, ipc_package->data_items);
}

MATCHER_P(GEMFlinkHandleIs, value, "")
{
    auto flink = reinterpret_cast<struct drm_gem_flink*>(arg);
    return flink->handle == value;
}

ACTION_P(SetGEMFlinkName, value)
{
    auto flink = reinterpret_cast<struct drm_gem_flink*>(arg2);
    flink->name = value;
}

TEST_F(GBMGraphicBufferBasic, buffer_native_handle_contains_correct_data)
{
    using namespace testing;

    uint32_t prime_fd{0x77};
    gbm_bo_handle mock_handle;
    mock_handle.u32 = 0xdeadbeef;

    EXPECT_CALL(mock_gbm, gbm_bo_get_handle(_))
            .Times(Exactly(1))
            .WillOnce(Return(mock_handle));

    EXPECT_CALL(mock_drm, drmPrimeHandleToFD(_,mock_handle.u32,_,_))
            .Times(Exactly(1))
            .WillOnce(DoAll(SetArgPointee<3>(prime_fd), Return(0)));

    auto buffer = allocator->alloc_buffer(buffer_properties);
    auto handle = buffer->native_buffer_handle();
    EXPECT_EQ(prime_fd, handle->ipc_fds[0]);
    EXPECT_EQ(stride.as_uint32_t(), handle->stride);
}

TEST_F(GBMGraphicBufferBasic, buffer_creation_throws_on_prime_fd_failure)
{
    using namespace testing;

    EXPECT_CALL(mock_drm, drmPrimeHandleToFD(_,_,_,_))
            .Times(Exactly(1))
            .WillOnce(Return(-1));

    EXPECT_THROW({
        auto buffer = allocator->alloc_buffer(buffer_properties);
    }, std::runtime_error);
}

TEST_F(GBMGraphicBufferBasic, bind_to_texture_egl_image_creation_failed)
{
    using namespace testing;

    ON_CALL(mock_egl, eglCreateImageKHR(_,_,_,_,_))
        .WillByDefault(Return(EGL_NO_IMAGE_KHR));

    EXPECT_THROW({
        auto buffer = allocator->alloc_buffer(buffer_properties);
        buffer->bind_to_texture();
    }, std::runtime_error);
}

TEST_F(GBMGraphicBufferBasic, bind_to_texture_uses_egl_image)
{
    using namespace testing;

    {
        InSequence seq;

        EXPECT_CALL(mock_egl, eglCreateImageKHR(_,_,_,_,_))
            .Times(Exactly(1));

        EXPECT_CALL(mock_gl, glEGLImageTargetTexture2DOES(_,mock_egl.fake_egl_image))
            .Times(Exactly(1));

        EXPECT_CALL(mock_egl, eglDestroyImageKHR(_,mock_egl.fake_egl_image))
            .Times(Exactly(1));
    }

    EXPECT_NO_THROW({
        auto buffer = allocator->alloc_buffer(buffer_properties);
        buffer->bind_to_texture();
    });
}
