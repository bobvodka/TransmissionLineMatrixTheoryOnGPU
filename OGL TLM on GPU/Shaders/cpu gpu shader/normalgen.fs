// Generate normal information for the final pass
#version 110
#extension GL_ARB_draw_buffers : enable
uniform sampler2D heightMap;
uniform float step;		// step between texture samples

void main(void)
{
	// Take all nine  taps
    vec3 tl = (texture2D (heightMap, gl_TexCoord[0].st  + vec2(-step, -step)).bgr );		// top left
    vec3  l = (texture2D (heightMap, gl_TexCoord[0].st  + vec2(-step,  0)).bgr    );		// left
    vec3 bl = (texture2D (heightMap, gl_TexCoord[0].st  + vec2(-step,  step)).bgr );		// bottom left
    vec3  t = (texture2D (heightMap, gl_TexCoord[0].st  + vec2( 0, -step)).bgr    );		// top
    vec3  b = (texture2D (heightMap, gl_TexCoord[0].st  + vec2( 0,  step)).bgr    );		// bottom
    vec3 tr = (texture2D (heightMap, gl_TexCoord[0].st  + vec2( step, -step)).bgr );		// top right
    vec3  r = (texture2D (heightMap, gl_TexCoord[0].st  + vec2( step,  0)).bgr    );		// right
    vec3 br = (texture2D (heightMap, gl_TexCoord[0].st  + vec2( step,  step)).bgr );		// bottom right
    vec3  c = (texture2D (heightMap, gl_TexCoord[0].st ).bgr);									// center
    
    vec3 normal = cross(c - t, c - tr);
    normal += cross(c - tr,c - r);
    normal += cross(c - r, c - br);
    normal += cross(c - br, c - b);
    normal += cross(c - b, c - bl);
    normal += cross(c - bl, c - l);
    normal += cross(c - l, c - tl);
    normal += cross(c - tl, c - t);
    
    normal /= 8;
    gl_FragData[0] = vec4(normalize(normal),1.0).bgra;
}