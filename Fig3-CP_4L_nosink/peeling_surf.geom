in vec4 projPos[];
in int vertexColorId[];
in vec3 vertexLightPos[];
in vec3 vertexLightNormal[];

uniform samplerBuffer cellColors;
uniform bool transparent;

centroid out vec4 fragPos;
centroid out vec4 fragColor;
centroid out vec3 fragLightPos;
centroid out vec3 fragLightNormal;

void main()
{
  vec4 vertexColor = texelFetch(cellColors, vertexColorId[0]);
  if(!transparent)
    vertexColor.a = 1;
  for(int i = 0 ; i < 3 ; ++i) {
    gl_Position = projPos[i];
    fragColor = vertexColor;
    fragLightNormal = vertexLightNormal[i];
    fragLightPos = vertexLightPos[i];
    fragPos = projPos[i];
    EmitVertex();
  }
  EndPrimitive();
}
