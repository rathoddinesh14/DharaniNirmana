#include "dharaninirmana/terrain/height_field.hpp"

#include <cmath>
#include <limits>
#include <string>
#include <utility>

#include "dharaninirmana/core/error.hpp"
#include "dharaninirmana/core/error_code.hpp"

namespace dharaninirmana::terrain
{
    namespace
    {

        using core::Error;
        using core::ErrorCode;

        [[nodiscard]] core::Result<HeightField> invalidField(
            ErrorCode code, const char *message)
        {
            return core::Result<HeightField>::failure(Error{code, message});
        }

        [[nodiscard]] bool finite(double value) noexcept
        {
            return std::isfinite(value);
        }

    } // namespace

    HeightField::HeightField(std::size_t width,
                             std::size_t depth,
                             double spacingX,
                             double spacingZ,
                             TerrainOrigin origin,
                             std::vector<double> heights)
        : width_(width),
          depth_(depth),
          spacingX_(spacingX),
          spacingZ_(spacingZ),
          origin_(origin),
          heights_(std::move(heights)) {}

    core::Result<HeightField> HeightField::create(
        std::size_t width,
        std::size_t depth,
        double spacingX,
        double spacingZ,
        TerrainOrigin origin,
        std::vector<double> heights)
    {

        if (width < 2 || depth < 2)
        {
            return invalidField(ErrorCode::InvalidArgument,
                                "HeightField width and depth must both be at least 2.");
        }

        if (width > std::numeric_limits<std::size_t>::max() / depth)
        {
            return invalidField(ErrorCode::InvalidArgument,
                                "HeightField sample-count multiplication overflows size_t.");
        }

        const std::size_t expectedCount = width * depth;
        if (heights.size() != expectedCount)
        {
            return invalidField(ErrorCode::InvalidData,
                                "HeightField elevation storage size must equal width * depth.");
        }

        if (!finite(spacingX) || spacingX <= 0.0 ||
            !finite(spacingZ) || spacingZ <= 0.0)
        {
            return invalidField(ErrorCode::InvalidArgument,
                                "HeightField sample spacing must be finite and strictly positive.");
        }

        if (!finite(origin.x) || !finite(origin.z))
        {
            return invalidField(ErrorCode::InvalidArgument,
                                "HeightField origin coordinates must be finite.");
        }

        for (double height : heights)
        {
            if (!finite(height))
            {
                return invalidField(ErrorCode::InvalidData,
                                    "HeightField elevations must all be finite.");
            }
        }

        const double lastX =
            origin.x + static_cast<double>(width - 1) * spacingX;
        const double lastZ =
            origin.z + static_cast<double>(depth - 1) * spacingZ;

        if (!finite(lastX) || !finite(lastZ))
        {
            return invalidField(ErrorCode::InvalidArgument,
                                "HeightField spatial extent must remain finite.");
        }

        return core::Result<HeightField>::success(
            HeightField{width, depth, spacingX, spacingZ, origin, std::move(heights)});
    }

    std::size_t HeightField::width() const noexcept { return width_; }
    std::size_t HeightField::depth() const noexcept { return depth_; }
    double HeightField::spacingX() const noexcept { return spacingX_; }
    double HeightField::spacingZ() const noexcept { return spacingZ_; }
    TerrainOrigin HeightField::origin() const noexcept { return origin_; }
    std::size_t HeightField::sampleCount() const noexcept { return heights_.size(); }

    bool HeightField::contains(std::size_t x, std::size_t z) const noexcept
    {
        return x < width_ && z < depth_;
    }

    std::size_t HeightField::indexOf(std::size_t x, std::size_t z) const noexcept
    {
        return z * width_ + x;
    }

    core::Result<double> HeightField::heightAt(
        std::size_t x, std::size_t z) const
    {
        if (!contains(x, z))
        {
            return core::Result<double>::failure(
                Error{ErrorCode::InvalidArgument,
                      "HeightField sample coordinates are out of range."});
        }

        return core::Result<double>::success(heights_[indexOf(x, z)]);
    }

    core::Result<void> HeightField::setHeight(
        std::size_t x, std::size_t z, double height)
    {
        if (!contains(x, z))
        {
            return core::Result<void>::failure(
                Error{ErrorCode::InvalidArgument,
                      "HeightField sample coordinates are out of range."});
        }

        if (!finite(height))
        {
            return core::Result<void>::failure(
                Error{ErrorCode::InvalidArgument,
                      "HeightField elevation must be finite."});
        }

        heights_[indexOf(x, z)] = height;
        return core::Result<void>::success();
    }

    core::Result<aryabhatta::math::DVec3> HeightField::samplePosition(
        std::size_t x, std::size_t z) const
    {
        if (!contains(x, z))
        {
            return core::Result<aryabhatta::math::DVec3>::failure(
                Error{ErrorCode::InvalidArgument,
                      "HeightField sample coordinates are out of range."});
        }

        const double worldX =
            origin_.x + static_cast<double>(x) * spacingX_;
        const double worldY = heights_[indexOf(x, z)];
        const double worldZ =
            origin_.z + static_cast<double>(z) * spacingZ_;

        if (!finite(worldX) || !finite(worldY) || !finite(worldZ))
        {
            return core::Result<aryabhatta::math::DVec3>::failure(
                Error{ErrorCode::InvalidData,
                      "Calculated HeightField sample position is not finite."});
        }

        return core::Result<aryabhatta::math::DVec3>::success(
            aryabhatta::math::DVec3{worldX, worldY, worldZ});
    }

    std::span<const double> HeightField::heights() const noexcept
    {
        return heights_;
    }

} // namespace dharaninirmana::terrain
