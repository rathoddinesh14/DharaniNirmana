#pragma once

#include <cstddef>
#include <span>
#include <vector>

#include <aryabhatta/math/math.hpp>

#include "dharaninirmana/core/result.hpp"
#include "dharaninirmana/terrain/terrain_origin.hpp"

namespace dharaninirmana::terrain
{
    class HeightField
    {
    public:
        [[nodiscard]] static core::Result<HeightField> create(
            std::size_t width,
            std::size_t depth,
            double spacingX,
            double spacingZ,
            TerrainOrigin origin,
            std::vector<double> heights);

        [[nodiscard]] std::size_t width() const noexcept;
        [[nodiscard]] std::size_t depth() const noexcept;
        [[nodiscard]] double spacingX() const noexcept;
        [[nodiscard]] double spacingZ() const noexcept;
        [[nodiscard]] TerrainOrigin origin() const noexcept;
        [[nodiscard]] std::size_t sampleCount() const noexcept;

        [[nodiscard]] core::Result<double> heightAt(
            std::size_t x, std::size_t z) const;

        [[nodiscard]] core::Result<void> setHeight(
            std::size_t x, std::size_t z, double height);

        [[nodiscard]] core::Result<aryabhatta::math::DVec3> samplePosition(
            std::size_t x, std::size_t z) const;

        [[nodiscard]] std::span<const double> heights() const noexcept;

    private:
        HeightField(std::size_t width,
                    std::size_t depth,
                    double spacingX,
                    double spacingZ,
                    TerrainOrigin origin,
                    std::vector<double> heights);

        [[nodiscard]] bool contains(std::size_t x, std::size_t z) const noexcept;
        [[nodiscard]] std::size_t indexOf(std::size_t x, std::size_t z) const noexcept;

        std::size_t width_{};
        std::size_t depth_{};
        double spacingX_{};
        double spacingZ_{};
        TerrainOrigin origin_{};
        std::vector<double> heights_;
    };

} // namespace dharaninirmana::terrain
