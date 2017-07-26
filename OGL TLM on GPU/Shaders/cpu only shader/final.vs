// Performs the vertex transform for the final lit shader pass
// to display on the screen
#version 110
varying vec3 normal;

void main(void)
{
	gl_Position = ftransform();	// transform real vertex coord
	normal = gl_NormalMatrix * gl_Normal;
}	