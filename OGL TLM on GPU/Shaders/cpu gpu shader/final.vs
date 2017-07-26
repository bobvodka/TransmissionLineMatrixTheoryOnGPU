// Performs the vertex transform for the final lit shader pass
// to display on the screen
#version 110

//attribute vec4 heightData;
attribute vec4 normalData;
varying vec3 normal;

void main(void)
{
	gl_Position = ftransform();
	normal = gl_NormalMatrix * vec3(normalData);
}	