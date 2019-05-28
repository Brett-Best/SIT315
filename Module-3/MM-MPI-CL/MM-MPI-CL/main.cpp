//
//  main.cpp
//  MM-MPI-CL
//
//  Created by Brett Best on 27/5/19.
//  Copyright © 2019 Brett Best. All rights reserved.
//

#include <iostream>
#include <stdio.h>
#include <thread>
#include <fstream>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#define CL_SILENCE_DEPRECATION
#include <OpenCL/cl.h>

using namespace std;

#define MATRIX_SIZE 32
#define MAX_VALUE 32
#define matrix int

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
  ofstream outputFileStream("/Volumes/Shared Folders/Module-3/DerivedData/MPI Matrix Multiplication/Build/Products/Debug/MM-MPI-CL.txt");

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

cl_device_id openCLDeviceID;
cl_context openCLContext;
cl_program openCLProgram;
cl_kernel openCLKernel;
cl_command_queue openCLCommandQueue;

cl_event openCLEvent = NULL;
int openCLError;

cl_mem matrixABuffer, matrixBBuffer, matrixCBuffer;
const int matrixSize = MATRIX_SIZE;
const int TS = 4;
const size_t local[2] = { TS, TS };
const size_t global[2] = { matrixSize, matrixSize };

cl_device_id createOpenCLDevice();
cl_program createOpenCLProgram(cl_context ctx, cl_device_id dev, const char* filename);

void setupOpenCLDeviceContextQueueKernel();
void configureKernalMemory();
void setKernelArgs();
void releaseOpenCLComponents();

int main() {
//    srand((int)time(NULL));
  srand(0);

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

  startTimer();

  matrixA = (matrix*)calloc(MATRIX_SIZE * MATRIX_SIZE, sizeof(matrix));
  matrixB = (matrix*)calloc(MATRIX_SIZE * MATRIX_SIZE, sizeof(matrix));
  matrixC = (matrix*)calloc(MATRIX_SIZE * MATRIX_SIZE, sizeof(matrix));

  if (worldRank == 0) generateMatrices();

  stopTimer();

  printf("Generate Matrices: %.3fs\n", durationBetweenTimers());

  startTimer();

  MPI_Bcast(matrixA, MATRIX_SIZE*MATRIX_SIZE, MPI_UNSIGNED_LONG_LONG, 0, MPI_COMM_WORLD);
  MPI_Bcast(matrixB, MATRIX_SIZE*MATRIX_SIZE, MPI_UNSIGNED_LONG_LONG, 0, MPI_COMM_WORLD);

  MPI_Barrier(MPI_COMM_WORLD);

  stopTimer();

  printf("Broadcast Matrices: %.3fs\n", durationBetweenTimers());

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

  //////

  if (worldRank == 0) {

    setupOpenCLDeviceContextQueueKernel();
    configureKernalMemory();
    setKernelArgs();

    clEnqueueNDRangeKernel(openCLCommandQueue, openCLKernel, 2, NULL, global, local, 0, NULL, &openCLEvent);

    if (NULL == openCLEvent) {
      printf("\n\nNULL OPEN CL EVENT\n\n");
    }

    clWaitForEvents(1, &openCLEvent);

    //copying data from the device back to host c matrix
    clEnqueueReadBuffer(openCLCommandQueue, matrixCBuffer, CL_TRUE, 0, MATRIX_SIZE * MATRIX_SIZE * sizeof(matrix), matrixC, 0, NULL, NULL);

    releaseOpenCLComponents();


  }
  //////

  if (worldRank == 0) {
    startTimer();

    writeMatricesToDisk();

    stopTimer();

    printf("Write Output: %.3fs\n", durationBetweenTimers());
  }

  MPI_Barrier(MPI_COMM_WORLD);
  MPI_Finalize();
}

void releaseOpenCLComponents() {
  clReleaseKernel(openCLKernel);
  clReleaseMemObject(matrixABuffer);
  clReleaseMemObject(matrixBBuffer);
  clReleaseMemObject(matrixCBuffer);

  clReleaseCommandQueue(openCLCommandQueue);
  clReleaseProgram(openCLProgram);
  clReleaseContext(openCLContext);
}

void setKernelArgs() {
  clSetKernelArg(openCLKernel, 0, sizeof(matrix), (void*)&matrixSize);
  clSetKernelArg(openCLKernel, 1, sizeof(matrix), (void*)&matrixSize);
  clSetKernelArg(openCLKernel, 2, sizeof(matrix), (void*)&matrixSize);
  clSetKernelArg(openCLKernel, 3, sizeof(cl_mem), (void*)&matrixABuffer);
  clSetKernelArg(openCLKernel, 4, sizeof(cl_mem), (void*)&matrixBBuffer);
  clSetKernelArg(openCLKernel, 5, sizeof(cl_mem), (void*)&matrixCBuffer);

  if (openCLError < 0) {
    perror("Couldn't create a kernel argument");
    printf("error = %d", openCLError);
    exit(1);
  }
}
void configureKernalMemory() {
  matrixABuffer = clCreateBuffer(openCLContext, CL_MEM_READ_ONLY,  MATRIX_SIZE*MATRIX_SIZE*sizeof(matrix), NULL, NULL);
  matrixBBuffer = clCreateBuffer(openCLContext, CL_MEM_READ_ONLY,  MATRIX_SIZE*MATRIX_SIZE*sizeof(matrix), NULL, NULL);
  matrixCBuffer = clCreateBuffer(openCLContext, CL_MEM_READ_WRITE, MATRIX_SIZE*MATRIX_SIZE*sizeof(matrix), NULL, NULL);

  // Copy matrices to the GPU
  clEnqueueWriteBuffer(openCLCommandQueue, matrixABuffer, CL_TRUE, 0, MATRIX_SIZE*MATRIX_SIZE*sizeof(matrix), matrixA, 0, NULL, NULL);
  clEnqueueWriteBuffer(openCLCommandQueue, matrixBBuffer, CL_TRUE, 0, MATRIX_SIZE*MATRIX_SIZE*sizeof(matrix), matrixB, 0, NULL, NULL);
  clEnqueueWriteBuffer(openCLCommandQueue, matrixCBuffer, CL_TRUE, 0, MATRIX_SIZE*MATRIX_SIZE*sizeof(matrix), matrixC, 0, NULL, NULL);
}

void setupOpenCLDeviceContextQueueKernel() {
  openCLDeviceID = createOpenCLDevice();
  cl_int err;
  openCLContext = clCreateContext(NULL, 1, &openCLDeviceID, NULL, NULL, &err);

  if (err < 0) {
    perror("Couldn't create a context");
    exit(1);
  }

  openCLProgram = createOpenCLProgram(openCLContext, openCLDeviceID, "/Volumes/Shared Folders/Module-3/DerivedData/MPI Matrix Multiplication/Build/Products/Debug/matrix_mul.cl");
  openCLCommandQueue = clCreateCommandQueue(openCLContext, openCLDeviceID, 0, &err);

  if (err < 0) {
    perror("Couldn't create a command queue");
    exit(1);
  }

  openCLKernel = clCreateKernel(openCLProgram, "multiply_matrices", &err);

  if (err < 0) {
    perror("Couldn't create a kernel");
    printf("error =%d", err);
    exit(1);
  }
}

cl_program createOpenCLProgram(cl_context ctx, cl_device_id dev, const char* filename) {
  cl_program program;
  FILE *program_handle;
  char *program_buffer, *program_log;
  size_t program_size, log_size;

  /* Read program file and place content into buffer */
  program_handle = fopen(filename, "r");

  if(program_handle == NULL) {
    perror("Couldn't find the program file");
    exit(1);
  }

  fseek(program_handle, 0, SEEK_END);
  program_size = ftell(program_handle);

  rewind(program_handle);

  program_buffer = (char*)malloc(program_size + 1);
  program_buffer[program_size] = '\0';

  fread(program_buffer, sizeof(char), program_size, program_handle);
  fclose(program_handle);

  /* Create program from file

   Creates a program from the source code in the add_numbers.cl file.
   Specifically, the code reads the file's content into a char array
   called program_buffer, and then calls clCreateProgramWithSource.
   */
  program = clCreateProgramWithSource(ctx, 1,
                                      (const char**)&program_buffer, &program_size, &openCLError);
  if(openCLError < 0) {
    perror("Couldn't create the program");
    exit(1);
  }
  free(program_buffer);

  /* Build program

   The fourth parameter accepts options that configure the compilation.
   These are similar to the flags used by gcc. For example, you can
   define a macro with the option -DMACRO=VALUE and turn off optimization
   with -cl-opt-disable.
   */
  openCLError = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);

  if(openCLError < 0) {
    /* Find size of log and print to std output */
    clGetProgramBuildInfo(program, dev, CL_PROGRAM_BUILD_LOG,
                          0, NULL, &log_size);
    program_log = (char*) malloc(log_size + 1);
    program_log[log_size] = '\0';
    clGetProgramBuildInfo(program, dev, CL_PROGRAM_BUILD_LOG,
                          log_size + 1, program_log, NULL);
    printf("%s\n", program_log);
    free(program_log);
    exit(1);
  }

  return program;
}

cl_device_id createOpenCLDevice() {
  cl_platform_id platform;
  cl_device_id dev;
  int err;

  /* Identify a platform */
  err = clGetPlatformIDs(1, &platform, NULL);
  if(err < 0) {
    perror("Couldn't identify a platform");
    exit(1);
  }

  // Access a device
  // GPU
//  clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &dev, NULL);
//  if(err == CL_DEVICE_NOT_FOUND) {
    //CPU
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &dev, NULL);
//  }
  if(err < 0) {
    perror("Couldn't access any devices");
    exit(1);
  }

  return dev;
}
