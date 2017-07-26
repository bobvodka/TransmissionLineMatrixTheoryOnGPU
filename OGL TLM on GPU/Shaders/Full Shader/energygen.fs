// Performs the energy transfer from the pervious frame
// Samples the driving map
#version 110
#extension GL_ARB_draw_buffers : enable
uniform sampler2D energySource;
uniform sampler2D drivingMap;

uniform float step;	// step between sample positions

void main(void)
{
	// Sample from the energy
	vec4 energy = texture2D(energySource, gl_TexCoord[0].st);
	// Next we read in the simulation driving data
	vec4 driving = vec4(texture2D(drivingMap,gl_TexCoord[0].st).a);
	vec4 sink = energy - driving;
	
	gl_FragData[0] = sink;
}