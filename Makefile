CUDA_FILES = simple_dom.cu sphere.cu point_source.cu
PTX_FILES = simple_dom.ptx sphere.ptx point_source.ptx
INCLUDES = -I${OPTIX_PATH}/include -I${OPTIX_PATH}/SDK
LIB_PATH = -L${OPTIX_PATH}/lib64
LIBS = -loptix -loptixu -lsutil_sdk
NVCC_FLAGS = -m64 -gencode arch=compute_75,code=sm_75

all: $(PTX_FILES) main.cpp
	nvcc $(NVCC_FLAGS) $(INCLUDES) -o main.exe main.cpp $(LIB_PATH) $(LIBS) 

$(PTX_FILES): $(CUDA_FILES)
	nvcc $(NVCC_FLAGS) $(INCLUDES) --ptx $(CUDA_FILES)

clean:
	rm *.exe *ptx