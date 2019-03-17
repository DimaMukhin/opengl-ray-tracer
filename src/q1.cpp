// Assignment 2 Question 1

#define _USE_MATH_DEFINES
#include <cmath>

#include "common.h"
#include "raytracer.h"

#include <iostream>

#include <glm/glm.hpp>

const char *WINDOW_TITLE = "Ray Tracing";
const double FRAME_RATE_MS = 1;

float xs[1<<16]; // big enough for a row of pixels
colour3 colours[1<<16];
GLuint Y, Window;
int vp_width, vp_height;
float drawing_y = 0;

point3 eye;
float d = 1;

//----------------------------------------------------------------------------

point3 s(int x, int y) {
	float aspect_ratio = (float)vp_width / vp_height;
	float h = d * (float)tan((M_PI * fov) / 180.0 / 2.0);
	float w = h * aspect_ratio;
   
	float top = h;
	float bottom = -h;
	float left = -w;
	float right = w;
   
	float u = left + (right - left) * (x + 0.5f) / vp_width;
	float v = bottom + (top - bottom) * (y + 0.5f) / vp_height;
   
	return point3(u, v, -d);
}

//----------------------------------------------------------------------------

// OpenGL initialization
void init(char *fn) {
	choose_scene(fn);
   
	// Create a vertex array object
	GLuint vao;
	glGenVertexArrays( 1, &vao );
	glBindVertexArray( vao );

	// Create and initialize a buffer object
	GLuint buffer;
	glGenBuffers( 1, &buffer );
	glBindBuffer( GL_ARRAY_BUFFER, buffer );
	glBufferData( GL_ARRAY_BUFFER, sizeof(xs) + sizeof(colours), NULL, GL_STATIC_DRAW );

	// Load shaders and use the resulting shader program
	GLuint program = InitShader( "v.glsl", "f.glsl" );
	glUseProgram( program );

	// set up vertex arrays
	GLuint vColour = glGetAttribLocation( program, "vColour" );
	glEnableVertexAttribArray( vColour );
	glVertexAttribPointer( vColour, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0) );

	GLuint xPos = glGetAttribLocation( program, "xPos" );
	glEnableVertexAttribArray( xPos );
	glVertexAttribPointer( xPos, 1, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(colours)) );

	Y = glGetUniformLocation(program, "Y");
	Window = glGetUniformLocation(program, "Window");

	// glClearColor( background_colour[0], background_colour[1], background_colour[2], 1 );
	glClearColor( 0.7, 0.7, 0.8, 1 );
    
	for (int x = 0; x < sizeof(xs)/sizeof(float); x++) {
		xs[x] = x;
	}
	glBufferSubData( GL_ARRAY_BUFFER, sizeof(colours), sizeof(xs), xs);
}

//----------------------------------------------------------------------------

void display( void ) {
	// draw one scanline at a time, to each buffer; only clear when we draw the first scanline
	// (when fract(drawing_y) == 0.0, draw one buffer, when it is 0.5 draw the other)
	
	if (drawing_y <= 0.5) {
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

		glFlush();
		glFinish();
		glutSwapBuffers();

		drawing_y += 0.5;

	} else if (drawing_y >= 1.0 && drawing_y <= vp_height + 0.5) {
		int y = int(drawing_y) - 1;

		// only recalculate if this is a new scanline
		if (drawing_y == int(drawing_y)) {

			// this would be better with a single-pixel quad and a texture
			for (int x = 0; x < vp_width; x++) {
				if (!trace(eye, s(x, y), colours[x], false)) {
					colours[x] = background_colour;
				}
			}

			glBufferSubData( GL_ARRAY_BUFFER, 0, vp_width * sizeof(colour3), colours);
		}

		glUniform1f( Y, y );
		glDrawArrays( GL_POINTS, 0, vp_width );
		
		glFlush();
		glFinish();
		glutSwapBuffers();
		
		drawing_y += 0.5;
	}
}

//----------------------------------------------------------------------------

void keyboard( unsigned char key, int x, int y ) {
	switch( key ) {
	case 033: // Escape Key
	case 'q': case 'Q':
		exit( EXIT_SUCCESS );
		break;
	case ' ':
		drawing_y = 1;
		break;
	}
}

//----------------------------------------------------------------------------

void mouse( int button, int state, int x, int y ) {
	y = vp_height - y - 1;
	if ( state == GLUT_DOWN ) {
		switch( button ) {
			case GLUT_LEFT_BUTTON:
			colour3 c;
			point3 uvw = s(x, y);
			std::cout << std::endl;
			if (trace(eye, uvw, c, true)) {
				std::cout << "HIT @ ( " << uvw.x << "," << uvw.y << "," << uvw.z << " )\n";
				std::cout << "      colour = ( " << c.r << "," << c.g << "," << c.b << " )\n";
			} else {
				std::cout << "MISS @ ( " << uvw.x << "," << uvw.y << "," << uvw.z << " )\n";
			}
			break;
		}
	}
}

//----------------------------------------------------------------------------

void update( void ) {
}

//----------------------------------------------------------------------------

void reshape( int width, int height ) {
	glViewport( 0, 0, width, height );

	// GLfloat aspect = GLfloat(width)/height;
	// glm::mat4  projection = glm::ortho( -aspect, aspect, -1.0f, 1.0f, -1.0f, 1.0f );
	// glUniformMatrix4fv( Projection, 1, GL_FALSE, glm::value_ptr(projection) );
	vp_width = width;
	vp_height = height;
	glUniform2f( Window, width, height );
	drawing_y = 0;
}
