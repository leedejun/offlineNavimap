attribute vec3 a_position;
attribute vec2 a_colorTexCoords;
uniform float u_zScale;


// A uniform to contain the heightmap image
uniform sampler2D u_heightmapTex;

// A uniform to contain the scaling constant
//uniform float heightmapScale = 1.0f;

// A variable to store the height of the point
varying float vAmount;
varying float elevation;
varying vec3 newPosition;
varying vec4 heightmapData;

uniform mat4 u_modelView;
uniform mat4 u_projection;
uniform mat4 u_pivotTransform;
varying vec2 v_colorTexCoords;

varying vec4 pos;

varying float a_zScale;


float getElevation(vec2 coord) {
    // Convert encoded elevation value to meters
    vec2 uvCoord = coord;
    if(coord.x<0.000001)
    {
      uvCoord.x = 0.0;
    }

    if(coord.x>1.0)
    {
      uvCoord.x = 1.0;
    }

    if(coord.y<0.000001)
    {
      uvCoord.y = 0.0;
    }

    if(coord.y>1.0)
    {
      uvCoord.y = 1.0;
    }

    vec4 data = texture2D(u_heightmapTex, uvCoord) * 255.0;
    float height = -10000.0 + ((data.r * 256.0 * 256.0 + data.g * 256.0 + data.b) * 0.1);
    return height;
}

uint xor_shift_rand(uint seed) {
    seed ^= (seed << 13);
    seed ^= (seed >> 17);
    seed ^= (seed << 5);
    return seed;
}

float random(uint seed) {
    return float(xor_shift_rand(seed)) * (1.0 / 4294967296.0);
}

float random8848(uint seed) {
    return random(seed) * 8848.0;
}


void main()
{
  v_colorTexCoords = a_colorTexCoords;
  // The heightmap data at those coordinates
  // vec4 heightmapData = texture2D(u_heightmapTex, v_colorTexCoords);
  // Sample the terrain-rgb tile at the current fragment location.
  heightmapData = texture(u_heightmapTex, v_colorTexCoords);
  elevation = -10000.0 + ((heightmapData.r * 255.0 * 256.0 * 256.0 + heightmapData.g * 255.0 * 256.0 + heightmapData.b * 255.0) * 0.1);
  newPosition = vec3(0.0, 0.0, -1.0) * elevation *(360.0 / 40008245.0);
  pos = vec4(a_position, 1.0) * u_modelView;
  pos.xyw = (pos * u_projection).xyw;
  pos.z = newPosition.z * u_zScale;
  gl_Position = u_pivotTransform * pos;
  // gl_Position = applyPivotTransform(pos, u_pivotTransform, 0.0);
}
