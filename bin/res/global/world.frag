#version 330

uniform sampler2D texture;
uniform float camOwnerHP;
uniform float rand;

out vec4 FragColor;

float lerp(in float start, in float end, in float t)
{
	return start + (end - start) * t;
}

void main()
{
	ivec2 texSize = textureSize(texture, 0);
	vec2 pos = gl_FragCoord.xy;
	vec4 pixel = texture2D(texture, pos / texSize);
	float light = (1.0 - distance(pos, vec2(200)) / 1000);
	float greyscale = (pixel.r + pixel.g + pixel.b) / 3;
	if (camOwnerHP > 75)
	{
		FragColor = pixel;
	}
	else if (camOwnerHP > 50)
	{
		float t = (75 - camOwnerHP) / 25;
		FragColor = pixel + vec4(lerp(0, greyscale - pixel.r, t), lerp(0, greyscale - pixel.g, t), lerp(0, greyscale - pixel.b, t), 0);
	}
	else if (camOwnerHP > 25)
	{
		float chance = (50 - camOwnerHP) / 25;
		if (rand > chance)
		{
			FragColor = clamp(vec4(greyscale) + noise1(rand), vec4(0.1), vec4(1));
		}
	}
	else if (camOwnerHP > 0)
	{
		float brightness = 1.0 - (25 - camOwnerHP) / 25;
		FragColor = vec4(greyscale) * clamp(brightness, 0, 0.5);
	}
	FragColor *= light;
	FragColor.a = 1;
}