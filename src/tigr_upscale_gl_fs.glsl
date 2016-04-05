#version 330 core

in vec2 uv;

out vec4 color;

uniform sampler2D image;
uniform vec4 parameters;

void main()
{
	vec2 tex_size = textureSize(image, 0);
	vec2 uv_blur = mix(floor(uv * tex_size) + 0.5, uv * tex_size, parameters.xy) / tex_size;
	vec4 c = texture(image, uv_blur);
	c.rgb *= mix(0.5, 1.0 - fract(uv.y * tex_size.y), parameters.z) * 2.0; //scanline
	c = mix(vec4(0.5), c, parameters.w); //contrast 
	color = c;
}
