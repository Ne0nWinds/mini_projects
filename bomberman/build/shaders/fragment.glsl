#version 330 core

in vec2 tex_coords;
out vec4 color;

uniform sampler2D image;

void main()
{
	color = texture(image, tex_coords);
}
