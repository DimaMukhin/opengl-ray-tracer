#pragma once

// The JSON library allows you to reference JSON arrays like C++ vectors and JSON objects like C++ maps.

#include <glm/glm.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <glm/gtx/string_cast.hpp>

#include "json.hpp"

using json = nlohmann::json;

typedef glm::vec3 point3;
typedef glm::vec3 colour3;

extern double fov;
extern colour3 background_colour;

void choose_scene(char const *fn);
bool trace(const point3 &e, const point3 &s, colour3 &colour, bool pick);
bool castRay(const point3 &e, const point3 &s, colour3 &colour);
float intersect(point3 e, point3 s, glm::vec3 &normal, json &object);
colour3 light(point3 e, point3 p, glm::vec3 n, json material);
bool rayTriangleIntersection(point3 e, point3 s, point3 a, point3 b, point3 c, glm::vec3 n, float &t);
bool rayPlaneIntersection(point3 e, point3 s, point3 a, glm::vec3 n, float &t);
bool raySphereIntersection(point3 e, point3 s, point3 c, float R, float &t);
