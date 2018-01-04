in vec4 vertexPos[];
in int vertexColorId[];
in vec3 vertexNormal[];
in vec3 vertexLightPos[];
in vec3 vertexLightNormal[];

vec3 upperLightPos[3];
vec4 upperVertex[3];


centroid out vec4 fragPos;
centroid out vec4 fragColor;
centroid out vec3 fragLightPos;
centroid out vec3 fragLightNormal;

uniform samplerBuffer faceColors;
uniform bool transparent;

uniform float PINSize;

vec4 faceColor;

void emitUpperVertex(int i)
{
  fragLightNormal = vertexLightNormal[0];
  fragLightPos = upperLightPos[i];
  fragColor = faceColor;
  fragPos = gl_ModelViewProjectionMatrix * upperVertex[i];
  gl_Position = fragPos;
  EmitVertex();
}

void emitLowerVertex(int i)
{
  fragLightNormal = -vertexLightNormal[0];
  fragLightPos = vertexLightPos[i];
  fragColor = faceColor;
  fragPos =  gl_ModelViewProjectionMatrix * vertexPos[i];
  gl_Position = fragPos;
  EmitVertex();
}

void emitSideVertex(int i, bool upper)
{
  fragLightNormal = vertexLightNormal[1];
  if(upper) {
    fragLightPos = upperLightPos[i];
    fragPos =  gl_ModelViewProjectionMatrix * upperVertex[i];
  } else {
    fragLightPos = vertexLightPos[i];
    fragPos =  gl_ModelViewProjectionMatrix * vertexPos[i];
  }
  gl_Position = fragPos;
  fragColor = faceColor;
  EmitVertex();
}

void main()
{
  vec4 vertexColor = texelFetch(faceColors, vertexColorId[0]);
  if(vertexColor.a == 0)
    return;
  if(!transparent)
    vertexColor.a = 1;

  faceColor = vertexColor;

  for(int i = 0 ; i < 3 ; ++i) {
    vec3 shift = PINSize * vertexNormal[0];
    upperLightPos[i] = vertexLightPos[i] + shift;
    upperVertex[i] = vertexPos[i];
    upperVertex[i].xyz += shift * vertexPos[i].w;
  }

  // First, upper triangle
  emitUpperVertex(0);
  emitUpperVertex(1);
  emitUpperVertex(2);
  EndPrimitive();

  // Second, lower triangle
  emitLowerVertex(0);
  emitLowerVertex(2);
  emitLowerVertex(1);
  EndPrimitive();

  // At last, emit the two side triangles
  emitSideVertex(1, true);
  emitSideVertex(1, false);
  emitSideVertex(2, true);
  emitSideVertex(2, false);
  EndPrimitive();

}
