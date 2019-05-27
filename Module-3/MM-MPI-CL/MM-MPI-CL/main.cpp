//
//  main.cpp
//  MM-MPI-CL
//
//  Created by Brett Best on 27/5/19.
//  Copyright © 2019 Brett Best. All rights reserved.
//


#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#define CL_SILENCE_DEPRECATION
#include <OpenCL/opencl.h>

////////////////////////////////////////////////////////////////////////////////

// Use a static data size for simplicity
//
#define DATA_SIZE (1024)

////////////////////////////////////////////////////////////////////////////////

// Simple compute kernel which computes the square of an input array
//
const char *KernelSource = "\n" \
"__kernel void square(                                                       \n" \
"   __global float* input,                                              \n" \
"   __global float* output,                                             \n" \
"   const unsigned int count)                                           \n" \
"{                                                                      \n" \
"   int i = get_global_id(0);                                           \n" \
"   if(i < count)                                                       \n" \
"       output[i] = input[i] * input[i];                                \n" \
"}                                                                      \n" \
"\n";

////////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <stdio.h>
#include <thread>
#include <fstream>
#include <mpi.h>

using namespace std;

#define MATRIX_SIZE 1000
#define MAX_VALUE 1'000'000UL
#define matrix unsigned long long

matrix *matrixA;
matrix *matrixB;
matrix *matrixC;

struct timespec startTimespec, endTimespec;

void generateMatrices() {
  int row, column;
  
  for (row = 0; row < MATRIX_SIZE; row++) {
    for (column = 0; column < MATRIX_SIZE; column++) {
      matrixA[row+(column*MATRIX_SIZE)] = rand() % MAX_VALUE;
      matrixB[row+(column*MATRIX_SIZE)] = rand() % MAX_VALUE;
    }
  }
}

static void loadBalance(int &from, int &to, int &bufferSize, int worldRank, int worldSize) {
  int maxChunkSize = max(MATRIX_SIZE / worldSize, 0);
  from = maxChunkSize * worldRank;
  to = (worldRank + 1) == worldSize ? MATRIX_SIZE : maxChunkSize * (worldRank + 1);
  int rowsToMultiply = to-from;
  bufferSize = rowsToMultiply*MATRIX_SIZE;
}

static void createGathervDistribution(int *&displs, int *&recvCounts, int worldSize, bool shouldPrint) {
  recvCounts = (int*)calloc(worldSize, sizeof(int));
  displs = (int*)calloc(worldSize, sizeof(int));
  
  int accumulatedBufferSize = 0;
  
  for (int worldRank = 0; worldRank < worldSize; worldRank++) {
    int from, to, bufferSize;
    loadBalance(from, to, bufferSize, worldRank, worldSize);
    
    if (shouldPrint) printf("[MM] Buffer Size: %i, Accum. Buffer Size: %i, From: %i, To: %i\n", bufferSize, accumulatedBufferSize, from, to);
    
    recvCounts[worldRank] = bufferSize;
    displs[worldRank] = accumulatedBufferSize;
    
    accumulatedBufferSize += bufferSize;
  }
}

void multiplyMatrices(int worldRank, int worldSize) {
  matrix *matrixBuffer;
  int row, column, offset;
  int from;
  int to;
  int bufferSize;
  
  loadBalance(from, to, bufferSize, worldRank, worldSize);
  
  int * recvCounts;
  int * displs;
  createGathervDistribution(displs, recvCounts, worldSize, 0 == worldRank);
  
  if (0 == bufferSize) {
    printf("[MM] Thread ID: %i, unneeded.\n", worldRank);
  } else {
    
    matrixBuffer = (matrix*)calloc(bufferSize, sizeof(matrix));
    
    for (row = from; row < to; row++) {
      int index = row-from;
      for (column = 0; column < MATRIX_SIZE; column++) {
        int bufferIndex = column+(index*MATRIX_SIZE);
        matrixBuffer[bufferIndex] = 0;
        for (offset = 0; offset < MATRIX_SIZE; offset++) {
          matrixBuffer[bufferIndex] += matrixA[column+(offset*MATRIX_SIZE)] * matrixB[offset+(row*MATRIX_SIZE)];
        }
      }
    }
  }
  
  MPI_Gatherv(matrixBuffer, recvCounts[worldRank], MPI_UNSIGNED_LONG_LONG, matrixC, recvCounts, displs, MPI_UNSIGNED_LONG_LONG, 0, MPI_COMM_WORLD);
}

void writeMatriceToDisk(string name, matrix matrice[MATRIX_SIZE], ofstream *outputFileStream) {
  *outputFileStream << "# Matrice: " << name << endl << endl;
  
  int row, column;
  
  for (row = 0; row < MATRIX_SIZE; row++) {
    for (column = 0; column < MATRIX_SIZE; column++) {
      *outputFileStream << matrice[row+(column*MATRIX_SIZE)];
      
      if (column != MATRIX_SIZE - 1) {
        *outputFileStream << ",";
      }
    }
    
    *outputFileStream << endl;
  }
  
  *outputFileStream << endl << endl;
}

void writeMatricesToDisk() {
  ofstream outputFileStream("/Volumes/Shared Folders/Module-3/DerivedData/MPI Matrix Multiplication/Build/Products/Debug/MM-MPI-P.txt");
  
  if (outputFileStream.is_open()) {
    writeMatriceToDisk("A", matrixA, &outputFileStream);
    writeMatriceToDisk("B", matrixB, &outputFileStream);
    writeMatriceToDisk("C = A * B", matrixC, &outputFileStream);
    
    outputFileStream.close();
  } else {
    printf("Failed to write output to file!");
  }
}

void printThreadInfo() {
  unsigned int hardwareConcurrency = thread::hardware_concurrency();
  cout << "Number of cores: " << hardwareConcurrency << endl;
}

void startTimer() {
  clock_gettime(CLOCK_MONOTONIC, &startTimespec);
}

void stopTimer() {
  clock_gettime(CLOCK_MONOTONIC, &endTimespec);
}

double durationBetweenTimers() {
  double seconds = (endTimespec.tv_sec - startTimespec.tv_sec);
  seconds += (endTimespec.tv_nsec - startTimespec.tv_nsec) / 1000000000.0;
  
  return seconds;
}

int main(int argc, const char * argv[]) {
  //  srand((int)time(NULL));
//  srand(0);
//
  MPI_Init(NULL, NULL);

  int worldSize = 0;
  MPI_Comm_size(MPI_COMM_WORLD, &worldSize);

  int worldRank = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &worldRank);

  char processorName[MPI_MAX_PROCESSOR_NAME];
  int nameLen;
  MPI_Get_processor_name(processorName, &nameLen);

  printf("Processor: %s, rank: %d out of %d processors\n", processorName, worldRank, worldSize);

  printThreadInfo();
//
//  startTimer();
//
//  matrixA = (matrix*)calloc(MATRIX_SIZE*MATRIX_SIZE, sizeof(matrix));
//  matrixB = (matrix*)calloc(MATRIX_SIZE*MATRIX_SIZE, sizeof(matrix));
//  matrixC = (matrix*)calloc(MATRIX_SIZE*MATRIX_SIZE, sizeof(matrix));
//
//  if (worldRank == 0) generateMatrices();
//
//  stopTimer();
//
//  printf("Generate Matrices: %.3fs\n", durationBetweenTimers());
//
//  startTimer();
//
//  MPI_Bcast(matrixA, MATRIX_SIZE*MATRIX_SIZE, MPI_UNSIGNED_LONG_LONG, 0, MPI_COMM_WORLD);
//  MPI_Bcast(matrixB, MATRIX_SIZE*MATRIX_SIZE, MPI_UNSIGNED_LONG_LONG, 0, MPI_COMM_WORLD);
//
//  MPI_Barrier(MPI_COMM_WORLD);
//
//  stopTimer();
//
//  printf("Broadcast Matrices: %.3fs\n", durationBetweenTimers());
//
//  if (worldRank == 0) startTimer();
//
//  multiplyMatrices(worldRank, worldSize);
//
//  MPI_Barrier(MPI_COMM_WORLD);
//
//  if (worldRank == 0) {
//    stopTimer();
//    printf("Multiply Matrices: %.3fs\n", durationBetweenTimers());
//  }
//
//  if (worldRank == 0) {
//    startTimer();
//
//    writeMatricesToDisk();
//
//    stopTimer();
//
//    printf("Write Output: %.3fs\n", durationBetweenTimers());
//  }
//

  
  
  
  
  
  
  
  
  
  
  int err;                            // error code returned from api calls
  
  float data[DATA_SIZE];              // original data set given to device
  float results[DATA_SIZE];           // results returned from device
  unsigned int correct;               // number of correct results returned
  
  size_t global;                      // global domain size for our calculation
  size_t local;                       // local domain size for our calculation
  
  cl_device_id device_id;             // compute device id
  cl_context context;                 // compute context
  cl_command_queue commands;          // compute command queue
  cl_program program;                 // compute program
  cl_kernel kernel;                   // compute kernel
  
  cl_mem input;                       // device memory used for the input array
  cl_mem output;                      // device memory used for the output array
  
  // Fill our data set with random float values
  //
  int i = 0;
  unsigned int count = DATA_SIZE;
  for(i = 0; i < count; i++)
    data[i] = rand() / (float)RAND_MAX;
  
  // Connect to a compute device
  //
  int gpu = worldRank == 0 ? 1 : 0;
  err = clGetDeviceIDs(NULL, gpu ? CL_DEVICE_TYPE_GPU : CL_DEVICE_TYPE_CPU, 1, &device_id, NULL);
  if (err != CL_SUCCESS)
  {
    printf("Error: Failed to create a device group!\n");
    return EXIT_FAILURE;
  }
  
  // Create a compute context
  //
  context = clCreateContext(0, 1, &device_id, NULL, NULL, &err);
  if (!context)
  {
    printf("Error: Failed to create a compute context!\n");
    return EXIT_FAILURE;
  }
  
  // Create a command commands
  //
  commands = clCreateCommandQueue(context, device_id, 0, &err);
  if (!commands)
  {
    printf("Error: Failed to create a command commands!\n");
    return EXIT_FAILURE;
  }
  
  // Create the compute program from the source buffer
  //
  program = clCreateProgramWithSource(context, 1, (const char **) & KernelSource, NULL, &err);
  if (!program)
  {
    printf("Error: Failed to create compute program!\n");
    return EXIT_FAILURE;
  }
  
  // Build the program executable
  //
  err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
  if (err != CL_SUCCESS)
  {
    size_t len;
    char buffer[2048];
    
    printf("Error: Failed to build program executable!\n");
    clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
    printf("%s\n", buffer);
    exit(1);
  }
  
  // Create the compute kernel in the program we wish to run
  //
  kernel = clCreateKernel(program, "square", &err);
  if (!kernel || err != CL_SUCCESS)
  {
    printf("Error: Failed to create compute kernel!\n");
    exit(1);
  }
  
  // Create the input and output arrays in device memory for our calculation
  //
  input = clCreateBuffer(context,  CL_MEM_READ_ONLY,  sizeof(float) * count, NULL, NULL);
  output = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(float) * count, NULL, NULL);
  if (!input || !output)
  {
    printf("Error: Failed to allocate device memory!\n");
    exit(1);
  }
  
  // Write our data set into the input array in device memory
  //
  err = clEnqueueWriteBuffer(commands, input, CL_TRUE, 0, sizeof(float) * count, data, 0, NULL, NULL);
  if (err != CL_SUCCESS)
  {
    printf("Error: Failed to write to source array!\n");
    exit(1);
  }
  
  // Set the arguments to our compute kernel
  //
  err = 0;
  err  = clSetKernelArg(kernel, 0, sizeof(cl_mem), &input);
  err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &output);
  err |= clSetKernelArg(kernel, 2, sizeof(unsigned int), &count);
  if (err != CL_SUCCESS)
  {
    printf("Error: Failed to set kernel arguments! %d\n", err);
    exit(1);
  }
  
  // Get the maximum work group size for executing the kernel on the device
  //
  err = clGetKernelWorkGroupInfo(kernel, device_id, CL_KERNEL_WORK_GROUP_SIZE, sizeof(local), &local, NULL);
  if (err != CL_SUCCESS)
  {
    printf("Error: Failed to retrieve kernel work group info! %d\n", err);
    exit(1);
  }
  
  // Execute the kernel over the entire range of our 1d input data set
  // using the maximum number of work group items for this device
  //
  global = count;
  err = clEnqueueNDRangeKernel(commands, kernel, 1, NULL, &global, &local, 0, NULL, NULL);
  if (err)
  {
    printf("Error: Failed to execute kernel!\n");
    return EXIT_FAILURE;
  }
  
  // Wait for the command commands to get serviced before reading back results
  //
  clFinish(commands);
  
  // Read back the results from the device to verify the output
  //
  err = clEnqueueReadBuffer( commands, output, CL_TRUE, 0, sizeof(float) * count, results, 0, NULL, NULL );
  if (err != CL_SUCCESS)
  {
    printf("Error: Failed to read output array! %d\n", err);
    exit(1);
  }
  
  // Validate our results
  //
  correct = 0;
  for(i = 0; i < count; i++)
  {
    if(results[i] == data[i] * data[i])
      correct++;
  }
  
  // Print a brief summary detailing the results
  //
  printf("Computed '%d/%d' correct values!\n", correct, count);
  
  // Shutdown and cleanup
  //
  clReleaseMemObject(input);
  clReleaseMemObject(output);
  clReleaseProgram(program);
  clReleaseKernel(kernel);
  clReleaseCommandQueue(commands);
  clReleaseContext(context);
  
  
  
  
  
  
  
  
  
  MPI_Barrier(MPI_COMM_WORLD);
  MPI_Finalize();
  
  
  return 0;
}
