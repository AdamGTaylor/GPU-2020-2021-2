#include <iostream>
#include <fstream>

#include <vector>
#include <array>
#include <string>

#include <numeric>
#include <algorithm>
#include <random>
#include <cmath>

#include <chrono>

#ifdef __APPLE__ //Mac OSX has a different name for the header file
#include <OpenCL/opencl.h>
#else
#include <CL/opencl.h>
#endif

#include <stdio.h>  // printf
#include <stdlib.h> // malloc
#include <stdint.h> // UINTMAX_MAX

int size1 = 0;                      //length
int size2 = 0;                      //width 
int V_min = 0;                      //the amount of minimum value pixels

static const int low = 0;           //min for bins
static const int high = 255;        //max for bins
static const int bins = 256;

static const std::string InputFileName("E:/_ELTE_PHYS_MSC/2_second_semester/gpu/project/gpu-parallel/pics/test_pic.txt");
static const std::string OutputFileName("E:/_ELTE_PHYS_MSC/2_second_semester/gpu/project/gpu-parallel/output/test_pic.txt");
void fromLinearMemory( std::vector<unsigned int> & input, std::vector<unsigned int>& veced);

template<typename T>
void save_pic(const std::vector<T> & veced_pic);

template<typename T>
T summer(const std::vector<T> & veced_data,int i1,int i2);

//error check
void checkErr(cl_int err, const char * name)
{
    if (err != CL_SUCCESS)
    {
        printf("ERROR: %s (%i)\n", name, err);
        exit( err );
    }
}

int main()
{      
    //creating objects from pics
    std::vector<cl_int> pic;               //input pic
    std::vector<cl_int> eq_pic;            //output pic

    
    //my hist is in what?
    std::vector<unsigned int> atomic_histogram(256,0);
    
    std::vector<cl_int> cdf(256,0);        //point mass function of pic
    std::vector<cl_int> h_v(256,0);        //new values for eq_pic created from eq

        //loading in texted picture
    std::ifstream myfile(InputFileName);
    if ( myfile.is_open() ){
        myfile >> size1;
        myfile >> size2;
        pic.resize(size1*size2);
        eq_pic.resize(size1*size2);
        for(int i=0; i < size1 * size2; ++i){
            myfile >> pic[i];
        }
        std::cout << "Succesful loading:" << size1 << "x" << size2 << std::endl;
    }

    //OpenCL stuff
    // queue -> device -> platform -> context -> kernel
    cl_int status = CL_SUCCESS;
    cl_uint numPlatforms = 0;
    std::vector<cl_platform_id> platforms;
    std::vector<std::vector<cl_device_id>> devices;
    //block num
    static const int block_size = 4;    //blocksize
    int nBlocksH = size2 / block_size;                   //number if block vertically
    
    status = clGetPlatformIDs(0, nullptr, &numPlatforms);
    if(status != CL_SUCCESS){ std::cout << "Cannot get number of platforms: " << status << "\n"; return -1; }
    
    platforms.resize(numPlatforms);
    devices.resize(numPlatforms);
	status = clGetPlatformIDs(numPlatforms, platforms.data(), nullptr);
    if(status != CL_SUCCESS){ std::cout << "Cannot get platform ids: " << status << "\n"; return -1; }

    for(cl_uint i=0; i<numPlatforms; ++i)
    {
        cl_uint numDevices = 0;
        status = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, 0, nullptr, &numDevices);
        if(status != CL_SUCCESS){ std::cout << "Cannot get number of devices: " << status << "\n"; return -1; }

        if(numDevices == 0){ std::cout << "There are no devices in platform " << i << "\n"; continue; }

        devices[i].resize(numDevices);
        
        status = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, numDevices, devices[i].data(), nullptr);
        if(status != CL_SUCCESS){ std::cout << "Cannot get device ids: " << status << "\n"; return -1; }
    }
    //select platform and device:
    const auto platformIdx = 0;
    const auto deviceIdx   = 0;
    const auto platform    = platforms[platformIdx];
    const auto device      = devices[platformIdx][deviceIdx];
    //print names:
    {
        size_t vendor_name_length = 0;
        status = clGetPlatformInfo(platform, CL_PLATFORM_VENDOR, 0, nullptr, &vendor_name_length);
        if(status != CL_SUCCESS){ std::cout << "Cannot get platform vendor name length: " << status << "\n"; return -1; }

        std::string vendor_name(vendor_name_length, '\0');
        status = clGetPlatformInfo(platform, CL_PLATFORM_VENDOR, vendor_name_length, (void*)vendor_name.data(), nullptr);
        if(status != CL_SUCCESS){ std::cout << "Cannot get platform vendor name: " << status << "\n"; return -1; }

        size_t device_name_length = 0;
        status = clGetDeviceInfo(device, CL_DEVICE_NAME, 0, nullptr, &device_name_length);
        if(status != CL_SUCCESS){ std::cout << "Cannot get device name length: " << status << "\n"; return -1; }

        std::string device_name(device_name_length, '\0');
        status = clGetDeviceInfo(device, CL_DEVICE_NAME, device_name_length, (void*)device_name.data(), nullptr);
        if(status != CL_SUCCESS){ std::cout << "Cannot get device name: " << status << "\n"; return -1; }

        std::cout << "Platform: " << vendor_name << "\n";
        std::cout << "Device: "   << device_name << "\n";
    }

	std::array<cl_context_properties, 3> cps = { CL_CONTEXT_PLATFORM, (cl_context_properties)platform, 0 };
	auto context = clCreateContext(cps.data(), 1, &device, 0, 0, &status);
    if(status != CL_SUCCESS){ std::cout << "Cannot create context: " << status << "\n"; return -1; }
    //now that we have choosen one platform, and it is displayed which one
    //the context should be created
    //Above OpenCL 1.2: I am using OpenCL 2.2
    cl_command_queue_properties cqps = CL_QUEUE_PROFILING_ENABLE;
	std::array<cl_queue_properties, 3> qps = { CL_QUEUE_PROPERTIES, cqps, 0 };
	auto queue = clCreateCommandQueueWithProperties(context, device, qps.data(), &status);
    if(status != CL_SUCCESS){ std::cout << "Cannot create command queue: " << status << "\n"; return -1; }

    //it requires absolute path. Solution: there is a package that knows absoute path!
	std::ifstream file("E:\\_ELTE_PHYS_MSC\\2_second_semester\\gpu\\project\\gpu-parallel\\atomics.cl");
	std::string source( std::istreambuf_iterator<char>(file), (std::istreambuf_iterator<char>()));
	size_t      sourceSize = source.size();
	const char* sourcePtr  = source.c_str();
	auto program = clCreateProgramWithSource(context, 1, &sourcePtr, &sourceSize, &status);
    if(status != CL_SUCCESS){ std::cout << "Cannot create program: " << status << "\n"; return -1; }

	status = clBuildProgram(program, 1, &device, "", nullptr, nullptr);
	if (status != CL_SUCCESS)
	{
        std::cout << "Cannot build program: " << status << "\n";
		size_t len = 0;
		status = clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, nullptr, &len);
		std::unique_ptr<char[]> log = std::make_unique<char[]>(len);
		status = clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, len, log.get(), nullptr);
		std::cout << log.get() << "\n";
		return -1;
	}

	auto kernel_shared_atomics = clCreateKernel(program, "gpu_histo_shared_atomics", &status);
    if(status != CL_SUCCESS){ std::cout << "Cannot create kernel 'gpu_histo_shared_atomics': " << status << "\n"; return -1; }

    auto kernel_accumulate = clCreateKernel(program, "gpu_histo_accumulate", &status);
    if(status != CL_SUCCESS){ std::cout << "Cannot create kernel 'gpu_histo_accumulate': " << status << "\n"; return -1; }
    //now we have a succesful kernel created!
    
    float dt2 = 0.0f;
    {   
        //input buffer has to be the size of pic! i have to give the pic pointer too
        auto bufferInput    = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, size1*size2*sizeof(cl_int), pic.data(), &status);
        if(status != CL_SUCCESS){ std::cout << "Cannot create input buffer: " << status << "\n"; return -1; }
        
        //buffer of partials has to be the size of bufferinput * block_num
        auto bufferPartials = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS,  nBlocksH*256*sizeof(unsigned int), nullptr,  &status);
        if(status != CL_SUCCESS){ std::cout << "Cannot create partial buffer: " << status << "\n"; return -1; }
        
        //filling the buffer with zeros
        cl_int zero = 0;
        status = clEnqueueFillBuffer(queue, bufferPartials, &zero, sizeof(cl_uchar4), 0, nBlocksH*256*sizeof(unsigned int), 0, nullptr, nullptr);
        if(status != CL_SUCCESS){ std::cout << "Cannot zero partial buffer: " << status << "\n"; return -1; }

        status = clFinish(queue);
        if(status != CL_SUCCESS){ std::cout << "Cannot finish queue: " << status << "\n"; return -1; }

        auto bufferOutput  = clCreateBuffer(context, CL_MEM_WRITE_ONLY | CL_MEM_HOST_READ_ONLY,  256*sizeof(unsigned int), nullptr,  &status);
        if(status != CL_SUCCESS){ std::cout << "Cannot create output buffer: " << status << "\n"; return -1; }

        cl_event evt[2];

        //NOTE: tehcnically the second kernel is unneeded, but i didn't want to differ from the base, if a problem appears

        //First kernel of global histograms:
        {
            status = clSetKernelArg(kernel_shared_atomics, 0, sizeof(bufferPartials), &bufferPartials); if(status != CL_SUCCESS){ std::cout << "Cannot set kernel 1 arg 0: " << status << "\n"; return -1; }
            status = clSetKernelArg(kernel_shared_atomics, 1, sizeof(bufferInput),    &bufferInput);    if(status != CL_SUCCESS){ std::cout << "Cannot set kernel 1 arg 1: " << status << "\n"; return -1; }
            status = clSetKernelArg(kernel_shared_atomics, 2, sizeof(int),            &size2);          if(status != CL_SUCCESS){ std::cout << "Cannot set kernel 1 arg 2: " << status << "\n"; return -1; }

            size_t kernel_global_size[2] = {(size_t)block_size, (size_t)nBlocksH*block_size};
            size_t kernel_local_size[2] = {(size_t)block_size, (size_t)block_size};
	        status = clEnqueueNDRangeKernel(queue, kernel_shared_atomics, 2, nullptr, kernel_global_size, kernel_local_size, 0, nullptr, &evt[0]);
            if(status != CL_SUCCESS){ std::cout << "Cannot enqueue kernel 3: " << status << "\n"; return -1; }
            if(status == CL_SUCCESS){std::cout << "Histogramm is done:" << status << std::endl; }
        }
        
        //THIS IS FOR ACCUMULATING RESULTS FOR EACH HIST -> I DON'T HAVE MANY, JUST ONE HIST
        //Second kernel: accumulate partial results:
        {
            //args for the function
            status = clSetKernelArg(kernel_accumulate, 0, sizeof(bufferOutput),   &bufferOutput);   if(status != CL_SUCCESS){ std::cout << "Cannot set kernel 2 arg 0: " << status << "\n"; return -1; }
            status = clSetKernelArg(kernel_accumulate, 1, sizeof(bufferPartials), &bufferPartials); if(status != CL_SUCCESS){ std::cout << "Cannot set kernel 2 arg 1: " << status << "\n"; return -1; }
            status = clSetKernelArg(kernel_accumulate, 2, sizeof(int),            &nBlocksH);       if(status != CL_SUCCESS){ std::cout << "Cannot set kernel 2 arg 2: " << status << "\n"; return -1; }

            size_t kernel_global_size[1] = {(size_t)(256)};
	        status = clEnqueueNDRangeKernel(queue, kernel_accumulate, 1, nullptr, kernel_global_size, nullptr, 0, nullptr, &evt[1]);
            if(status != CL_SUCCESS){ std::cout << "Cannot enqueue kernel 4: " << status << "\n"; return -1; }
        }
        
        std::vector<unsigned int> tmp(256);
        
        //reading the buffer!
        status = clEnqueueReadBuffer(queue, bufferOutput, CL_TRUE, 0, 256*sizeof(unsigned int), tmp.data(), 1, &evt[1], nullptr);
        if(status != CL_SUCCESS){ std::cout << "Cannot read buffer: " << status << "\n"; return -1; }
        std::cout << "Reached" << std::endl;

        //time measurement
        cl_ulong t1_0, t1_1;
        status = clGetEventProfilingInfo(evt[0], CL_PROFILING_COMMAND_START, sizeof(t1_0), &t1_0, nullptr);
        status = clGetEventProfilingInfo(evt[1], CL_PROFILING_COMMAND_END,   sizeof(t1_1), &t1_1, nullptr);
        dt2 = (t1_1 - t1_0)*0.001f*0.001f;
        std::cout << "GPU shared atomics computation took: " << dt2 << " ms\n";

        clReleaseEvent(evt[0]);
        clReleaseEvent(evt[1]);

        for(int i=0; i < 256; ++i) {
            std::cout << i << " : " << &tmp[i] << std::endl;
        }

        clReleaseMemObject(bufferInput);
        clReleaseMemObject(bufferPartials);
        clReleaseMemObject(bufferOutput);
        
    }
    
    clReleaseKernel(kernel_shared_atomics);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    clReleaseDevice(device);
    
    return 0;
}

template<typename T>
void save_pic(const std::vector<T> & veced_pic){
    int count = 0;
    std::ofstream my_file("../../../output/big_pic.txt");
    my_file << size1 << " " << size2 << "\n";
    for(int i=0; i!=size1; ++i){
        for(int j=0;j!=size2-1; ++j){
            my_file << veced_pic[i*size1 + j] << " ";
            count++;
        }
        my_file << veced_pic[i*size1 + size2 -1] << "\n";
        count++;
    }
}

template<typename T>
T summer(const std::vector<T> & veced_data,int i1,int i2){
    int sum = 0;
    for(int i=i1; i<=i2; ++i) sum+=veced_data[i].data.load();
    return sum;
}

void fromLinearMemory( std::vector<unsigned int> & input, std::vector<unsigned int> veced){
    for(int i=0; i<256; ++i){
        veced[i] = input[0*256+i];
    }
}