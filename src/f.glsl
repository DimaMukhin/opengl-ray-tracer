#version 150

in float u;
out vec4 out_colour;
uniform sampler1D tex_sampler;

void main() 
{ 
  out_colour.rgb = texture( tex_sampler, u ).rgb;
  out_colour.a = 1;
}
