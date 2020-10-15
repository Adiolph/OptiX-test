#include <optix.h>

using namespace optix;

struct PerRayData_pathtrace
{
  unsigned int hitID;
  unsigned int seed;
};

rtDeclareVariable(float3, hit_pos, attribute hit_pos);
rtDeclareVariable(PerRayData_pathtrace, prd_radiance, rtPayload, );

RT_PROGRAM void closest_hit_radiance()
{
  PerRayData_pathtrace.hitID = hit_pos.x / 10;
};


