// Does final colour output for the render
#version 110

varying vec3 normal;

void main(void)
{
	gl_FragColor = vec4(vec3(normal.r),1.0);
}