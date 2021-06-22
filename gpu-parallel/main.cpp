#include <vector>
#include <array>
#include <string>
#include <numeric>
#include <algorithm>
#include <random>
#include <chrono>
#include <iostream>
#include <fstream>



#ifdef __APPLE__ //Mac OSX has a different name for the header file
#include <OpenCL/opencl.h>
#else
#include <CL/opencl.h>
#endif



#include <stdio.h>  // printf
#include <stdlib.h> // malloc
#include <stdint.h> // UINTMAX_MAX

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
    /*int CRT_err = 0;
    cl_int CL_err = CL_SUCCESS;
    cl_uint numPlatforms = 0;

    CL_err = clGetPlatformIDs(0, NULL, &numPlatforms);
    checkErr(CL_err, "clGetPlatformIDs(numPlatforms)");

    if (numPlatforms == 0)
    {
        printf("No OpenCL platform detected.\n");
        exit( -1 );
    }
    printf("Found %u platform(s)\n\n", numPlatforms);
    fflush(NULL);

    cl_platform_id* platforms = (cl_platform_id*)malloc(numPlatforms * sizeof(cl_platform_id));
    CL_err = clGetPlatformIDs(numPlatforms, platforms, NULL);
    checkErr(CL_err, "clGetPlatformIDs(platforms)");

    for (cl_uint i = 0; i < numPlatforms; ++i)
    {
        size_t vendor_length;
        CL_err = clGetPlatformInfo(platforms[i], CL_PLATFORM_VENDOR, UINTMAX_MAX, NULL, &vendor_length);
        checkErr(CL_err, "clGetPlatformInfo(CL_PLATFORM_VENDOR, NULL, &vendor_length)");

        char* platform_name = (char*)malloc(vendor_length * sizeof(char));
        CL_err = clGetPlatformInfo(platforms[i], CL_PLATFORM_VENDOR, vendor_length, platform_name, NULL);
        checkErr(CL_err, "clGetPlatformInfo(CL_PLATFORM_VENDOR, vendor_length, platform_name)");

        printf("%s\n", platform_name);
        fflush(NULL);
        free(platform_name);

        cl_uint numDevices = 0;
        clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, 0, NULL, &numDevices);
        checkErr(CL_err, "clGetDeviceIDs(CL_DEVICE_TYPE_ALL, 0, NULL, &numDevices)");

        cl_device_id* devices = (cl_device_id*)malloc(numDevices * sizeof(cl_device_id));
        clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, numDevices, devices, NULL);
        checkErr(CL_err, "clGetDeviceIDs(CL_DEVICE_TYPE_ALL, numDevices, devices, NULL)");

        for (cl_uint j = 0; j < numDevices; ++j)
        {
            size_t device_length;
            CL_err = clGetDeviceInfo(devices[j], CL_DEVICE_NAME, 0, NULL, &device_length);
            checkErr(CL_err, "clGetDeviceInfo(CL_DEVICE_NAME, NULL, &device_length)");

            char* device_name = (char*)malloc(device_length * sizeof(char));
            CL_err = clGetDeviceInfo(devices[j], CL_DEVICE_NAME, device_length, device_name, NULL);
            checkErr(CL_err, "clGetDeviceInfo(devices[j], CL_DEVICE_NAME, device_length, device_name)");

            printf("\t%s\n", device_name);
            fflush(NULL);

            CL_err = clReleaseDevice(devices[j]);
            checkErr(CL_err, "clReleaseDevice(devices[j])");
        }
        free(devices);
    }*/
    cl_int status = CL_SUCCESS;
    cl_uint numPlatforms = 0;
    std::vector<cl_platform_id> platforms;
    std::vector<std::vector<cl_device_id>> devices;
    
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

    return 0;
}