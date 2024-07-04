#version 330 core

uniform vec2 lightPos[32]; 
uniform vec4 lightClr[32];
uniform vec3 lightData[32];

uniform vec4 triggers[32];
uniform vec2 triggersData[32];

out vec4 FragColor;

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

bool LineLine(in vec2 p1, in vec2 p2, in vec2 p3, in vec2 p4)
{
	float x1 = p1.x, y1 = p1.y,
		x2 = p2.x, y2 = p2.y,
		x3 = p3.x, y3 = p3.y,
		x4 = p4.x, y4 = p4.y;
	float uA = ((x4-x3)*(y1-y3) - (y4-y3)*(x1-x3)) / ((y4-y3)*(x2-x1) - (x4-x3)*(y2-y1));
	float uB = ((x2-x1)*(y1-y3) - (y2-y1)*(x1-x3)) / ((y4-y3)*(x2-x1) - (x4-x3)*(y2-y1));
	return (uA >= 0.0 && uA <= 1.0 && uB >= 0.0 && uB <= 1.0);
}

bool LineAABB(in vec2 p1, in vec2 p2, in vec4 aabb)
{
	return LineLine(p1, p2, aabb.xy, aabb.xy + vec2(0, aabb.w)) ||
		LineLine(p1, p2, aabb.xy, aabb.xy + vec2(aabb.z, 0)) ||
		LineLine(p1, p2, aabb.xy + vec2(aabb.z, 0), aabb.xy + aabb.zw) ||
		LineLine(p1, p2, aabb.xy + vec2(0, aabb.w), aabb.xy + aabb.zw);
}

bool LineBox(in vec2 p1, in vec2 p2, in vec4 box, in float degree)
{
	bool left = false, right = false, top = false, bottom = false;
	float angle = radians(degree);
	vec2 c = box.xy;
	float ca = cos(angle), sa = sin(angle);
	
	// Corners(normal)
	vec2 c1 = c - box.zw / 2.0,
		c2 = c + vec2(box.z, -box.w) / 2.0,
		c3 = c + box.zw / 2.0,
		c4 = c + vec2(-box.z, box.w) / 2.0;
	c1 -= c; c2 -= c; c3 -= c; c4 -= c;
	// Corners(rotated)
	vec2 r1 = vec2(c1.x * ca - c1.y * sa, c1.x * sa + c1.y * ca) + c,
		r2 = vec2(c2.x * ca - c2.y * sa, c2.x * sa + c2.y * ca) + c,
		r3 = vec2(c3.x * ca - c3.y * sa, c3.x * sa + c3.y * ca) + c,
		r4 = vec2(c4.x * ca - c4.y * sa, c4.x * sa + c4.y * ca) + c;
	// Intersection check
	left = LineLine(p1, p2, r1, r4);
	right = LineLine(p1, p2, r2, r3);
	top = LineLine(p1, p2, r1, r2);
	bottom = LineLine(p1, p2, r3, r4);
	
	return left || right || bottom || top;
}

bool ShadowLineBox(in vec2 p1, in vec2 p2, in vec4 box, in float degree)
{
	bool checkLeft = false, left = false,
		checkRight = false, right = false,
		checkTop = false, top = false,
		checkBottom = false, bottom = false;
	float angle = radians(degree);
	vec2 c = box.xy;
	float ca = cos(angle), sa = sin(angle);

	// On what side of box is the light source?
	vec2 offset = p1 - box.xy;
	checkLeft = offset.x > 0.0;
	checkRight = offset.x < 0.0;
	checkTop = offset.y < 0.0;
	checkBottom = offset.y > 0.0;

	// Corners(normal)
	vec2 c1 = c - box.zw / 2.0,
		c2 = c + vec2(box.z, -box.w) / 2.0,
		c3 = c + box.zw / 2.0,
		c4 = c + vec2(-box.z, box.w) / 2.0;
	c1 -= c; c2 -= c; c3 -= c; c4 -= c;
	// Corners(rotated)
	vec2 r1 = vec2(c1.x * ca - c1.y * sa, c1.x * sa + c1.y * ca) + c,
		r2 = vec2(c2.x * ca - c2.y * sa, c2.x * sa + c2.y * ca) + c,
		r3 = vec2(c3.x * ca - c3.y * sa, c3.x * sa + c3.y * ca) + c,
		r4 = vec2(c4.x * ca - c4.y * sa, c4.x * sa + c4.y * ca) + c;
	// Intersection check
	left = LineLine(p1, p2, r1, r4);
	right = LineLine(p1, p2, r2, r3);
	top = LineLine(p1, p2, r1, r2);
	bottom = LineLine(p1, p2, r3, r4);

	return (checkLeft && left) || (checkRight && right) || (checkTop && top) || (checkBottom && bottom);
}

void main()
{
	FragColor = vec4(0.0);
	int lit = 0;
	//Light
	for (int i = 0; i < 32; i++)
	{
		if (lightData[i].x == 0) break;
		float radius = lightData[i].x;
		vec2 delta = lightPos[i] - gl_FragCoord.xy;
		float lightAngle = lightData[i].y;
		float angle = 180 - degrees(atan(delta.y, delta.x));
		float angle1 = lightData[i].y - lightData[i].z / 2;
		float angle2 = lightData[i].y + lightData[i].z / 2;
		float minAngle = min(angle1, angle2);
		float maxAngle = max(angle1, angle2);
		float brightness = 1;
		if (angle < minAngle || angle > maxAngle) continue;
		float dist = distance(lightPos[i], gl_FragCoord.xy);
		if (dist > radius) brightness = clamp(1.0 - (dist - radius) / lightData[i].x, 0, 1);
		float angular = 1;
		if (angle < lightAngle) angular = smoothstep(minAngle, lightAngle, angle);
		if (angle > lightAngle) angular = smoothstep(maxAngle, lightAngle, angle);
		FragColor += clamp(lightClr[i] * brightness * angular * 2, vec4(0), vec4(1));
		lit += 1;
	}
	//Shadows
	for (int i = 0; i < 32; i++)
	{
		vec2 light = lightPos[i];
		if (lightData[i].x == 0) break;
		for (int j = 0; j < 32; j++)
		{
			vec4 body = vec4(triggers[j].x + triggers[j].z / 2, triggers[j].y + triggers[j].w / 2, triggers[j].zw);
			float bodyAngle1 = triggersData[j].x;
			float bodyAngle2 = 180 - triggersData[j].x;
			if (PointBox(gl_FragCoord.xy, body, bodyAngle1)) continue;
			bool slb1 = ShadowLineBox(light, gl_FragCoord.xy, body, bodyAngle1);
			bool slb2 = ShadowLineBox(light, gl_FragCoord.xy, body, bodyAngle2);
			if ((int(bodyAngle1) % 90 == 0 && (slb1 || slb2)) || slb2) lit -= int(triggersData[j].y);
		}
	}
	//End
	if (lit > 0) FragColor.a = 1;
	else FragColor *= 0.75;
	FragColor = clamp(FragColor, vec4(0), vec4(1));
}