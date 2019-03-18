// The JSON library allows you to reference JSON arrays like C++ vectors and JSON objects like C++ maps.

#include "raytracer.h"

#include <iostream>
#include <fstream>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

#include "json.hpp"

using json = nlohmann::json;

const char *PATH = "scenes/";

double fov = 60;
colour3 background_colour(0, 0, 0);

json scene;

/****************************************************************************/

// here are some potentially useful utility functions

json find(json &j, const std::string key, const std::string value) {
	json::iterator it;
	for (it = j.begin(); it != j.end(); ++it) {
		if (it->find(key) != it->end()) {
			if ((*it)[key] == value) {
				return *it;
			}
		}
	}
	return json();
}

glm::vec3 vector_to_vec3(const std::vector<float> &v) {
	return glm::vec3(v[0], v[1], v[2]);
}

/****************************************************************************/

void choose_scene(char const *fn) {
	if (fn == NULL) {
		std::cout << "Using default input file " << PATH << "c.json\n";
		fn = "c";
	}
	
	std::string fname = PATH + std::string(fn) + ".json";
	std::fstream in(fname);
	if (!in.is_open()) {
		std::cout << "Unable to open scene file " << fname << std::endl;
		exit(EXIT_FAILURE);
	}
	
	in >> scene;
	
	json camera = scene["camera"];
	// these are optional parameters (otherwise they default to the values initialized earlier)
	if (camera.find("field") != camera.end()) {
		fov = camera["field"];
		std::cout << "Setting fov to " << fov << " degrees.\n";
	}
	if (camera.find("background") != camera.end()) {
		background_colour = vector_to_vec3(camera["background"]);
		std::cout << "Setting background colour to " << glm::to_string(background_colour) << std::endl;
	}
}

bool trace(const point3 &e, const point3 &s, colour3 &colour, bool pick) {
	// traverse the objects
	json &objects = scene["objects"];
	for (json::iterator it = objects.begin(); it != objects.end(); ++it) {
		json &object = *it;
		json &material = object["material"];
		point3 p;

		// TODO: implement "depth buffer" because right now you just take first hit instead of closest hit

		if (object["type"] == "sphere") {
			point3 c = vector_to_vec3(object["position"]);
			float R = float(object["radius"]);

			if (raySphereIntersection(e, s, c, R, p)) {
				glm::vec3 kd = vector_to_vec3(material["diffuse"]);

				// TODO: implement light
				colour = kd;
				return true;
			}
		}

		if (object["type"] == "plane") {
			point3 a = vector_to_vec3(object["position"]);
			glm::vec3 n = vector_to_vec3(object["normal"]);

			if (rayPlaneIntersection(e, s, a, n, p)) {
				glm::vec3 kd = vector_to_vec3(material["diffuse"]);

				// TODO: implement light
				colour = kd;
				return true;
			}
		}
	}

	return false;
}

bool rayTriangleIntersection(point3 e, point3 s, point3 a, point3 b, point3 c, glm::vec3 n, point3 &p) {

}

bool rayPlaneIntersection(point3 e, point3 s, point3 a, glm::vec3 n, point3 &p) {
	glm::vec3 d = s - e;

	float denominator = glm::dot(n, d);

	if (denominator != 0) {
		float t = glm::dot(n, a - e) / denominator;
		p = e + d * t;

		if (t < 1)
			return false;

		return true;
	}

	return false;
}

bool raySphereIntersection(point3 e, point3 s, point3 c, float R, point3 &p) {
	glm::vec3 d = s - e;

	float discriminant = std::pow(glm::dot(d, e - c), 2.0) - (glm::dot(d, d) * (glm::dot(e - c, e - c) - std::pow(R, 2.0)));
	if (discriminant < 0)
		return false;

	float rootDiscriminant = std::sqrt(discriminant);

	float t1 = (glm::dot(-d, e - c) - rootDiscriminant) / glm::dot(d, d);
	float t2 = (glm::dot(-d, e - c) + rootDiscriminant) / glm::dot(d, d);

	if (t1 < 1 && t2 < 1) {
		return false;
	}
	else if (t1 <= t2 && t1 >= 1) {
		p = e + d * t1;
		return true;
	}
	else if (t2 <= t1 && t2 >= 1) {
		p = e + d * t2;
	}
	else {
		return false;
	}
}

//bool trace(const point3 &e, const point3 &s, colour3 &colour, bool pick) {
//	// NOTE: This is a demo, not ray tracing code! You will need to replace all of this with your own code...
//
//	// traverse the objects
//	json &objects = scene["objects"];
//	for (json::iterator it = objects.begin(); it != objects.end(); ++it) {
//		json &object = *it;
//
//		// every object in the scene will have a "type"
//		if (object["type"] == "sphere") {
//			// This is NOT ray-sphere intersection
//			// Every sphere will have a position and a radius
//			std::vector<float> pos = object["position"];
//			point3 p = -(s - e) * pos[2];
//			if (glm::length(glm::vec3(p.x - pos[0], p.y - pos[1], 0)) < float(object["radius"])) {
//				// Every object will have a material
//				json &material = object["material"];
//				std::vector<float> diffuse = material["diffuse"];
//				colour = vector_to_vec3(diffuse);
//				// This is NOT correct: it finds the first hit, not the closest
//				return true;
//			}
//		}
//	}
//
//	return false;
//}