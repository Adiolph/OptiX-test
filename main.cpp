#include <optix.h>
#include <stdlib.h>
#include <stdio.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sutil/sutil.h>
#include <optixu/optixpp_namespace.h>
#include <optixu/optixu_math_namespace.h>

int SP_NUM_X = 20;
int SP_NUM_Y = 20;
int SP_NUM_Z = 20;
float SP_RAD = 15;
float SP_SEP = 50;

const int NUM_DOM = 8000;

int NUM_PHOTON = 1000;

const char *const TEST_NAME = "optixTest";

void printUsageAndExit(const char *argv0);
void createContext(RTcontext *context);
void createBuffer(RTcontext context, RTbuffer *sphere_coor, RTbuffer *output_id);
void createGeometry(RTcontext context, RTgeometry *sphere, int primIdx, RTprogram sphere_intersect, RTprogram sphere_bounds);
void createMaterial(RTcontext context, RTmaterial *material);
void createGroup(RTcontext context);

void printUsageAndExit(const char *argv0)
{
  fprintf(stderr, "Usage  : %s [options]\n", argv0);
  fprintf(stderr, "Options: --file | -f <filename>      Specify file for image output\n");
  fprintf(stderr, "         --help | -h                 Print this usage message\n");
  fprintf(stderr, "         --dim=<width>x<height>      Set image dimensions; defaults to 512x384\n");
  exit(1);
}

int main(int argc, char *argv[])
{
  RTcontext context = 0;
  try
  {
    // Primary RTAPI objects
    RTcontext context;
    RTprogram ray_gen_program;
    RTbuffer sphere_coor;
    RTbuffer output_id;

    // Set up state
    createContext(&context);
    createBuffer(context, &sphere_coor, &output_id);
    createGroup(context);
    RT_CHECK_ERROR(rtContextValidate(context));
    RT_CHECK_ERROR(rtContextLaunch1D(context, 0, NUM_PHOTON));

    // read output buffer
    void *output_id_ptr;
    RT_CHECK_ERROR(rtBufferMap(output_id, &output_id_ptr));
    unsigned int *output_id_data = (unsigned int *)output_id_ptr;
    for (int i = 0; i < NUM_PHOTON; i++)
      std::cout << output_id_data[i] << std::endl;
    RT_CHECK_ERROR(rtBufferUnmap(output_id));

    // clean up
    RT_CHECK_ERROR(rtContextDestroy(context));
    return (0);
  }
  SUTIL_CATCH(context)
}

void createContext(RTcontext *context)
{
  RTprogram ray_gen_program;
  RTprogram exception_program;
  RTprogram miss_program;

  /* variables for ray gen program */
  RTvariable source_pos;

  /* Setup context */
  RT_CHECK_ERROR(rtContextCreate(context));
  RT_CHECK_ERROR(rtContextSetRayTypeCount(*context, 1));
  RT_CHECK_ERROR(rtContextSetEntryPointCount(*context, 1));

  /* Ray generation program */
  const char *ptx = sutil::getPtxString(TEST_NAME, "point_source.cu");
  RT_CHECK_ERROR(rtProgramCreateFromPTXString(*context, ptx, "point_source", &ray_gen_program));
  RT_CHECK_ERROR(rtContextSetRayGenerationProgram(*context, 0, ray_gen_program));
  RT_CHECK_ERROR(rtContextDeclareVariable(*context, "source_pos", &source_pos));
  rtVariableSet3f(source_pos, 0.0, 0.0, 0.0);

  /* Exception program */
  RT_CHECK_ERROR(rtProgramCreateFromPTXString(*context, ptx, "exception", &exception_program));
  RT_CHECK_ERROR(rtContextSetExceptionProgram(*context, 0, exception_program));

  /* Miss program */
  RT_CHECK_ERROR(rtProgramCreateFromPTXString(*context, ptx, "miss", &miss_program));
  RT_CHECK_ERROR(rtContextSetMissProgram(*context, 0, miss_program));
}

void createBuffer(RTcontext context, RTbuffer *sphere_coor, RTbuffer *output_id)
{
  // create buffer
  rtBufferCreate(context, RT_BUFFER_INPUT, sphere_coor);
  rtBufferSetFormat(*sphere_coor, RT_FORMAT_FLOAT4);
  rtBufferSetSize1D(*sphere_coor, NUM_DOM);
  rtBufferCreate(context, RT_BUFFER_OUTPUT, output_id);
  rtBufferSetFormat(*output_id, RT_FORMAT_UNSIGNED_INT);
  rtBufferSetSize1D(*output_id, NUM_PHOTON);

  // create data for dom position and map to buffer
  void *sphere_coor_ptr;
  RT_CHECK_ERROR(rtBufferMap(*sphere_coor, &sphere_coor_ptr));
  float4 *sphere_coor_data = (float4 *)sphere_coor_ptr;
  for (int i = 0; i < NUM_DOM; i++)
  {
    int nx = static_cast<int>(i / 400);
    int ny = static_cast<int>((i - 400 * nx) / 20);
    int nz = i - 400 * nx - 20 * ny;
    sphere_coor_data[i].x = nx * 50 - 475;
    sphere_coor_data[i].y = ny * 50 - 475;
    sphere_coor_data[i].z = nz * 50 - 475;
    sphere_coor_data[i].w = 10;
  }
  RT_CHECK_ERROR(rtBufferUnmap(*sphere_coor));

  // declear variable
  RTvariable sphere_coor_var;
  RTvariable output_id_var;
  rtContextDeclareVariable(context, "sphere_coor", &sphere_coor_var);
  rtContextDeclareVariable(context, "output_id", &output_id_var);
  rtVariableSetObject(sphere_coor_var, &sphere_coor);
  rtVariableSetObject(output_id_var, &output_id);

}

void createMaterial(RTcontext context, RTmaterial *material)
{
  RTprogram chp;

  const char *ptx = sutil::getPtxString(TEST_NAME, "simple_dom.cu");
  RT_CHECK_ERROR(rtProgramCreateFromPTXString(context, ptx, "closest_hit", &chp));

  RT_CHECK_ERROR(rtMaterialCreate(context, material));
  RT_CHECK_ERROR(rtMaterialSetClosestHitProgram(*material, 0, chp));
}

void createGeometry(RTcontext context, RTgeometry *sphere, int primIdx, RTprogram sphere_intersect, RTprogram sphere_bounds)
{
  RTvariable sphere_coor;
  float sphere_loc[4] = {0, 0, 0, 1.5};

  RT_CHECK_ERROR(rtGeometryCreate(context, sphere));
  RT_CHECK_ERROR(rtGeometrySetPrimitiveCount(*sphere, 1u));
  RT_CHECK_ERROR(rtGeometrySetPrimitiveIndexOffset(*sphere, primIdx));

  RT_CHECK_ERROR(rtGeometrySetBoundingBoxProgram(*sphere, sphere_intersect));
  RT_CHECK_ERROR(rtGeometrySetIntersectionProgram(*sphere, sphere_bounds));

  RT_CHECK_ERROR(rtGeometryDeclareVariable(*sphere, "sphere_coor", &sphere_coor));
  RT_CHECK_ERROR(rtVariableSet4fv(sphere_coor, sphere_loc));
}

void createGroup(RTcontext context)
{
  // create geometry program
  RTprogram intersection_program;
  RTprogram bounding_box_program;
  const char *ptx = sutil::getPtxString(TEST_NAME, "sphere.cu");
  RT_CHECK_ERROR(rtProgramCreateFromPTXString(context, ptx, "bounds", &bounding_box_program));
  RT_CHECK_ERROR(rtProgramCreateFromPTXString(context, ptx, "intersect", &intersection_program));

  // create geometry instances
  std::vector<RTgeometryinstance> gis;
  RTmaterial material;
  rtMaterialCreate(context, &material);
  createMaterial(context, &material);
  for (int i = 0; i < NUM_DOM; i++)
  {
    RTgeometry sphere;
    createGeometry(context, &sphere, i, intersection_program, bounding_box_program);
    RTgeometryinstance geoInstance;
    rtGeometryInstanceCreate(context, &geoInstance);
    RT_CHECK_ERROR(rtGeometryInstanceGetGeometry(geoInstance, &sphere));
    RT_CHECK_ERROR(rtGeometryInstanceSetMaterialCount(geoInstance, 1u));
    RT_CHECK_ERROR(rtGeometryInstanceGetMaterial(geoInstance, 0, &material));
    gis.push_back(geoInstance);
  }

  // create geometry group
  RTgeometrygroup geoGroup;
  rtGeometryGroupCreate(context, &geoGroup);
  rtGeometryGroupSetChildCount(geoGroup, gis.size());
  for (int i = 0; i < gis.size(); i++)
    rtGeometryGroupSetChild(geoGroup, i, gis[i]);
  RTacceleration accel_geoGroup;
  rtAccelerationCreate(context, &accel_geoGroup);
  rtAccelerationSetBuilder(accel_geoGroup, "Trbvh");
  rtGeometryGroupSetAcceleration(geoGroup, accel_geoGroup);

  // create group
  RTgroup group;
  rtGroupCreate(context, &group);
  rtGroupSetChildCount(group, 1);
  RT_CHECK_ERROR(rtGroupSetChild(group, 0, geoGroup));
  RTacceleration accel_group;
  rtAccelerationCreate(context, &accel_group);
  rtAccelerationSetBuilder(accel_group, "NoAccel");
  rtGroupSetAcceleration(group, accel_group);

  // set group object
  RTvariable top_object;
  rtContextDeclareVariable(context, "top_object", &top_object);
  RT_CHECK_ERROR(rtVariableSetObject(top_object, group));
}