#pragma once

namespace dharaninirmana::terrain
{
    // Horizontal world-space origin of a heightfield.
    // X and Z are used deliberately: Y is the elevation axis.
    struct TerrainOrigin
    {
        double x{0.0};
        double z{0.0};
    };

} // namespace dharaninirmana::terrain
