layout (triangles) in;
layout (triangle_strip, max_vertices=10) out;

in gl_PerVertex {
  vec4 gl_Position;
  vec4 gl_FrontColor;
} gl_in[];

out gl_PerVertex {
  vec4 gl_Position;
  vec4 gl_FrontColor;
};
