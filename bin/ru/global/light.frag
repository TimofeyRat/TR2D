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
		vec2 delta = lightPos[i] - gl_FragCoord.xy;
		float angle = degrees(atan(delta.y, delta.x));
		float minAngle = lightData[i].y - lightData[i].z / 2;
		float maxAngle = lightData[i].y + lightData[i].z / 2;
		if (angle < minAngle || angle > maxAngle) continue;
		float dist = distance(lightPos[i], gl_FragCoord.xy);
		float brightness = 1.0 - dist / lightData[i].x;
		// brightness -= smoothstep(minAngle, maxAngle, angle);
		FragColor += lightClr[i] * brightness;
		lit += 1;
	}
	if (lit > 0) FragColor.a = 1;
	else FragColor.a = 0;
}