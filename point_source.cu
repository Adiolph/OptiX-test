#include <optix_world.h>
#include <common.h>
#include <helpers.h>
#include <random.h>  // OptiX random header file in SDK/cuda/random.h

using namespace optix;

struct PerRayData_pathtrace
{
  unsigned int hitID;
  unsigned int seed;
};

rtDeclareVariable(float3, source_pos, , );  // position of source
rtDeclareVariable(rtObject, top_object, , );  // group object
rtDeclareVariable(uint, launch_index, rtLaunchIndex, );
rtDeclareVariable(uint, launch_dim, rtLaunchDim, );
rtBuffer<uint, 1> output_id;  // record the id of dom that photon hit, 0 if no hit


RT_PROGRAM void point_source()
{
  float3 ray_origin = source_pos;
  PerRayData_pathtrace prd;
  prd.hitID = 0;
  prd.seed = tea<4>(launch_index, 0);
  float cos_th = 2.0 * rnd(prd.seed) - 1.0;
  float sin_th = sqrt(1 - cos_th*cos_th);
  float cos_ph = cos(2 * pi * rnd(prd.seed));
  float sin_ph = sin(2 * pi * rnd(prd.seed));
  float3 ray_direction = make_float3(cos_th*cos_ph, cos_th*sin_ph, sin_th);
  Ray ray = make_Ray(ray_origin, ray_direction, RADIANCE_RAY_TYPE, 0.01, RT_DEFAULT_MAX);
  rtTrace(top_object, ray, prd);
  output_id[launch_index] = prd.hitID;
}
