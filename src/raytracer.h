#include <glm/glm.hpp>

typedef glm::vec3 point3;
typedef glm::vec3 colour3;

extern double fov;
extern colour3 background_colour;

void choose_scene(char const *fn);
bool trace(const point3 &e, const point3 &s, colour3 &colour, bool pick);
bool rayPlaneIntersection(point3 e, point3 s, point3 a, glm::vec3 n, point3 &p);
bool raySphereIntersection(point3 e, point3 s, point3 c, float R, point3 &p);
