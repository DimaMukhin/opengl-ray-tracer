// The JSON library allows you to reference JSON arrays like C++ vectors and JSON objects like C++ maps.

#include "raytracer.h"

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
	// TODO: for initial ray, we only want hit points that happen after VP
	return castRay(e, s, colour);
}

bool castRay(const point3 &e, const point3 &s, colour3 &colour) {
	json object;
	glm::vec3 n;
	float t;

	t = intersect(e, s, n, object);

	if (t < 0)
		return false;

	point3 p = e + (s - e) * t;

	colour = light(e, p, n, object["material"]);

	return true;
}

float intersect(point3 e, point3 s, glm::vec3 &normal, json &object) {
	bool hit = false;
	float minT = -1.0f;
	float safeT = 0.001f;

	json &objects = scene["objects"];
	for (json::iterator it = objects.begin(); it != objects.end(); ++it) {
		json &currObject = *it;
		float currT;

		if (currObject["type"] == "sphere") {
			point3 c = vector_to_vec3(currObject["position"]);
			float R = float(currObject["radius"]);

			if (raySphereIntersection(e, s, c, R, currT) && currT > safeT && (currT < minT || !hit)) {
				hit = true;
				minT = currT;
				object = currObject;
				
				// getting sphere normal
				point3 p = e + (s - e) * currT;
				normal = glm::normalize(p - c);

				continue;
			}
		}

		if (currObject["type"] == "plane") {
			point3 a = vector_to_vec3(currObject["position"]);
			glm::vec3 n = glm::normalize(vector_to_vec3(currObject["normal"]));

			if (rayPlaneIntersection(e, s, a, n, currT) && currT > safeT && (currT < minT || !hit)) {
				hit = true;
				minT = currT;
				object = currObject;
				normal = n;
				continue;
			}
		}

		if (currObject["type"] == "mesh") {
			std::vector<std::vector<std::vector<float>>> triangles = currObject["triangles"];

			for (int i = 0; i < triangles.size(); i++) {
				std::vector<std::vector<float>> triangle = triangles[i];
				point3 a = vector_to_vec3(triangle[0]);
				point3 b = vector_to_vec3(triangle[1]);
				point3 c = vector_to_vec3(triangle[2]);
				glm::vec3 n = glm::normalize(glm::cross(b - a, c - b));

				if (rayTriangleIntersection(e, s, a, b, c, n, currT) && currT > safeT && (currT < minT || !hit)) {
					hit = true;
					minT = currT;
					object = currObject;
					normal = n;
					continue;
				}
			}
		}
	}

	return minT;
}

colour3 light(point3 e, point3 p, glm::vec3 n, json material) {
	colour3 color = colour3(0, 0, 0);

	json &lights = scene["lights"];

	n = glm::normalize(n);
	glm::vec3 v = glm::normalize(e - p);

	for (json::iterator it = lights.begin(); it != lights.end(); ++it) {
		json &light = *it;

		if (light["type"] == "ambient") {
			colour3 ia = vector_to_vec3(light["color"]);
			colour3 ka = vector_to_vec3(material["ambient"]);
			color += ia * ka;

			continue;
		}

		if (light["type"] == "directional") {
			glm::vec3 direction = vector_to_vec3(light["direction"]);
			colour3 id = vector_to_vec3(light["color"]);
			glm::vec3 l = glm::normalize(-direction);

			// dont light it if light doesnt reach it
			if (pointInShadow(p, p + (l * 100.0f))) {
				continue;
			}

			// diffuse
			colour3 kd = vector_to_vec3(material["diffuse"]);
			color += id * kd * (glm::dot(n, l));

			// specular if material supports specular component
			if (material.find("specular") != material.end()) {
				colour3 is = vector_to_vec3(light["color"]);
				colour3 ks = vector_to_vec3(material["specular"]);
				float alpha = material["shininess"];
				glm::vec3 h = glm::normalize(l + v);

				float dotP = glm::dot(n, h);
				if (dotP < 0.0f) dotP = 0.0f;
				color += is * ks * std::pow(dotP, alpha);
			}

			continue;
		}

		if (light["type"] == "point") {
			point3 lightPosition = vector_to_vec3(light["position"]);
			glm::vec3 id = vector_to_vec3(light["color"]);
			glm::vec3 l = glm::normalize(lightPosition - p);

			// dont add color if light doesnt reach the point
			if (pointInShadow(p, lightPosition)) {
				continue;
			}

			// diffuse
			colour3 kd = vector_to_vec3(material["diffuse"]);
			color += id * kd * (glm::dot(n, l));

			// specular if material supports specular component
			if (material.find("specular") != material.end()) {
				colour3 is = vector_to_vec3(light["color"]);
				colour3 ks = vector_to_vec3(material["specular"]);
				float alpha = material["shininess"];
				glm::vec3 h = glm::normalize(l + v);

				float dotP = glm::dot(n, h);
				if (dotP < 0.0f) dotP = 0.0f;
				color += is * ks * std::pow(dotP, alpha);
			}

			continue;
		}
	}

	return color;
}

bool pointInShadow(point3 p, point3 l) {
	json object;
	glm::vec3 normal;
	float t = intersect(p, l, normal, object);

	if (t > 0.0f && t < 1.0f)
		return true;

	return false;
}

bool rayTriangleIntersection(point3 e, point3 s, point3 a, point3 b, point3 c, glm::vec3 n, float &t) {
	n = glm::normalize(n);
	float planeT;

	if (rayPlaneIntersection(e, s, a, n, planeT)) {
		glm::vec3 d = s - e;
		point3 x = e + d * planeT;

		float term1 = glm::dot(glm::cross((b - a), (x - a)), n);
		float term2 = glm::dot(glm::cross((c - b), (x - b)), n);
		float term3 = glm::dot(glm::cross((a - c), (x - c)), n);

		if (term1 > 0 && term2 > 0 && term3 > 0) {
			t = planeT;
			return true;
		}
	}

	return false;
}

bool rayPlaneIntersection(point3 e, point3 s, point3 a, glm::vec3 n, float &t) {
	n = glm::normalize(n);
	glm::vec3 d = s - e;

	float denominator = glm::dot(n, d);

	if (denominator != 0) {
		t = glm::dot(n, a - e) / denominator;

		if (t < 0)
			return false;

		return true;
	}

	return false;
}

bool raySphereIntersection(point3 e, point3 s, point3 c, float R, float &t) {
	glm::vec3 d = s - e;

	float discriminant = std::pow(glm::dot(d, e - c), 2.0) - (glm::dot(d, d) * (glm::dot(e - c, e - c) - std::pow(R, 2.0)));
	if (discriminant < 0)
		return false;

	float rootDiscriminant = std::sqrt(discriminant);

	float t1 = (glm::dot(-d, e - c) - rootDiscriminant) / glm::dot(d, d);
	float t2 = (glm::dot(-d, e - c) + rootDiscriminant) / glm::dot(d, d);

	if (t1 < 0 && t2 < 0) {
		return false;
	}
	else if (t1 <= t2 && t1 >= 0) {
		t = t1;
		return true;
	}
	else if (t2 <= t1 && t2 >= 0) {
		float t = t2;
		return true;
	}
	else {
		return false;
	}
}

//bool trace(const point3 &e, const point3 &s, colour3 &colour, bool pick) {
//	// traverse the objects
//	bool hit = false;
//
//	// TODO: think about implementing a better "deapth buffer" we can use find() to do this easily
//	float minT = -1;
//
//	json &objects = scene["objects"];
//	for (json::iterator it = objects.begin(); it != objects.end(); ++it) {
//		json &object = *it;
//		json &material = object["material"];
//		float t;
//
//		if (object["type"] == "sphere") {
//			point3 c = vector_to_vec3(object["position"]);
//			float R = float(object["radius"]);
//
//			if (raySphereIntersection(e, s, c, R, t) && (t < minT || !hit)) {
//				point3 p = e + (s - e) * t;
//
//				// TODO: implement light
//				colour = light(e, p, glm::normalize(p - c), material);
//
//				minT = t;
//				hit = true;
//			}
//		}
//
//		if (object["type"] == "plane") {
//			point3 a = vector_to_vec3(object["position"]);
//			glm::vec3 n = glm::normalize(vector_to_vec3(object["normal"]));
//
//			if (rayPlaneIntersection(e, s, a, n, t) && (t < minT || !hit)) {
//				point3 p = e + (s - e) * t;
//
//				// TODO: implement light
//				colour = light(e, p, n, material);
//
//				minT = t;
//				hit = true;
//			}
//		}
//
//		if (object["type"] == "mesh") {
//			std::vector<std::vector<std::vector<float>>> triangles = object["triangles"];
//
//			for (int i = 0; i < triangles.size(); i++) {
//				std::vector<std::vector<float>> triangle = triangles[i];
//				point3 a = vector_to_vec3(triangle[0]);
//				point3 b = vector_to_vec3(triangle[1]);
//				point3 c = vector_to_vec3(triangle[2]);
//				glm::vec3 n = glm::normalize(glm::cross(b - a, c - b));
//
//				if (rayTriangleIntersection(e, s, a, b, c, n, t) && (t < minT || !hit)) {
//					point3 p = e + (s - e) * t;
//
//					// TODO: implement light
//					colour = light(e, p, n, material);
//
//					minT = t;
//					hit = true;
//				}
//			}
//		}
//	}
//
//	return hit;
//}

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