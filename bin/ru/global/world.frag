#version 330

uniform sampler2D texture;
uniform float camOwnerHP;
uniform float rand;
uniform sampler2D lightMap;

out vec4 FragColor;

float lerp(in float start, in float end, in float t)
{
	return start + (end - start) * t;
}

void main()
{
	ivec2 texSize = textureSize(texture, 0);
	vec2 pos = vec2(gl_FragCoord.x, texSize.y - gl_FragCoord.y);
	vec4 pixel = texture2D(texture, pos / texSize);
	vec4 light = texture2D(lightMap, pos / texSize);
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
			vec4 blackout = pixel + vec4(lerp(0, greyscale - pixel.r, 1.0), lerp(0, greyscale - pixel.g, 1.0), lerp(0, greyscale - pixel.b, 1.0), 0);
			FragColor = clamp(blackout + noise4(pixel), vec4(0.1), vec4(1));
		}
	}
	else if (camOwnerHP > 0)
	{
		float brightness = 1.0 - (25 - camOwnerHP) / 25;
		vec4 blackout = pixel + vec4(lerp(0, greyscale - pixel.r, 1.0), lerp(0, greyscale - pixel.g, 1.0), lerp(0, greyscale - pixel.b, 1.0), 0);
		FragColor = blackout * clamp(brightness, 0.5, 0.75);
	}
	FragColor *= light;
}