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
	// NOTE: This is a demo, not ray tracing code! You will need to replace all of this with your own code...

	// traverse the objects
	json &objects = scene["objects"];
	for (json::iterator it = objects.begin(); it != objects.end(); ++it) {
		json &object = *it;
		
		// every object in the scene will have a "type"
		if (object["type"] == "sphere") {
			// This is NOT ray-sphere intersection
			// Every sphere will have a position and a radius
			std::vector<float> pos = object["position"];
			point3 p = -(s - e) * pos[2];
			if (glm::length(glm::vec3(p.x - pos[0],p.y - pos[1],0)) < float(object["radius"])) {
				// Every object will have a material
				json &material = object["material"];
				std::vector<float> diffuse = material["diffuse"];
				colour = vector_to_vec3(diffuse);
				// This is NOT correct: it finds the first hit, not the closest
				return true;
			}
		}
	}

	return false;
}