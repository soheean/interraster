#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include "gclFile.h"
#include "gclTimer.h"

using namespace std;
#define BUFSIZE 262144
#define NUMERSIZE 9000000
#define NUMERSIZE1 256

#pragma comment (lib,"OpenCL.lib")

//定义结构体
typedef struct coordinationNode
{
	float X;
	float Y;
	float Z;
} coordination;
cl_program CreateProgram(cl_context context, cl_device_id device, const char* fileName)
{
	cl_int errNum;
	cl_program program;

	std::ifstream kernelFile(fileName, std::ios::in);
	if (!kernelFile.is_open())
	{
		std::cerr << "Failed to open file for reading: " << fileName << std::endl;
		return NULL;
	}

	std::ostringstream oss;
	oss << kernelFile.rdbuf();

	std::string srcStdStr = oss.str();
	const char *srcStr = srcStdStr.c_str();
	program = clCreateProgramWithSource(context, 1,
		(const char**)&srcStr,
		NULL, NULL);
	if (program == NULL)
	{
		std::cerr << "Failed to create CL program from source." << std::endl;
		return NULL;
	}

	errNum = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
	if (errNum != CL_SUCCESS)
	{
		// Determine the reason for the error
		char buildLog[16384];
		clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG,
			sizeof(buildLog), buildLog, NULL);

		std::cerr << "Error in kernel: " << std::endl;
		std::cerr << buildLog;
		clReleaseProgram(program);
		return NULL;
	}

	return program;
}
void Cleanup(cl_context context, cl_command_queue commandQueue,
	cl_program program, cl_kernel kernel)
{
	if (commandQueue != 0)
		clReleaseCommandQueue(commandQueue);

	if (kernel != 0)
		clReleaseKernel(kernel);

	if (program != 0)
		clReleaseProgram(program);

	if (context != 0)
		clReleaseContext(context);
}

//等待事件完成
int waitForEventAndRelease(cl_event *event)
{
	cl_int status = CL_SUCCESS;
	cl_int eventStatus = CL_QUEUED;
	while (eventStatus != CL_COMPLETE)
	{
		status = clGetEventInfo(
			*event,
			CL_EVENT_COMMAND_EXECUTION_STATUS,
			sizeof(cl_int),
			&eventStatus,
			NULL);
	}

	status = clReleaseEvent(*event);

	return 0;
}
int main(int argc, char* argv[])
//int main(void)
{
	//在host内存中创建三个缓冲区
	ifstream myfile("test.txt");
	ifstream myfile1("test.txt");
	ofstream gridout("gridout.txt");
	cl_kernel kernel = 0;
	cl_program program = 0;
	if (!myfile){
		cout << "Unable to open myfile";
		exit(1); // terminate with error  

	}
	if (!gridout){
		cout << "Unable to open gridout";
		exit(1); // terminate with error 
	}
	int pointNum = 1;
	int gridNum = 1;
	float gridx;
	float gridy;
	char c;
	while (myfile1.get(c))
	{
		if (c == '\n')
		{
			pointNum++;
		}
	}
	myfile1.close();
	//coordination dic;
	coordination *dic = new coordination[NUMERSIZE1 * NUMERSIZE1];
	for (int i = 0; i < NUMERSIZE1; i++){
		dic[i].X = 0;
		dic[i].Y = 0;
		dic[i].Z = 0;
	}
	for (int i = 0; i < pointNum; i++){
		for (int j = 0; j < 3; j++)
		{
			if (j == 0)
			{
				myfile >> dic[i].X;
			}
			if (j == 1)
			{
				myfile >> dic[i].Y;
			}
			if (j == 2)
			{
				myfile >> dic[i].Z;
			}
		}
	}
	myfile.close();
	//依据读入的离散点坐标获取插值范围（X及Y的最大值和最小值）
	float xmin, xmax, ymin, ymax;
	xmin = dic[0].X;
	xmax = dic[0].X;
	ymin = dic[0].Y;
	ymax = dic[0].Y;
	for (int i = 1; i < pointNum; i++){
		if (xmin > dic[i].X)
		{
			xmin = dic[i].X;
		}
		if (xmax < dic[i].X)
		{
			xmax = dic[i].X;
		}
		if (ymin > dic[i].Y)
		{
			ymin = dic[i].Y;
		}
		if (ymax < dic[i].Y)
		{
			ymax = dic[i].Y;
		}
	}
	xmin--;
	xmax++;
	ymin--;
	ymax++;

	float stepx;
	float stepy;
	stepx = (xmax - xmin) / NUMERSIZE1;
	stepy = (ymax - ymin) / NUMERSIZE1;

	coordination **grid1 = new coordination*[NUMERSIZE1];//动态分配数组空间
	for (int i = 0; i < NUMERSIZE1; i++)
	{
		grid1[i] = new coordination[NUMERSIZE1];
	}
	coordination *grid = new coordination[NUMERSIZE1 * NUMERSIZE1];//动态分配数组空间
	coordination **martix = new coordination*[NUMERSIZE1];//动态分配数组空间
	for (int i = 0; i < NUMERSIZE1; i++)
	{
		martix[i] = new coordination[NUMERSIZE1];
	}
	//coordination martix[NUMERSIZE1][NUMERSIZE1];
	coordination *martix1 = new coordination[NUMERSIZE1 * NUMERSIZE1];//动态分配数组空间
	for (int i = 0; i < NUMERSIZE1; i++)
	{
		for (int j = 0; j < NUMERSIZE1; j++)
		{
			grid1[i][j].X = xmin + stepx * i;
			grid1[i][j].Y = ymin + stepy * j;
			grid1[i][j].Z = 0;
			grid[i * NUMERSIZE1 + j].X = xmin + stepx * i;
			grid[i * NUMERSIZE1 + j].Y = ymin + stepy * j;
			grid[i * NUMERSIZE1 + j].Z = 0;
		}
	}
	for (int i = 0; i < NUMERSIZE1; i++)
	{
		for (int j = 0; j < NUMERSIZE1; j++)
		{
			martix[i][j].X = 0;
			martix[i][j].Y = 0;
			martix[i][j].Z = 0;
			martix1[i * NUMERSIZE1 + j].X = 0;
			martix1[i * NUMERSIZE1 + j].Y = 0;
			martix1[i * NUMERSIZE1 + j].Z = 0;
		}
	}
	int dicx, dicy;
	for (int i = 0; i < pointNum; ++i)
	{
		dicx = (int)((dic[i].X - xmin) / stepx);
		dicy = (int)((dic[i].Y - ymin) / stepy);
		martix[dicx][dicy].X = dic[i].X;
		martix[dicx][dicy].Y = dic[i].Y;
		martix[dicx][dicy].Z = dic[i].Z;
		martix1[dicx * NUMERSIZE1 + dicy].X = dic[i].X;
		martix1[dicx * NUMERSIZE1 + dicy].Y = dic[i].Y;
		martix1[dicx * NUMERSIZE1 + dicy].Z = dic[i].Z;
	}
	int msn = 5;//最大影响点数
	cl_uint status;
	cl_platform_id platform;
	//创建平台对象
	status = clGetPlatformIDs(1, &platform, NULL);
	cl_uint numPlatforms;
	std::string platformVendor;
	status = clGetPlatformIDs(0, NULL, &numPlatforms);
	if (status != CL_SUCCESS)
	{
		return 0;
	}
	if (0 < numPlatforms)
	{
		cl_platform_id* platforms = new cl_platform_id[numPlatforms];
		status = clGetPlatformIDs(numPlatforms, platforms, NULL);

		char platformName[100];
		for (unsigned i = 0; i < numPlatforms; ++i)
		{
			status = clGetPlatformInfo(platforms[i],
				CL_PLATFORM_VENDOR,
				sizeof(platformName),
				platformName,
				NULL);

			platform = platforms[i];
			platformVendor.assign(platformName);

			if (!strcmp(platformName, "Advanced Micro Devices, Inc."))
			{
				break;
			}
		}
		std::cout << "Platform found : " << platformName << "\n";
		delete[] platforms;
	}
	cl_device_id device;
	//创建GPU设备
	clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU,
		1,
		&device,
		NULL);
	//创建context
	cl_context context = clCreateContext(NULL,
		1,
		&device,
		NULL, NULL, NULL);
	//创建命令队列
	cl_command_queue queue = clCreateCommandQueue(context,
		device,
		CL_QUEUE_PROFILING_ENABLE, NULL);
	//创建三个OpenCL内存对象，并把buf1的内容通过隐式拷贝的方式
	//拷贝到clbuf1,buf2的内容通过显示拷贝的方式拷贝到clbuf2
	cl_mem clbuf1 = clCreateBuffer(context,
		CL_MEM_COPY_HOST_PTR,
		NUMERSIZE1 * NUMERSIZE1 * sizeof(coordination), grid,
		NULL);

	cl_mem clbuf2 = clCreateBuffer(context,
		CL_MEM_COPY_HOST_PTR,
		NUMERSIZE1 * NUMERSIZE1 * sizeof(coordination), martix1,
		NULL);
	unsigned int *outbuffer = new unsigned int[300000];
	memset(outbuffer, 0, 300000);
	cl_mem buffer = clCreateBuffer(context,
		CL_MEM_ALLOC_HOST_PTR,
		BUFSIZE * sizeof(cl_float),
		NULL, NULL);
	// Create OpenCL program from HelloWorld.cl kernel source
	program = CreateProgram(context, device, "tt.cl");
	if (program == NULL)
	{
		Cleanup(context, queue, program, kernel);
		return 1;
	}
	//创建Kernel对象
	kernel = clCreateKernel(program, "tt", NULL);
	//if (kernel == NULL)
	//{
	//	std::cerr << "Failed to create kernel" << std::endl;
	//	Cleanup(context, queue, program, kernel);
	//	return 1;
	//}
	//设置Kernel参数
	cl_int clnum = 3000;
	clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)&clbuf1);
	clSetKernelArg(kernel, 1, sizeof(cl_mem), (void*)&clbuf2);
	clSetKernelArg(kernel, 2, sizeof(int), (void*)&msn);
	clSetKernelArg(kernel, 3, sizeof(cl_mem), (void*)&buffer);

	//执行kernel,Range用1维，work itmes size为BUFSIZE,
	cl_event ev;
	size_t globalThreads[] = { NUMERSIZE1, NUMERSIZE1 };
	size_t localx, localy;
	if (NUMERSIZE1 / 8 > 4)

		localx = 16;

	else if (NUMERSIZE1 < 8)

		localx = NUMERSIZE1;

	else localx = 8;

	if (NUMERSIZE1 / 8 > 4)

		localy = 16;

	else if (NUMERSIZE1 < 8)

		localy = NUMERSIZE1;

	else localy = 8;

	size_t localThreads[] = { localx, localy }; // localx*localy应该是64的倍数
	gclTimer clTimer;
	clTimer.Reset();
	clTimer.Start();
	clEnqueueNDRangeKernel(queue,
		kernel,
		2,
		NULL,
		globalThreads,
		localThreads, 0, NULL, &ev);
	status = clFlush(queue);
	waitForEventAndRelease(&ev);
	//clWaitForEvents(1, &ev);

	clTimer.Stop();
	printf("GPU内核总计时间:%.6f ms \n ", clTimer.GetElapsedTime() * 1000);

	//数据拷回host内存
	cl_float *ptr;
	clTimer.Reset();
	clTimer.Start();
	cl_event mapevt;
	//cl_int sta;
	//sta = clEnqueueReadBuffer(queue,
	//	buffer, CL_TRUE, 0,
	//	0, outbuffer, 0, NULL, NULL);
	ptr = (cl_float *)clEnqueueMapBuffer(queue,
		buffer,
		CL_TRUE,
		CL_MAP_READ,
		0,
		BUFSIZE * sizeof(cl_float),
		0, NULL, &mapevt, NULL);
	status = clFlush(queue);
	waitForEventAndRelease(&mapevt);
	//clWaitForEvents(1, &mapevt);
	clTimer.Stop();
	printf("从设备到主机拷贝:%.6f ms \n ", clTimer.GetElapsedTime() * 1000);

	////输出结果到TXT文件
	for (int i = 0; i < NUMERSIZE1 * NUMERSIZE1; i++)
	{
		gridout << i + 1;
		gridout << "值：";
		gridout << ptr[i];
		gridout << "\n";
		//	std::cout << "result：" << "第" << i + 1 << "个"<< ptr[i] << "\n";	
	}

	//删除OpenCL资源对象
	clReleaseMemObject(clbuf1);
	clReleaseMemObject(clbuf2);
	clReleaseMemObject(buffer);
	clReleaseKernel(kernel);
	clReleaseProgram(program);
	clReleaseCommandQueue(queue);
	clReleaseContext(context);
	gridout.close();
	delete dic;
	delete outbuffer;
	for (int i = 0; i != NUMERSIZE1; i++)
	{
		delete[] martix[i];
	}
	delete[] martix;
	delete[] martix1;
	delete[] grid;
	for (int i = 0; i != NUMERSIZE1; i++)
	{
		delete[] grid1[i];
	}
	delete[] grid1;

	return 0;
}