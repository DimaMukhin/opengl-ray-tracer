#version 150

in vec3 vPos;
uniform vec2 Window;
out float u;

void main()
{
   gl_Position = vec4((vPos.x+0.5) * 2 / Window.x - 1, (vPos.y+0.5) * 2 / Window.y - 1, 0, 1);
   u = vPos.z;
}
