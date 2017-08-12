const char *SOURCE_LOADGREY_FRAGMENT =

"uniform float u_Percent;\n"
"uniform vec2 u_Resolution;\n"
"uniform sampler2D iChannel0;\n"
"uniform float u_Radius;\n"

"vec4 grayscale(vec4 Color)\n"
"{\n"
"    float g = (Color.r + Color.g + Color.b) / 3.0;\n"
" 	return vec4(g, g, g, Color.a);\n"
"}\n"

"vec4 circle(vec2 uv, vec2 pos, float rad, vec3 color) {\n"
"	float d = length(pos - uv) - rad;\n"
"	float t = clamp(d, 0.0, 1.0);\n"
"	return vec4(color, 1.0 - t);\n"
"}\n"

"void main() \n"
"{\n"
"	vec2 uv = gl_FragCoord.xy / u_Resolution.xy;\n"
"    vec2 center = u_Resolution.xy * 0.5;\n"
"    float radius = u_Radius;\n"
"    float Percent = u_Percent;\n"

"    vec4 Background = vec4(texture(iChannel0, uv));\n"
"    vec4 Color = grayscale(Background);\n"
"    vec4 CircCol = circle(uv * u_Resolution.xy, center, radius * Percent, Background.rgb);\n"


"    vec4 FinalColor = mix(Color, CircCol, CircCol.a);\n"

"	gl_FragColor = Background;\n"
"}\n";