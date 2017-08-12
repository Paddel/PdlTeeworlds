const char *SOURCE_RIPPLE_FRAGMENT =

"uniform float u_Time;\n"
"uniform vec2 u_Resolution;\n"
"uniform sampler2D iChannel0;\n"

"\n"
"float speed = 1.8; \n"
"float freq = 40.;\n"
"float amp = 0.001; \n"
"float zoom = 0.995;\n"

"void main() \n"
"{\n"
"   float time = u_Time;\n"
"	vec2 uv = gl_FragCoord.xy / u_Resolution.xy;\n"

"   vec2 ripple = vec2(cos((length(uv-0.5)*freq)+(-time*speed)),sin((length(uv-0.5)*freq)+(-time*speed)))*amp;\n"

"   vec2 Coord = (uv+ripple) * zoom + ((1.0 - zoom) *0.5);\n"
"   gl_FragColor = vec4(texture(iChannel0, Coord));\n"
"}\n";