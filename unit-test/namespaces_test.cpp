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

namespace _4D
{
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
// Universe::Mars::Shape  mars{};

// Plane::_3D::Shape      Space{};

Plane::_4D::Universe::Shape Star{};
