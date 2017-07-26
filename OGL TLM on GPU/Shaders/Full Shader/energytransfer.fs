// Performs the energy transfer from the pervious frame
// Samples the driving map
#version 110
#extension GL_ARB_draw_buffers : enable
uniform sampler2D energySource;
uniform sampler2D drivingMap;

uniform float step;	// step between sample positions

void main(void)
{
	// Sample from the surrounding nodes
	vec4 east = texture2D(energySource, gl_TexCoord[0].st + vec2(step,0.0));
	vec4 west = texture2D(energySource, gl_TexCoord[0].st + vec2(-step,0.0));
	vec4 north = texture2D(energySource, gl_TexCoord[0].st + vec2(0.0,step));
	vec4 south = texture2D(energySource, gl_TexCoord[0].st + vec2(0.0,-step));
	
	// Perform the transfer
	// x = north, y = south, z = east, w = west components
	// Thanks to Zeux for pointing out the dot product solution below
	float SinkWest  = dot(east,vec4( 0.5, 0.5,-0.5, 0.5));
	float SinkEast  = dot(west,vec4( 0.5, 0.5, 0.5,-0.5));
	float SinkSouth = dot(north,vec4(-0.5, 0.5, 0.5, 0.5));
	float SinkNorth = dot(south,vec4( 0.5,-0.5, 0.5, 0.5)); 
	
	// Next we read in the simulation driving data
	vec4 energy = vec4(SinkNorth, SinkSouth, SinkEast, SinkWest);
	
	gl_FragData[0] = energy;
}