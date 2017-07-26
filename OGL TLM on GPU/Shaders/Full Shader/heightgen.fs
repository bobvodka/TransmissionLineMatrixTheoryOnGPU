// Generates heightmap information
#version 110
#extension GL_ARB_draw_buffers : enable
uniform sampler2D energySource;
uniform sampler2D orignalPos;
uniform sampler2D orignalNormal;

uniform float scale;	// scale factor for output


void main(void)
{
	vec4 height = texture2D(energySource, gl_TexCoord[0].st);
	vec3 pos = texture2D(orignalPos, gl_TexCoord[0].st).xyz;
	vec3 normal = texture2D(orignalNormal, gl_TexCoord[0].st).xyz;
	
	// Work out height and write it out
	// Move the orignal vertex position by the height amount in the direction of the normal
	// swizzle needed so it decodes in VS properly when sourced as a vertex stream
	gl_FragData[0] = vec4(pos + (normal * dot(height,vec4(scale))),1.0).bgra;  
}