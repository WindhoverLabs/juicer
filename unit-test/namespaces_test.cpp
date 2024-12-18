namespace World
{
struct Shape
{
    int width;
    int length;
};
}  // namespace World

class WorldClass
{
   public:
    struct Shape
    {
        int width;
        int length;
    };
};  // namespace World

World::Shape      myShape{};
WorldClass::Shape classyShape{};
