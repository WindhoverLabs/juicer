#include "stdint.h"
/**
 *The fields padding1 and padding2(as the name implies) are to prevent
 *gcc from inserting padding at compile-time and altering the expected results in our tests.
 * Tested on Ubuntu 20.04 and Ubuntu 18.04.
 */
typedef struct
{
    int32_t  width = 101;
    uint16_t stuff;
    uint16_t padding1;
    int32_t  length;
    uint16_t more_stuff;
    uint16_t padding2;
    float    floating_stuff;
    float    matrix3D[2][4][4];
    float    matrix1D[2];
    uint8_t  extra;
} Square;

namespace Universe
{
namespace Earth
{
struct Shape
{
    int width;
    int length;
};

}  // namespace Earth

namespace Mars
{
struct Shape
{
    int width;
    int length;
};
}  // namespace Mars
}  // namespace Universe

namespace Plane
{
namespace _3D
{
struct Shape
{
    int width;
    int length;
};
}  // namespace _3D

namespace _2D
{
typedef struct
{
    int32_t  width = 101;
    uint16_t stuff;
    uint16_t padding1;
    int32_t  length;
    uint16_t more_stuff;
    uint16_t padding2;
    float    floating_stuff;
    float    matrix3D[2][4][4];
    float    matrix1D[2];
    uint8_t  extra;
} Square;
}  // namespace _2D

namespace _4D
{

Square s3[6];
namespace Universe
{
struct Shape
{
    int width;
    int length;
};
}  // namespace Universe
}  // namespace _4D

}  // namespace Plane
Universe::Earth::Shape      earth{};
Universe::Mars::Shape       mars{};

Plane::_3D::Shape           Space{};

Plane::_4D::Universe::Shape Star{};

Square                      s{};

Plane::_2D::Square          s2{};