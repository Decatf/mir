
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

#include "src/graphics/gbm/gbm_platform.h"
#include "mir/compositor/graphic_buffer_allocator.h"
#include "src/graphics/gbm/gbm_buffer_allocator.h"
#include "mir/compositor/buffer_properties.h"

#include "mock_drm.h"
#include "mock_gbm.h"
#include "mir_test/egl_mock.h"
#include "mir_test/gl_mock.h"
#include "mir_test_doubles/mock_buffer_initializer.h"
#include "mir_test_doubles/mock_id_generator.h"

#include <memory>
#include <stdexcept>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <gbm.h>

namespace mg = mir::graphics;
namespace mgg = mir::graphics::gbm;
namespace mc = mir::compositor;
namespace geom = mir::geometry;
namespace mtd = mir::test::doubles;

class GBMBufferAllocatorTest  : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        using namespace testing;

        size = geom::Size{geom::Width{300}, geom::Height{200}};
        pf = geom::PixelFormat::rgba_8888;
        usage = mc::BufferUsage::hardware;
        buffer_properties = mc::BufferProperties{size, pf, usage};

        ON_CALL(mock_gbm, gbm_bo_get_handle(_))
        .WillByDefault(Return(mock_gbm.fake_gbm.bo_handle));

        typedef mir::EglMock::generic_function_pointer_t func_ptr_t;

        ON_CALL(mock_egl, eglGetProcAddress(StrEq("eglCreateImageKHR")))
            .WillByDefault(Return(reinterpret_cast<func_ptr_t>(eglCreateImageKHR)));
        ON_CALL(mock_egl, eglGetProcAddress(StrEq("eglDestroyImageKHR")))
            .WillByDefault(Return(reinterpret_cast<func_ptr_t>(eglDestroyImageKHR)));
        ON_CALL(mock_egl, eglGetProcAddress(StrEq("glEGLImageTargetTexture2DOES")))
            .WillByDefault(Return(reinterpret_cast<func_ptr_t>(glEGLImageTargetTexture2DOES)));

        platform = std::make_shared<mgg::GBMPlatform>();
        mock_buffer_initializer = std::make_shared<testing::NiceMock<mtd::MockBufferInitializer>>();
        allocator.reset(new mgg::GBMBufferAllocator(platform, mock_buffer_initializer));
    }

    // Defaults
    geom::Size size;
    geom::PixelFormat pf;
    mc::BufferUsage usage;
    mc::BufferProperties buffer_properties;

    ::testing::NiceMock<mgg::MockDRM> mock_drm;
    ::testing::NiceMock<mgg::MockGBM> mock_gbm;
    ::testing::NiceMock<mir::EglMock> mock_egl;
    ::testing::NiceMock<mir::GLMock> mock_gl;
    std::shared_ptr<mgg::GBMPlatform> platform;
    std::shared_ptr<testing::NiceMock<mtd::MockBufferInitializer>> mock_buffer_initializer;
    std::unique_ptr<mgg::GBMBufferAllocator> allocator;
};

TEST_F(GBMBufferAllocatorTest, allocator_returns_non_null_buffer)
{
    using namespace testing;
    EXPECT_CALL(mock_gbm, gbm_bo_create(_,_,_,_,_));
    EXPECT_CALL(mock_gbm, gbm_bo_destroy(_));

    EXPECT_TRUE(allocator->alloc_buffer(buffer_properties).get() != NULL);
}

TEST_F(GBMBufferAllocatorTest, correct_buffer_format_translation_rgba_8888)
{
    using namespace testing;

    EXPECT_CALL(mock_gbm, gbm_bo_create(_,_,_,GBM_FORMAT_ARGB8888,_));
    EXPECT_CALL(mock_gbm, gbm_bo_destroy(_));

    allocator->alloc_buffer(mc::BufferProperties{size, geom::PixelFormat::rgba_8888, usage});
}

TEST_F(GBMBufferAllocatorTest, correct_buffer_format_translation_rgbx_8888)
{
    using namespace testing;

    EXPECT_CALL(mock_gbm, gbm_bo_create(_,_,_,GBM_FORMAT_XRGB8888,_));
    EXPECT_CALL(mock_gbm, gbm_bo_destroy(_));

    allocator->alloc_buffer(mc::BufferProperties{size, geom::PixelFormat::rgbx_8888, usage});
}

MATCHER_P(has_flag_set, flag, "")
{
    return arg & flag;
}

TEST_F(GBMBufferAllocatorTest, creates_hardware_rendering_buffer)
{
    using namespace testing;

    mc::BufferProperties properties{size, pf, mc::BufferUsage::hardware};

    EXPECT_CALL(mock_gbm, gbm_bo_create(_,_,_,_,has_flag_set(GBM_BO_USE_RENDERING)));
    EXPECT_CALL(mock_gbm, gbm_bo_destroy(_));

    allocator->alloc_buffer(properties);
}

TEST_F(GBMBufferAllocatorTest, creates_software_rendering_buffer)
{
    using namespace testing;

    mc::BufferProperties properties{size, pf, mc::BufferUsage::software};

    EXPECT_CALL(mock_gbm, gbm_bo_create(_,_,_,_,has_flag_set(GBM_BO_USE_WRITE)));
    EXPECT_CALL(mock_gbm, gbm_bo_destroy(_));

    allocator->alloc_buffer(properties);
}

TEST_F(GBMBufferAllocatorTest, creates_hardware_rendering_buffer_for_undefined_usage)
{
    using namespace testing;

    mc::BufferProperties properties{size, pf, mc::BufferUsage::undefined};

    EXPECT_CALL(mock_gbm, gbm_bo_create(_,_,_,_,has_flag_set(GBM_BO_USE_RENDERING)));
    EXPECT_CALL(mock_gbm, gbm_bo_destroy(_));

    allocator->alloc_buffer(properties);
}

TEST_F(GBMBufferAllocatorTest, requests_correct_buffer_dimensions)
{
    using namespace testing;

    EXPECT_CALL(mock_gbm, gbm_bo_create(_,size.width.as_uint32_t(),size.height.as_uint32_t(),_,_));
    EXPECT_CALL(mock_gbm, gbm_bo_destroy(_));

    allocator->alloc_buffer(buffer_properties);
}

TEST_F(GBMBufferAllocatorTest, correct_buffer_handle_is_destroyed)
{
    using namespace testing;
    gbm_bo* bo{reinterpret_cast<gbm_bo*>(0xabcd)};

    EXPECT_CALL(mock_gbm, gbm_bo_create(_,_,_,_,_))
    .WillOnce(Return(bo));
    EXPECT_CALL(mock_gbm, gbm_bo_destroy(bo));

    allocator->alloc_buffer(buffer_properties);
}

TEST_F(GBMBufferAllocatorTest, buffer_initializer_is_called)
{
    using namespace testing;

    EXPECT_CALL(*mock_buffer_initializer, operator_call(_))
        .Times(1);

    allocator->alloc_buffer(buffer_properties);
}

TEST_F(GBMBufferAllocatorTest, null_buffer_initializer_does_not_crash)
{
    using namespace testing;

    auto null_buffer_initializer = std::make_shared<mg::NullBufferInitializer>();
    allocator.reset(new mgg::GBMBufferAllocator(platform, null_buffer_initializer));

    EXPECT_NO_THROW({
        allocator->alloc_buffer(buffer_properties);
    });
}

TEST_F(GBMBufferAllocatorTest, throws_on_buffer_creation_failure)
{
    using namespace testing;

    EXPECT_CALL(mock_gbm, gbm_bo_create(_,_,_,_,_))
        .WillOnce(Return(reinterpret_cast<gbm_bo*>(0)));
    EXPECT_CALL(mock_gbm, gbm_bo_destroy(_))
        .Times(0);

    EXPECT_THROW({
        allocator->alloc_buffer(buffer_properties);
    }, std::runtime_error);
}

TEST_F(GBMBufferAllocatorTest, constructor_throws_if_egl_image_not_supported)
{
    using namespace testing;
    typedef mir::EglMock::generic_function_pointer_t func_ptr_t;

    ON_CALL(mock_egl, eglGetProcAddress(StrEq("eglCreateImageKHR")))
        .WillByDefault(Return(reinterpret_cast<func_ptr_t>(0)));
    ON_CALL(mock_egl, eglGetProcAddress(StrEq("eglDestroyImageKHR")))
        .WillByDefault(Return(reinterpret_cast<func_ptr_t>(0)));

    EXPECT_THROW({
        mgg::GBMBufferAllocator allocator(platform, mock_buffer_initializer);
    }, std::runtime_error);
}

TEST_F(GBMBufferAllocatorTest, constructor_throws_if_gl_oes_egl_image_not_supported)
{
    using namespace testing;
    typedef mir::EglMock::generic_function_pointer_t func_ptr_t;

    ON_CALL(mock_egl, eglGetProcAddress(StrEq("glEGLImageTargetTexture2DOES")))
        .WillByDefault(Return(reinterpret_cast<func_ptr_t>(0)));

    EXPECT_THROW({
        mgg::GBMBufferAllocator allocator(platform, mock_buffer_initializer);
    }, std::runtime_error);
}

TEST_F(GBMBufferAllocatorTest, basic_id_generation)
{
    auto id1 = mc::BufferID{4};
    auto id2 = mc::BufferID{5};
    auto mock_id_generator = std::unique_ptr<mtd::MockIDGenerator>(new mtd::MockIDGenerator);
    /* we move a unique_ptr in test. google mock leak checker doesn't know how to deal with things 
       that are moved, so we manually tell gmock to not check the object */
    testing::Mock::AllowLeak(mock_id_generator.get());

    using namespace testing;
    EXPECT_CALL(*mock_id_generator, generate_unique_id())
        .Times(2)
        .WillOnce(Return(id1))
        .WillRepeatedly(Return(id2));

    mgg::GBMBufferAllocator allocator(platform, mock_buffer_initializer);

    auto buffer1 = allocator.alloc_buffer(buffer_properties);
    auto buffer2 = allocator.alloc_buffer(buffer_properties);

    EXPECT_EQ(buffer1->id(), id1);
    EXPECT_EQ(buffer2->id(), id2);

}
