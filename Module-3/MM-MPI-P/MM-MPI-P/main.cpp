//
//  main.cpp
//  Matrix Multiplication - Parallel
//
//  Created by Brett Best on 9/4/19.
//  Copyright © 2019 Brett Best. All rights reserved.
//

#include <iostream>
#include <stdio.h>
#include <thread>
#include <fstream>
#include <mpi.h>

using namespace std;

#define MATRIX_SIZE 18
#define MAX_VALUE 10UL
#define matrix unsigned long long
//#define THREADS_TO_USE 8

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

void multiplyMatrices(int worldRank, int worldSize) {
//  if (worldRank != 0) return;
//
//  bool canContinue = false;
//  while (!canContinue) {
//  }
  
  matrix *matrixBuffer;
  int row, column, offset;
  
  int maxChunkSize = max(MATRIX_SIZE / worldSize, 1);
  int from = maxChunkSize * worldRank;
  
  if (from > MATRIX_SIZE - 1) {
    printf("[MM] Thread ID: %i, unneeded.\n", worldRank);
    return;
  }
  
  int to = (worldRank + 1) == worldSize ? MATRIX_SIZE : maxChunkSize * (worldRank + 1);
  int rowsToMultiply = to-from;
  int bufferSize = rowsToMultiply*MATRIX_SIZE;
  
  matrixBuffer = (matrix*)calloc(bufferSize, sizeof(matrix));
  
  printf("[MM] World Rank: %i, Buffer Size: %i, Rows: %i, From: %i, To: %i\n", worldRank, bufferSize, rowsToMultiply, from, to-1);
  
  for (row = from; row < to; row++) {
    int index = row-from;
    printf("[MM] World Rank: %i, Row: %i, Index: %i\n", worldRank, row, index);
    for (column = 0; column < MATRIX_SIZE; column++) {
      int bufferIndex = column+(index*MATRIX_SIZE);
      printf("[MM] World Rank: %i, Buffer Index: %i, Row: %i, Index: %i, Column: %i\n", worldRank, bufferIndex, row, index, column);
      matrixBuffer[bufferIndex] = 0;
      for (offset = 0; offset < MATRIX_SIZE; offset++) {
        matrixBuffer[bufferIndex] += matrixA[row+(offset*MATRIX_SIZE)] * matrixB[offset+(column*MATRIX_SIZE)];
      }
    }
  }
  
  MPI_Barrier(MPI_COMM_WORLD);
  MPI_Gather(matrixBuffer, bufferSize, MPI_UNSIGNED_LONG_LONG, matrixC, bufferSize, MPI_UNSIGNED_LONG_LONG, 0, MPI_COMM_WORLD);
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
  
  matrixA = (matrix*)calloc(MATRIX_SIZE*MATRIX_SIZE, sizeof(matrix));
  matrixB = (matrix*)calloc(MATRIX_SIZE*MATRIX_SIZE, sizeof(matrix));
  matrixC = (matrix*)calloc(MATRIX_SIZE*MATRIX_SIZE, sizeof(matrix));
  
  if (worldRank == 0) generateMatrices();
  
  stopTimer();
  
  printf("Generate Matrices: %.3fs\n", durationBetweenTimers());
  
  startTimer();
  
  MPI_Bcast(matrixA, MATRIX_SIZE*MATRIX_SIZE, MPI_UNSIGNED_LONG_LONG, 0, MPI_COMM_WORLD);
  MPI_Bcast(matrixB, MATRIX_SIZE*MATRIX_SIZE, MPI_UNSIGNED_LONG_LONG, 0, MPI_COMM_WORLD);
  
  MPI_Barrier(MPI_COMM_WORLD);
  
  stopTimer();
  
  printf("Broadcast Matrices: %.3fs\n", durationBetweenTimers());
  
  startTimer();
  
  multiplyMatrices(worldRank, worldSize);
  
  stopTimer();
  
  printf("Multiply Matrices: %.3fs\n", durationBetweenTimers());
  
  MPI_Barrier(MPI_COMM_WORLD);
  
  if (worldRank == 0) {
    startTimer();
    
    writeMatricesToDisk();
    
    stopTimer();
    
    printf("Write Output: %.3fs\n", durationBetweenTimers());
  }
  
  MPI_Barrier(MPI_COMM_WORLD);
  MPI_Finalize();
  
  return 0;
}
