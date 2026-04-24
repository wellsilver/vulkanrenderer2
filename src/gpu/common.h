struct vertice {
  float x,y,z;
  float r,g,b;
};

static const char shadercode[] = {
#embed "../../out/shaders.spv"
};