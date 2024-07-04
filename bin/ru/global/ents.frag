#version 330

uniform sampler2D render;
uniform sampler2D lightMap;
uniform float camOwnerHP;
uniform vec4 camOwnerRect;
uniform vec4 camOwnerBones[100];
uniform float camOwnerBonesRot[100];
uniform int camOwnerBoneCount;
uniform float rand;

out vec4 FragColor;

float lerp(in float start, in float end, in float t)
{
	return start + (end - start) * t;
}

bool PointAABB(in vec2 point, in vec4 box)
{
	return point.x >= box.x && point.x <= box.x + box.z && point.y >= box.y && point.y <= box.y + box.w;
}

bool PointBox(in vec2 p, in vec4 b, in float angle)
{
	vec2 size = b.zw / 2.0;
	vec2 c = b.xy;
	vec2 tl = vec2(b.x - size.x, b.y - size.y);
	vec2 tr = vec2(b.x + size.x, b.y - size.y);
	vec2 bl = vec2(b.x - size.x, b.y + size.y);
	vec2 br = vec2(b.x + size.x, b.y + size.y);

	float cosA = cos(radians(angle));
	float sinA = sin(radians(angle));

	tl = vec2(dot(tl - c, vec2(cosA, sinA)), dot(tl - c, vec2(-sinA, cosA))) + c;
    tr = vec2(dot(tr - c, vec2(cosA, sinA)), dot(tr - c, vec2(-sinA, cosA))) + c;
    bl = vec2(dot(bl - c, vec2(cosA, sinA)), dot(bl - c, vec2(-sinA, cosA))) + c;
    br = vec2(dot(br - c, vec2(cosA, sinA)), dot(br - c, vec2(-sinA, cosA))) + c;

	float l1 = (tl.y - tr.y) / (tl.x - tr.x);
    float l2 = (tr.y - br.y) / (tr.x - br.x);
    float l3 = (br.y - bl.y) / (br.x - bl.x);
    float l4 = (bl.y - tl.y) / (bl.x - tl.x);
    float d1 = tl.y - l1 * tl.x;
    float d2 = tr.y - l2 * tr.x;
    float d3 = br.y - l3 * br.x;
    float d4 = bl.y - l4 * bl.x;

	bool s1 = (tl.y - tr.y) * (p.x - tr.x) > (p.y - tr.y) * (tl.x - tr.x);
    bool s2 = (tr.y - br.y) * (p.x - br.x) > (p.y - br.y) * (tr.x - br.x);
    bool s3 = (br.y - bl.y) * (p.x - bl.x) > (p.y - bl.y) * (br.x - bl.x);
    bool s4 = (bl.y - tl.y) * (p.x - tl.x) > (p.y - tl.y) * (bl.x - tl.x);

	return s1 == s2 && s2 == s3 && s3 == s4;
}

void main()
{
	ivec2 renderSize = textureSize(render, 0);
	vec2 uv = gl_FragCoord.xy / renderSize;
	vec2 lightUV = vec2(gl_FragCoord.x, renderSize.y - gl_FragCoord.y) / renderSize;
	vec4 pixel = texture2D(render, uv);
	vec4 light = texture2D(lightMap, lightUV);
	vec2 point = vec2(gl_FragCoord.x, renderSize.y - gl_FragCoord.y);
	int player = 0;
	if (PointAABB(point, camOwnerRect))
	{
		for (int i = 0; i < camOwnerBoneCount; i++)
		{
			vec4 bone = camOwnerBones[i];
			float rot = camOwnerBonesRot[i];
			if (bone.zw != vec2(0.0) && PointBox(point, bone, 180 - rot)) { FragColor = pixel * light; player = 1; break; }
		}
	}
	if (player == 0)
	{
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
}
