namespace Universe
{
namespace World
{
struct Shape
{
    int width;
    int length;
};
}  // namespace World
}  // namespace Universe

namespace Universe
{

struct Shape2
{
    int width;
    int length;
};
}  // namespace Universe

class WorldClass
{
   public:
    struct Shape
    {
        int width;
        int length;
    };
};  // namespace World

Universe::World::Shape myShape{};

Universe::Shape2       myShape2{};
WorldClass::Shape      classyShape{};
