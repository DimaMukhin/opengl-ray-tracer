/*
	Assignment 3
	By Dima Mukhin 7773184
*/

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

// clamp a vector between 0,0,0 and 1,1,1
glm::vec3 clamp(glm::vec3 vector) {
	return glm::clamp(vector, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f));
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
	return castRay(e, s, colour, -1.0, 8);
}

/*
cast a ray in the scene
	e: start point of the ray
	s: intersection point of the ray
	colour: output colour of casted ray (supports all features)
	ni: current index of refraction (if -1.0, ni is by default 1.0)
	iterations: # of iterations left (stop recursion when iterations = 0)
*/
bool castRay(const point3 &e, const point3 &s, colour3 &colour, float ni, int iterations) {
	if (iterations == 0) {
		return false;
	}

	json object;
	glm::vec3 n;
	float t;

	// find closest intersection in the scene and get the normal and object at intersection
	t = intersect(e, s, n, object);

	if (t < 0)
		return false;

	point3 p = e + (s - e) * t;
	json material = object["material"];

	// light the point of intersection
	colour = light(e, p, n, material);
	colour = clamp(colour);

	// if object has a reflative property, reflect the ray and update color
	if (material.find("reflective") != material.end()) {
		reflect(e, p, n, vector_to_vec3(material["reflective"]), colour, iterations);
	}

	// if object has transmissive property...
	if (material.find("transmissive") != material.end()) {
		glm::vec3 kt = vector_to_vec3(material["transmissive"]);

		// if object has refraction proprty, refract the ray. otherwise, perform simple transparent ray
		if (material.find("refraction") != material.end()) {
			refract(p, e, s, n, colour, ni, kt, material["refraction"], iterations);
		}
		else {
			transparentRay(p, s - e, colour, kt, iterations);
		}
	}

	return true;
}

/*
find closest intersection in the scene, return -1 if no intersection
	e: origin of ray
	s: intersection of ray
	normal: output normal at intersection
	object: output object at intersection
*/
float intersect(point3 e, point3 s, glm::vec3 &normal, json &object) {
	bool hit = false;
	float minT = -1.0f;
	float safeT = 0.001f;

	// for all objects in the scene
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

/*
light ponit of intersection
	e: origin of ray (eye position initially)
	p: point of intersection
	n: normal at point of intersection
	material: material properties of object at point of intersection
*/
colour3 light(point3 e, point3 p, glm::vec3 n, json material) {
	colour3 color = colour3(0, 0, 0);
	json &lights = scene["lights"];

	n = glm::normalize(n);
	glm::vec3 v = glm::normalize(e - p);

	// for all lights in the scene...
	for (json::iterator it = lights.begin(); it != lights.end(); ++it) {
		json &light = *it;

		if (light["type"] == "ambient" && material.find("diffuse") != material.end()) {
			colour3 ia = vector_to_vec3(light["color"]);
			colour3 ka = vector_to_vec3(material["ambient"]);
			color += ia * ka;
			color = clamp(color);

			continue;
		}

		if (light["type"] == "directional") {
			glm::vec3 direction = vector_to_vec3(light["direction"]);
			glm::vec3 l = glm::normalize(-direction);

			// dont light it if light doesnt reach it
			if (pointInShadow(p, p + (l * 100.0f))) {
				continue;
			}

			// diffuse
			if (material.find("diffuse") != material.end()) {
				colour3 id = vector_to_vec3(light["color"]);
				colour3 kd = vector_to_vec3(material["diffuse"]);
				float dotP = glm::dot(n, l);
				if (dotP < 0.0f) dotP = 0.0f;
				color += id * kd * dotP;
				color = clamp(color);
			}

			// specular if material supports specular component
			if (material.find("specular") != material.end()) {
				colour3 is = vector_to_vec3(light["color"]);
				colour3 ks = vector_to_vec3(material["specular"]);
				float alpha = material["shininess"];
				glm::vec3 h = glm::normalize(l + v);

				float dotP = glm::dot(n, h);
				if (dotP < 0.0f) dotP = 0.0f;
				color += is * ks * std::pow(dotP, alpha);
				color = clamp(color);
			}

			continue;
		}

		if (light["type"] == "point") {
			point3 lightPosition = vector_to_vec3(light["position"]);
			glm::vec3 l = glm::normalize(lightPosition - p);

			// dont add color if light doesnt reach the point
			if (pointInShadow(p, lightPosition)) {
				continue;
			}

			// diffuse
			if (material.find("diffuse") != material.end()) {
				glm::vec3 id = vector_to_vec3(light["color"]);
				colour3 kd = vector_to_vec3(material["diffuse"]);
				float dotP = glm::dot(n, l);
				if (dotP < 0.0f) dotP = 0.0f;
				color += id * kd * dotP;
				color = clamp(color);
			}

			// specular if material supports specular component
			if (material.find("specular") != material.end()) {
				colour3 is = vector_to_vec3(light["color"]);
				colour3 ks = vector_to_vec3(material["specular"]);
				float alpha = material["shininess"];
				glm::vec3 h = glm::normalize(l + v);

				float dotP = glm::dot(n, h);
				if (dotP < 0.0f) dotP = 0.0f;
				color += is * ks * std::pow(dotP, alpha);
				color = clamp(color);
			}

			continue;
		}

		if (light["type"] == "spot") {
			point3 lightPosition = vector_to_vec3(light["position"]);
			glm::vec3 direction = glm::normalize(vector_to_vec3(light["direction"]));
			glm::vec3 l = glm::normalize(lightPosition - p);
			float cutoff = light["cutoff"];

			// dont calculate light if point is outside the cutoff angle
			if (glm::dot(l, -direction) < glm::cos(glm::radians(cutoff))) {
				continue;
			}

			// dont add color if light doesnt reach the point
			if (pointInShadow(p, lightPosition)) {
				continue;
			}

			// diffuse
			if (material.find("diffuse") != material.end()) {
				glm::vec3 id = vector_to_vec3(light["color"]);
				colour3 kd = vector_to_vec3(material["diffuse"]);
				float dotP = glm::dot(n, l);
				if (dotP < 0.0f) dotP = 0.0f;
				color += id * kd * dotP;
				color = clamp(color);
			}
			
			// specular if material supports specular component
			if (material.find("specular") != material.end()) {
				colour3 is = vector_to_vec3(light["color"]);
				colour3 ks = vector_to_vec3(material["specular"]);
				float alpha = material["shininess"];
				glm::vec3 h = glm::normalize(l + v);

				float dotP = glm::dot(n, h);
				if (dotP < 0.0f) dotP = 0.0f;
				color += is * ks * std::pow(dotP, alpha);
				color = clamp(color);
			}

			continue;
		}
	}

	return color;
}

/*
Point in shadow test
	p: the point to test
	l: light position
*/
bool pointInShadow(point3 p, point3 l) {
	json object;
	glm::vec3 normal;
	float t = intersect(p, l, normal, object);

	if (t > 0.0f && t < 1.0f)
		return true;

	return false;
}

/*
Reflect the ray and find colour (recursive)
	e: origin point of ray
	p: point of intersection
	n: normal at point of intersection
	km: mirror constant property of object
	colour: output color
	interations: # of recursive iterations left
*/
void reflect(point3 e, point3 p, glm::vec3 n, glm::vec3 km, colour3 &colour, int iterations) {
	glm::vec3 v = glm::normalize(e - p);
	glm::vec3 r = glm::normalize(2 * glm::dot(n, v) * n - v);
	colour3 hitColor;

	if (castRay(p, p + r, hitColor, -1.0, iterations - 1)) {
		hitColor = clamp(hitColor);
		colour += hitColor * km;
		colour = clamp(colour);
	}
	else {
		colour += background_colour * km;
		colour = clamp(colour);
	}
}

/*
cast transparent ray and update color
	p: point of intersection
	d: direction of transparent ray
	colour: output color
	kt: transparent constant for material
	iteractions: # of iterations left
*/
void transparentRay(point3 p, point3 d, colour3 &colour, glm::vec3 kt, int iterations) {
	colour3 hitColor;

	if (castRay(p, p + d, hitColor, -1.0, iterations - 1)) {
		hitColor = clamp(hitColor);
		colour = (1.0f - kt) * colour + hitColor * kt;
		colour = clamp(colour);
	}
	else {
		colour = (1.0f - kt) * colour + background_colour * kt;
		colour = clamp(colour);
	}
}

/*
reflect ray and update color
	p: point of intersection
	e: eye position
	s: point of intersection of ray
	n: normal at point of intersection
	colour: output color
	ni: current index of refraction
	kt: transparency constant for material
	materialNR: material's index of refraction
	iterations: # of iterations left
*/
void refract(point3 p, point3 e, point3 s, glm::vec3 n, colour3 &colour, float ni, glm::vec3 kt, float materialNR, int iterations) {
	colour3 hitColor;

	if (ni > 0) {
		float nr = 1.0f;

		glm::vec3 vi = glm::normalize(s - e);
		glm::vec3 N = glm::normalize(-n);

		glm::vec3 vr = ((ni * (vi - N * glm::dot(vi, N))) / nr) - (N * std::sqrt(1 - ((glm::pow(ni, 2) * (1 - std::pow(glm::dot(vi, N), 2))) / std::pow(nr, 2))));

		if (castRay(p, p + vr, hitColor, -1.0f, iterations - 1)) {
			hitColor = clamp(hitColor);
			colour = (1.0f - kt) * colour + hitColor * kt;
			colour = clamp(colour);
		}
		else {
			colour = (1.0f - kt) * colour + background_colour * kt;
			colour = clamp(colour);
		}
	}
	else {
		float ni = 1.0f;
		float nr = materialNR;

		glm::vec3 vi = glm::normalize(s - e);
		glm::vec3 N = glm::normalize(n);

		if (glm::dot(vi, N) <= 1 - (std::pow(ni, 2) / std::pow(nr, 2))) {
			glm::vec3 vr = ((ni * (vi - N * glm::dot(vi, N))) / nr) - (N * std::sqrt(1 - ((glm::pow(ni, 2) * (1 - std::pow(glm::dot(vi, N), 2))) / std::pow(nr, 2))));

			if (castRay(p, p + vr, hitColor, nr, iterations - 1)) {
				hitColor = clamp(hitColor);
				colour = (1.0f - kt) * colour + hitColor * kt;
				colour = clamp(colour);
			}
			else {
				colour = (1.0f - kt) * colour + background_colour * kt;
				colour = clamp(colour);
			}
		}
		else {
			reflect(e, p, n, glm::vec3(1.0f, 1.0f, 1.0f), colour, iterations - 1);
		}
	}
}

/*
check for ray triangle itersection
	e: eye position
	s: ray intersection position
	a: point a of triangle
	b: point b of triangle
	c: point c of triangle
	n: normal of triangle
	t: output t value for intersection
*/
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

/*
ray plane intersection test
	e: eye position
	s: ray intersection position
	a: plane position
	n: plane normal
	t: output t value for point of intersection
*/
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

/*
ray sphere intersection test
	e: eye position
	s: ray intersection position
	c: sphere center point
	R: sphere radius length
	t: point of intersection t value
*/
bool raySphereIntersection(point3 e, point3 s, point3 c, float R, float &t) {
	glm::vec3 d = s - e;
	float safeT = 0.001f;

	float discriminant = std::pow(glm::dot(d, e - c), 2.0) - (glm::dot(d, d) * (glm::dot(e - c, e - c) - std::pow(R, 2.0)));
	if (discriminant < 0)
		return false;

	float rootDiscriminant = std::sqrt(discriminant);

	float t1 = (glm::dot(-d, e - c) - rootDiscriminant) / glm::dot(d, d);
	float t2 = (glm::dot(-d, e - c) + rootDiscriminant) / glm::dot(d, d);

	if (t1 < safeT && t2 < safeT) {
		return false;
	}
	else if ((t1 <= t2 && t1 >= safeT) || (t2 < safeT && t1 >= safeT)) {
		t = t1;
		return true;
	}
	else if ((t2 <= t1 && t2 >= safeT) || (t1 < safeT && t2 >= safeT)) {
		t = t2;
		return true;
	}
	else {
		return false;
	}
}
