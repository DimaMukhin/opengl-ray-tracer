#version 150

in vec3 colour;
out vec4 out_colour;

void main() 
{ 
  out_colour.rgb = colour;
  out_colour.a = 1;
}
