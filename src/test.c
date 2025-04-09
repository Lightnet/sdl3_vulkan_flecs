
#include "mimalloc.h"
#include "cglm/cglm.h"
#include <stdio.h>

int main(){
  int num = 10;
  char str[] = "Hello";
   
  printf("Integer: %d, String: %s\n", num, str);
  
  // vec2 vector;
  // glm_vec2_zero(vector);

  vec3 vector3;
  glm_vec3_zero(vector3);
  glm_vec3_print(vector3, stderr);

  void* p1 = mi_malloc(16);

  mi_free(p1);

  return 0;
}