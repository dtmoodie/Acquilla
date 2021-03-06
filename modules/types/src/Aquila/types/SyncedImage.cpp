#include "SyncedImage.hpp"
#include <MetaObject/logging/logging.hpp>
#include <MetaObject/logging/logging_macros.hpp>

#include <ce/ICacheEngine.hpp>

namespace aq
{

    PixelType SyncedImage::pixelType() const
    {
        return m_pixel_type;
    }

    PixelFormat SyncedImage::pixelFormat() const
    {
        return m_pixel_type.pixel_format;
    }

    size_t SyncedImage::pixelSize() const
    {
        return m_pixel_type.pixel_format.numChannels() * m_pixel_type.data_type.elemSize();
    }

    size_t SyncedImage::elemSize() const
    {
        return m_pixel_type.data_type.elemSize();
    }

    size_t SyncedImage::sizeBytes() const
    {
        return m_shape.numel() * elemSize();
    }

    DataFlag SyncedImage::dataType() const
    {
        return m_pixel_type.data_type;
    }

    uint8_t SyncedImage::channels() const
    {
        return m_pixel_type.pixel_format.numChannels();
    }

    Shape<3> SyncedImage::shape() const
    {
        return {m_shape(0), m_shape(1), channels()};
    }

    void SyncedImage::reshape(Shape<2> shape)
    {
        m_shape(0) = shape(0);
        m_shape(1) = shape(1);
    }

    uint32_t SyncedImage::rows() const
    {
        return m_shape(0);
    }

    uint32_t SyncedImage::cols() const
    {
        return m_shape(1);
    }

    /////////////////////////////////////////////////////////////////////////////

    SyncedImage::SyncedImage(Shape<2> size, PixelFormat fmt, DataFlag type, std::shared_ptr<mo::IDeviceStream> stream)
        : m_pixel_type{type, fmt}
        , m_shape(size)
    {
        if (size.numel())
        {
            create(size, fmt, type);
        }
        setStream(stream);
    }

    SyncedImage::SyncedImage(const SyncedImage& other, std::shared_ptr<mo::IDeviceStream> stream)
        : m_data(other.m_data)
        , m_pixel_type(other.m_pixel_type)
        , m_shape(other.m_shape)
    {
        setHash(other.hash());
        m_data.setConst();
        auto current = static_cast<const ce::shared_ptr<SyncedMemory>&>(m_data)->stream().lock();
        if(current != stream)
        {
            setStream(stream);
        }
    }

    SyncedImage::SyncedImage(SyncedImage&& other, std::shared_ptr<mo::IDeviceStream> stream)
        : m_data(other.m_data)
        , m_pixel_type(other.m_pixel_type)
        , m_shape(other.m_shape)
    {
        setHash(other.hash());
        setStream(stream);
    }

    SyncedImage::SyncedImage(SyncedImage& other, std::shared_ptr<mo::IDeviceStream> stream)
        : m_data(other.m_data)
        , m_pixel_type(other.m_pixel_type)
        , m_shape(other.m_shape)
    {
        setHash(other.hash());
        setStream(stream);
    }

    SyncedImage& SyncedImage::operator=(const SyncedImage&) = default;

    SyncedImage& SyncedImage::operator=(SyncedImage& other) = default;

    SyncedImage& SyncedImage::operator=(SyncedImage&& other) = default;

    void SyncedImage::create(Shape<2> size, PixelFormat fmt, DataFlag type)
    {
        if (fmt == PixelFormat::kUNCHANGED)
        {
            MO_ASSERT(m_pixel_type.pixel_format != PixelFormat::kUNCHANGED);
        }
        else
        {
            m_pixel_type.pixel_format = fmt;
        }

        m_pixel_type.data_type = type;

        m_shape = std::move(size);
        makeData();
    }

    void SyncedImage::create(uint32_t height, uint32_t width, PixelFormat fmt, DataFlag type)
    {
        create({height, width}, fmt, type);
    }

    std::weak_ptr<mo::IDeviceStream> SyncedImage::stream() const
    {
        return m_data->stream();
    }

    void SyncedImage::makeData()
    {
        std::shared_ptr<mo::IDeviceStream> stream = mo::IDeviceStream::current();
        if (m_data)
        {
            stream = m_data->stream().lock();
        }

        const auto num_pixels = m_shape.numel();
        const auto pixel_size = m_pixel_type.pixel_format.numChannels() * m_pixel_type.data_type.elemSize();

        m_data = ce::shared_ptr<SyncedMemory>::create(num_pixels * pixel_size, pixel_size, std::move(stream));
    }

    void SyncedImage::setStream(std::shared_ptr<mo::IDeviceStream> stream)
    {
        if (!m_data)
        {
            makeData();
        }
        m_data->setStream(std::move(stream));
    }

    ce::shared_ptr<SyncedMemory> SyncedImage::data()
    {
        return m_data;
    }

    ce::shared_ptr<const SyncedMemory> SyncedImage::data() const
    {
        return m_data;
    }

    void SyncedImage::setData(ce::shared_ptr<SyncedMemory> data)
    {
        MO_ASSERT(data.get() != nullptr);
        auto size = m_shape.numel() * pixelSize();
        if (size > 0)
        {
            MO_ASSERT_EQ(data->size(), size);
            m_data = data;
        }
    }

    bool SyncedImage::empty() const
    {
        return m_shape.numel() == 0;
    }
}

namespace ct
{
    aq::Shape<2> getImageShape(const aq::SyncedImage& img)
    {
        return {img.rows(), img.cols()};
    }

    TArrayView<void> makeArrayView(AccessToken<void (aq::SyncedImage::*)(ce::shared_ptr<aq::SyncedMemory>)>&& view,
                                   size_t)
    {
        return view.operator ce::shared_ptr<aq::SyncedMemory>&()->mutableHost();
    }
}
