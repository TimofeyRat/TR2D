#version 330 core

uniform vec2 lightPos[32]; 
uniform vec4 lightClr[32];
uniform vec3 lightData[32];

out vec4 FragColor;

void main()
{
	FragColor = vec4(0.0);
	int lit = 0;
	for (int i = 0; i < 32; i++)
	{
		if (lightData[i].x == 0) break;
		float radius = lightData[i].x;
		vec2 delta = lightPos[i] - gl_FragCoord.xy;
		float angle = degrees(atan(delta.y, delta.x));
		float minAngle = lightData[i].y - lightData[i].z / 2;
		float maxAngle = lightData[i].y + lightData[i].z / 2;
		float brightness;
		if (angle < minAngle || angle > maxAngle) continue;
		float dist = distance(lightPos[i], gl_FragCoord.xy);
		if (dist <= radius) { brightness = 1.0; }
		else brightness = 1.0 - (dist - radius) / lightData[i].x;
		FragColor += clamp(lightClr[i] * brightness, vec4(0), vec4(1));
		lit += 1;
	}
	if (lit > 0) FragColor.a = 1;
	else FragColor.a = 0;
	FragColor = clamp(FragColor, vec4(0), vec4(1));
}