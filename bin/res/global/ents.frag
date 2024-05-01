#version 330 core

uniform sampler2D texture;

out vec4 FragColor;

void main()
{
	FragColor = texture2D(texture, gl_FragCoord.xy / textureSize(texture, 0));
}