#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "dharaninirmana/terrain/height_field.hpp"

namespace
{

    int failures = 0;

    void check(bool condition, const char *description)
    {
        if (!condition)
        {
            std::cerr << "[FAIL] " << description << '\n';
            ++failures;
        }
        else
        {
            std::cout << "[PASS] " << description << '\n';
        }
    }

    using dharaninirmana::terrain::HeightField;
    using dharaninirmana::terrain::TerrainOrigin;

    HeightField makeField()
    {
        auto result = HeightField::create(
            3, 2, 2.0, 5.0, TerrainOrigin{10.0, -4.0},
            std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0, 6.0});
        if (!result)
        {
            std::cerr << "Unexpected fixture creation failure: "
                      << result.error().message() << '\n';
            std::exit(EXIT_FAILURE);
        }
        return std::move(result).value();
    }

    void testValidConstructionAndMetadata()
    {
        auto field = makeField();
        check(field.width() == 3, "width metadata");
        check(field.depth() == 2, "depth metadata");
        check(field.spacingX() == 2.0, "X spacing metadata");
        check(field.spacingZ() == 5.0, "Z spacing metadata");
        check(field.origin().x == 10.0 && field.origin().z == -4.0,
              "TerrainOrigin metadata");
        check(field.sampleCount() == 6, "sample count metadata");
        check(field.heights().size() == 6, "read-only contiguous height view");
    }

    void testConstructionValidation()
    {
        auto tooNarrow = HeightField::create(
            1, 2, 1.0, 1.0, {}, std::vector<double>{1.0, 2.0});
        check(!tooNarrow, "reject width below two");

        auto tooShallow = HeightField::create(
            2, 1, 1.0, 1.0, {}, std::vector<double>{1.0, 2.0});
        check(!tooShallow, "reject depth below two");

        auto overflow = HeightField::create(
            std::numeric_limits<std::size_t>::max(), 2, 1.0, 1.0, {}, {});
        check(!overflow, "reject width-depth multiplication overflow");

        auto wrongCount = HeightField::create(
            2, 2, 1.0, 1.0, {}, std::vector<double>{1.0, 2.0, 3.0});
        check(!wrongCount, "reject mismatched elevation storage size");

        auto zeroSpacing = HeightField::create(
            2, 2, 0.0, 1.0, {}, std::vector<double>(4, 0.0));
        check(!zeroSpacing, "reject zero spacing");

        auto infiniteSpacing = HeightField::create(
            2, 2, 1.0, std::numeric_limits<double>::infinity(), {},
            std::vector<double>(4, 0.0));
        check(!infiniteSpacing, "reject infinite spacing");

        auto invalidOrigin = HeightField::create(
            2, 2, 1.0, 1.0,
            TerrainOrigin{std::numeric_limits<double>::quiet_NaN(), 0.0},
            std::vector<double>(4, 0.0));
        check(!invalidOrigin, "reject non-finite origin");

        auto invalidHeight = HeightField::create(
            2, 2, 1.0, 1.0, {},
            std::vector<double>{0.0, 1.0,
                                std::numeric_limits<double>::infinity(), 3.0});
        check(!invalidHeight, "reject non-finite elevation");

        auto extentOverflow = HeightField::create(
            2, 2, std::numeric_limits<double>::max(), 1.0,
            TerrainOrigin{std::numeric_limits<double>::max(), 0.0},
            std::vector<double>(4, 0.0));
        check(!extentOverflow, "reject non-finite last-sample spatial extent");
    }

    void testRowMajorAccessAndBounds()
    {
        auto field = makeField();

        auto first = field.heightAt(0, 0);
        auto interior = field.heightAt(2, 0);
        auto last = field.heightAt(2, 1);
        check(first && first.value() == 1.0, "read first row-major sample");
        check(interior && interior.value() == 3.0, "read X-axis boundary sample");
        check(last && last.value() == 6.0, "read last row-major sample");
        check(!field.heightAt(3, 0), "reject out-of-range X");
        check(!field.heightAt(0, 2), "reject out-of-range Z");
    }

    void testMutation()
    {
        auto field = makeField();

        auto changed = field.setHeight(1, 1, 42.5);
        auto readBack = field.heightAt(1, 1);
        check(changed && readBack && readBack.value() == 42.5,
              "successful mutation updates exactly the selected sample");

        auto invalidCoordinate = field.setHeight(3, 1, 9.0);
        check(!invalidCoordinate, "reject out-of-range mutation");

        auto invalidValue = field.setHeight(
            0, 0, std::numeric_limits<double>::quiet_NaN());
        auto unchanged = field.heightAt(0, 0);
        check(!invalidValue, "reject non-finite mutation value");
        check(unchanged && unchanged.value() == 1.0,
              "invalid mutation leaves original sample unchanged");
    }

    void testSpatialMapping()
    {
        auto field = makeField();
        auto position = field.samplePosition(2, 1);

        check(static_cast<bool>(position), "sample position succeeds for valid coordinates");
        if (position)
        {
            const auto &p = position.value();
            check(p.x == 14.0, "sample position maps X using origin and X spacing");
            check(p.y == 6.0, "sample position maps elevation to Y");
            check(p.z == 1.0, "sample position maps Z using origin and Z spacing");
        }

        check(!field.samplePosition(3, 1),
              "sample position rejects out-of-range coordinates");
    }

    void testCopyAndMoveOwnership()
    {
        auto original = makeField();
        auto copied = original;
        auto changed = copied.setHeight(0, 0, 99.0);
        auto originalValue = original.heightAt(0, 0);
        auto copiedValue = copied.heightAt(0, 0);

        check(changed && originalValue && copiedValue &&
                  originalValue.value() == 1.0 && copiedValue.value() == 99.0,
              "copy has independent elevation storage");

        auto moved = std::move(copied);
        auto movedValue = moved.heightAt(0, 0);
        check(movedValue && movedValue.value() == 99.0,
              "move transfers elevation storage and metadata");
    }

} // namespace

int main()
{
    testValidConstructionAndMetadata();
    testConstructionValidation();
    testRowMajorAccessAndBounds();
    testMutation();
    testSpatialMapping();
    testCopyAndMoveOwnership();

    if (failures != 0)
    {
        std::cerr << failures << " terrain test(s) failed.\n";
        return EXIT_FAILURE;
    }

    std::cout << "All Terrain tests passed.\n";
    return EXIT_SUCCESS;
}
