uniform sampler2D screen;
uniform vec2 screenSize;

const int maxLightsCount = 16;

uniform vec2 lightPos[maxLightsCount];
uniform float lightDist[maxLightsCount];
uniform vec4 lightClr[maxLightsCount];

void main()
{
	vec4 tex = texture2D(screen, gl_TexCoord[0].xy);
	vec4 lightSum;
	for (int i = 0; i < maxLightsCount; i++)
	{
		float dist = lightDist[i];
		if (dist <= 0.0) { continue; }
		vec2 pos = vec2(lightPos[i].x, screenSize.y - lightPos[i].y);
		vec4 clr = lightClr[i];
		float brightness = ((dist - distance(gl_FragCoord.xy, pos)) / dist);
		vec4 light = clr * brightness;
		lightSum += light;
	}
	gl_FragColor = tex * lightSum;
	gl_FragColor.a = 1;
}