uniform float u_opacity;
uniform sampler2D u_colorTex;
varying vec2 v_colorTexCoords;

void main()
{
  vec4 finalColor = texture2D(u_colorTex, v_colorTexCoords);
  finalColor.a *= u_opacity;
  gl_FragColor = finalColor;
}

