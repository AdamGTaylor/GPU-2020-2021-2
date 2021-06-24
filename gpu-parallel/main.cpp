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

    std::vector<unsigned int> atomic_histogram(256,0);
    std::vector<cl_int> cdf(256,0);        //point mass function of pic
    std::vector<cl_int> h_v(256,0);        //new values for eq_pic created from eq

    int V_MIN = 0;
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
    static const int block_size = 6;    //blocksize    
    //YE DIVISION WITH 4 IS NOT LIKED
    double ejnye = (size2 / block_size);
    std::cout << ejnye << std::endl; 
    int nBlocksH = size2 / block_size;  //number if block vertically
    
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

    auto kernel_cdf = clCreateKernel(program, "gpu_global_cdf", &status);
    if(status != CL_SUCCESS){ std::cout << "Cannot create kernel 'gpu_global_cdf': " << status << "\n"; return -1; }

    auto kernel_hv = clCreateKernel(program, "gpu_global_h_v", &status);
    if(status != CL_SUCCESS){ std::cout << "Cannot create kernel 'gpu_global_h_v': " << status << "\n"; return -1; }
    //now we have a succesful kernel created!

    auto kernel_eq = clCreateKernel(program, "gpu_global_eq", &status);
    if(status != CL_SUCCESS){ std::cout << "Cannot create kernel 'gpu_global_eq': " << status << "\n"; return -1; }
    //now we have a succesful kernel created!
    
    float dt1 = 0.0f; float dt2 = 0.0f; float dt3 = 0.0f; float dt4 = 0.0f; float dt5 = 0.0f;

    {   
        //input buffer has to be the size of pic! i have to give the pic pointer too
        auto bufferInput    = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, size1*size2*sizeof(cl_int), pic.data(), &status);
        if(status != CL_SUCCESS){ std::cout << "Cannot create input buffer: " << status << "\n"; return -1; }
        
        //buffer of partials has to be the size of bufferinput * block_num
        auto bufferPartials = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_HOST_READ_ONLY,  nBlocksH*256*sizeof(unsigned int), nullptr,  &status);
        if(status != CL_SUCCESS){ std::cout << "Cannot create partial buffer: " << status << "\n"; return -1; }

        auto bufferHist  = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_HOST_READ_ONLY,  256*sizeof(unsigned int), nullptr,  &status);
        if(status != CL_SUCCESS){ std::cout << "Cannot create output buffer: " << status << "\n"; return -1; }

        auto bufferCDF  = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_HOST_READ_ONLY, 256*sizeof(unsigned int), nullptr, &status);
        if(status != CL_SUCCESS){ std::cout << "Cannot create CDF buffer: " << status << "\n"; return -1; }

        auto bufferHV = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_HOST_READ_ONLY, 256*sizeof(int), nullptr, &status);
        if(status != CL_SUCCESS){ std::cout << "Cannot create HV buffer: " << status << "\n"; return -1; }

        auto bufferOutput   = clCreateBuffer(context,  CL_MEM_READ_WRITE | CL_MEM_HOST_READ_ONLY, size1*size2*sizeof(cl_int), nullptr, &status);
        if(status != CL_SUCCESS){ std::cout << "Cannot create input buffer: " << status << "\n"; return -1; }

        //filling the buffer with zeros
        cl_int zero = 0;
        status = clEnqueueFillBuffer(queue, bufferPartials, &zero, sizeof(cl_int), 0, nBlocksH*256*sizeof(unsigned int), 0, nullptr, nullptr);
        if(status != CL_SUCCESS){ std::cout << "Cannot zero partial buffer: " << status << "\n"; return -1; }

        status = clFinish(queue);
        if(status != CL_SUCCESS){ std::cout << "Cannot finish queue: " << status << "\n"; return -1; }

        cl_event evt[5];

        //NOTE: tehcnically the second kernel is unneeded, but i didn't want to differ from the base, if a problem appears

        //First kernel of global histograms:
        {
            status = clSetKernelArg(kernel_shared_atomics, 0, sizeof(bufferPartials), &bufferPartials); if(status != CL_SUCCESS){ std::cout << "Cannot set kernel 1 arg 0: " << status << "\n"; return -1; }
            status = clSetKernelArg(kernel_shared_atomics, 1, sizeof(bufferInput),    &bufferInput);    if(status != CL_SUCCESS){ std::cout << "Cannot set kernel 1 arg 1: " << status << "\n"; return -1; }
            status = clSetKernelArg(kernel_shared_atomics, 2, sizeof(int),            &size2);          if(status != CL_SUCCESS){ std::cout << "Cannot set kernel 1 arg 2: " << status << "\n"; return -1; }

            size_t kernel_global_size[2] = {(size_t)block_size, (size_t)nBlocksH*block_size};
            size_t kernel_local_size[2] = {(size_t)block_size, (size_t)block_size};
	        status = clEnqueueNDRangeKernel(queue, kernel_shared_atomics, 2, nullptr, kernel_global_size, kernel_local_size, 0, nullptr, &evt[0]);
            if(status != CL_SUCCESS){ std::cout << "Cannot enqueue kernel 1: " << status << "\n"; return -1; }
        }
        
        //THIS IS FOR ACCUMULATING RESULTS FOR EACH HIST -> I DON'T HAVE MANY, JUST ONE HIST
        //Second kernel: accumulate partial results for hist
        {
            //args for the function
            status = clSetKernelArg(kernel_accumulate, 0, sizeof(bufferHist),     &bufferHist);   if(status != CL_SUCCESS){ std::cout << "Cannot set kernel 2 arg 0: " << status << "\n"; return -1; }
            status = clSetKernelArg(kernel_accumulate, 1, sizeof(bufferPartials), &bufferPartials); if(status != CL_SUCCESS){ std::cout << "Cannot set kernel 2 arg 1: " << status << "\n"; return -1; }
            status = clSetKernelArg(kernel_accumulate, 2, sizeof(int),            &nBlocksH);       if(status != CL_SUCCESS){ std::cout << "Cannot set kernel 2 arg 2: " << status << "\n"; return -1; }

            //accumulate, due to the blocks -> if one block, there is no need for this.
            size_t kernel_global_size[1] = {(size_t)(256)};
	        status = clEnqueueNDRangeKernel(queue, kernel_accumulate, 1, nullptr, kernel_global_size, nullptr, 0, nullptr, &evt[1]);
            if(status != CL_SUCCESS){ std::cout << "Cannot enqueue kernel 2: " << status << "\n"; return -1; }
        }
        //copy hit
        status = clEnqueueReadBuffer(queue, bufferHist, CL_TRUE, 0, 256*sizeof(unsigned int), atomic_histogram.data(), 1, &evt[1], nullptr);
        if(status != CL_SUCCESS){ std::cout << "Cannot read buffer: " << status << "\n"; return -1; }

        //Third kernel to generate CDF
        {
            status = clSetKernelArg(kernel_cdf, 0, sizeof(bufferCDF),    &bufferCDF);   if(status != CL_SUCCESS){ std::cout << "Cannot set kernel 3 arg 0: " << status << "\n"; return -1; }
            status = clSetKernelArg(kernel_cdf, 1, sizeof(bufferHist),   &bufferHist);  if(status != CL_SUCCESS){ std::cout << "Cannot set kernel 3 arg 1: " << status << "\n"; return -1; }
        
            size_t kernel_global_size2[1] = {(size_t)(256)};
            status = clEnqueueNDRangeKernel(queue, kernel_cdf, 1, nullptr, kernel_global_size2, nullptr, 0, nullptr, &evt[2]);
            if(status != CL_SUCCESS){ std::cout << "Cannot enqueue kernel 3: " << status << "\n"; return -1; }
        }
        status = clEnqueueReadBuffer(queue, bufferCDF, CL_TRUE, 0, 256*sizeof(unsigned int), cdf.data(), 1, &evt[2], nullptr);
        bool found = true;
        for(int i=0; found && i!=cdf.size();++i){
            if(atomic_histogram[i]!=0){
                V_MIN = cdf[i];
                found = false;
            }
        }
        std::cout << "V_MIN as " << V_MIN << std::endl; 
        //Fourth kernel to generate H_V
        {
            status = clSetKernelArg(kernel_hv, 0, sizeof(bufferHV),    &bufferHV);    if(status != CL_SUCCESS){ std::cout << "Cannot set kernel 4 arg 0: " << status << "\n"; return -1; }
            status = clSetKernelArg(kernel_hv, 1, sizeof(bufferCDF),   &bufferCDF);   if(status != CL_SUCCESS){ std::cout << "Cannot set kernel 4 arg 1: " << status << "\n"; return -1; }
            status = clSetKernelArg(kernel_hv, 2, sizeof(int),         &V_MIN);       if(status != CL_SUCCESS){ std::cout << "Cannot set kernel 4 arg 2: " << status << "\n"; return -1; }
            status = clSetKernelArg(kernel_hv, 3, sizeof(int),         &size1);       if(status != CL_SUCCESS){ std::cout << "Cannot set kernel 4 arg 3: " << status << "\n"; return -1; }
            status = clSetKernelArg(kernel_hv, 4, sizeof(int),         &size2);       if(status != CL_SUCCESS){ std::cout << "Cannot set kernel 4 arg 4: " << status << "\n"; return -1; }

            size_t kernel_global_size3[1] = {(size_t)(256)};
            status = clEnqueueNDRangeKernel(queue, kernel_hv, 1, nullptr, kernel_global_size3, nullptr, 0, nullptr, &evt[3]);
            if(status != CL_SUCCESS){ std::cout << "Cannot enqueue kernel 4: " << status << "\n"; return -1; }
        }
        status = clEnqueueReadBuffer(queue, bufferHV, CL_TRUE, 0, 256*sizeof(int), h_v.data(), 1, &evt[3], nullptr);

        //Fifth kernel to generate eq_pic
        {
            status = clSetKernelArg(kernel_eq, 0, sizeof(bufferOutput),    &bufferOutput);    if(status != CL_SUCCESS){ std::cout << "Cannot set kernel 5 arg 0: " << status << "\n"; return -1; }
            status = clSetKernelArg(kernel_eq, 1, sizeof(bufferInput),     &bufferInput);     if(status != CL_SUCCESS){ std::cout << "Cannot set kernel 5 arg 1: " << status << "\n"; return -1; }
            status = clSetKernelArg(kernel_eq, 2, sizeof(bufferHV),        &bufferHV);        if(status != CL_SUCCESS){ std::cout << "Cannot set kernel 5 arg 2: " << status << "\n"; return -1; }
            status = clSetKernelArg(kernel_eq, 3, sizeof(int),             &size1);           if(status != CL_SUCCESS){ std::cout << "Cannot set kernel 5 arg 3: " << status << "\n"; return -1; }

            size_t kernel_global_size4[2] = {(size_t)size1,(size_t)size2};
            status = clEnqueueNDRangeKernel(queue, kernel_eq, 2, nullptr, kernel_global_size4, nullptr, 0, nullptr, &evt[4]);
            if(status != CL_SUCCESS){ std::cout << "Cannot enqueue kernel 5: " << status << "\n"; return -1; }
        }

        status = clEnqueueReadBuffer(queue, bufferOutput, CL_TRUE, 0, size1*size2*sizeof(unsigned int), eq_pic.data(), 1, &evt[4], nullptr);



        //time measurement
        cl_ulong t1_0, t1_1, t2_0, t2_1, t3_0, t3_1, t4_0,t4_1,t5_0,t5_1;
        status = clGetEventProfilingInfo(evt[0], CL_PROFILING_COMMAND_START, sizeof(t1_0), &t1_0, nullptr);
        status = clGetEventProfilingInfo(evt[0], CL_PROFILING_COMMAND_END,   sizeof(t1_1), &t1_1, nullptr);
        status = clGetEventProfilingInfo(evt[1], CL_PROFILING_COMMAND_START, sizeof(t2_0), &t2_0, nullptr);
        status = clGetEventProfilingInfo(evt[1], CL_PROFILING_COMMAND_END,   sizeof(t2_1), &t2_1, nullptr);
        status = clGetEventProfilingInfo(evt[2], CL_PROFILING_COMMAND_START, sizeof(t3_0), &t3_0, nullptr);
        status = clGetEventProfilingInfo(evt[2], CL_PROFILING_COMMAND_END,   sizeof(t3_1), &t3_1, nullptr);
        status = clGetEventProfilingInfo(evt[3], CL_PROFILING_COMMAND_START, sizeof(t4_0), &t4_0, nullptr);
        status = clGetEventProfilingInfo(evt[3], CL_PROFILING_COMMAND_END,   sizeof(t4_1), &t4_1, nullptr);
        status = clGetEventProfilingInfo(evt[4], CL_PROFILING_COMMAND_START, sizeof(t5_0), &t5_0, nullptr);
        status = clGetEventProfilingInfo(evt[4], CL_PROFILING_COMMAND_END,   sizeof(t5_1), &t5_1, nullptr);
        dt1 = (t1_1 - t1_0)*0.001f*0.001f;
        dt2 = (t2_1 - t2_0)*0.001f*0.001f;
        dt3 = (t3_1 - t3_0)*0.001f*0.001f;
        dt4 = (t4_1 - t4_0)*0.001f*0.001f;
        dt5 = (t5_1 - t5_0)*0.001f*0.001f;
        std::cout << "GPU Histogramm Equalization computation took: \t" << dt1 << " + " << dt2 << " + " << dt3 << " + " << dt4 << " + " << dt5 << " ms\n";
        std::cout << "\t\t\t\t\tSum\t" << dt1+dt2+dt3+dt4+dt5 << " ms\n";



        /*
        for(int i=0; i < atomic_histogram.size(); ++i){
            std::cout<< i << " : " << atomic_histogram[i] << " : " << cdf[i] << " : " << h_v[i] <<std::endl;
        }
        */
        //there has to be a mor elegant way to do this...
        clReleaseEvent(evt[0]);
        clReleaseEvent(evt[1]);
        clReleaseEvent(evt[2]);
        clReleaseEvent(evt[3]);
        clReleaseEvent(evt[4]);
        //release stuf from previous kernels
        clReleaseMemObject(bufferHist);
        clReleaseMemObject(bufferCDF);
        clReleaseMemObject(bufferInput);
        clReleaseMemObject(bufferPartials);
        
    }
    
    clReleaseKernel(kernel_shared_atomics);
    clReleaseKernel(kernel_accumulate);
    clReleaseKernel(kernel_cdf);
    clReleaseKernel(kernel_hv);
    clReleaseKernel(kernel_eq);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    clReleaseDevice(device);

    save_pic(eq_pic);
    
    return 0;
}

template<typename T>
void save_pic(const std::vector<T> & veced_pic){
    int count = 0;
    std::ofstream my_file("E:/_ELTE_PHYS_MSC/2_second_semester/gpu/project/gpu-parallel/output/test_pic.txt");
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