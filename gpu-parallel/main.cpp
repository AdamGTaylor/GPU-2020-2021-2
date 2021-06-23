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
    std::vector<cl_int> atomic_histogram(256,0);
    
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
    static const int blocksize = 16;    //blocksize
    int nBlocksH = 0;                   //number if block vertically
    
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
    //now we have a succesful kernel created!

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