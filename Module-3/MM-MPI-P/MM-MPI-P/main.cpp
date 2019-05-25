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
#include <vector>

using namespace std;

#define MATRIX_SIZE 18
//#define MAX_VALUE 1'000'000UL
#define MAX_VALUE 3UL
#define matrix unsigned long long
#define THREADS_TO_USE 8

matrix matrixA[MATRIX_SIZE][MATRIX_SIZE];
matrix matrixB[MATRIX_SIZE][MATRIX_SIZE];
vector<vector<matrix>> matrixC;

struct timespec startTimespec, endTimespec;

void printMatrice(matrix matrice[MATRIX_SIZE][MATRIX_SIZE]);

void generateMatrices() {
  int row, column;

  for (row = 0; row < MATRIX_SIZE; row++) {
    for (column = 0; column < MATRIX_SIZE; column++) {
      matrixA[row][column] = rand() % MAX_VALUE;
      matrixB[row][column] = rand() % MAX_VALUE;
    }
  }
}

void multiplyMatrices(int worldRank, int worldSize) {
  int chunks = worldSize;
  int row, column, offset;

  int maxChunkSize = max(MATRIX_SIZE / chunks, 1);
  int from = maxChunkSize * worldRank;

  if (from > MATRIX_SIZE - 1) {
    printf("World Rank: %i, unneeded.\n", worldRank);
    return;
  }

  int to = (worldRank + 1) == chunks ? MATRIX_SIZE : maxChunkSize * (worldRank + 1);
  int rowsToMultiply = to-from;
  
  printf("World Rank: %i, Rows to mulitply: %i, From: %i, To: %i\n", worldRank, rowsToMultiply, from, to-1);

//  matrix** matrixBuffer = new matrix*[rowsToMultiply];
//  for(matrix i = 0; i < rowsToMultiply; i++) matrixBuffer[i] = new matrix[MATRIX_SIZE];

  
//  matrix matrixBuffer[MATRIX_SIZE][MATRIX_SIZE];
  
  vector<vector<matrix>> matrixBuffer;
  matrixBuffer.reserve(rowsToMultiply);
  
  for (row = from; row < to; row++) {
    int index = row-from;
    matrixBuffer[index].reserve(MATRIX_SIZE);
    for (column = 0; column < MATRIX_SIZE; column++) {
      for (offset = 0; offset < MATRIX_SIZE; offset++) {
//        printf("Index: %i\n", index);
        matrixBuffer[index][column] += matrixA[row][offset] * matrixB[offset][column];
      }
    }
  }
  
  MPI_Barrier(MPI_COMM_WORLD);
//  if (worldRank == 0) {
//    printMatrice(matrixBuffer);
//  }
  MPI_Barrier(MPI_COMM_WORLD);
  MPI_Gather(matrixBuffer.data(), rowsToMultiply, MPI_UNSIGNED_LONG_LONG, matrixC.data(), rowsToMultiply, MPI_UNSIGNED_LONG_LONG, 0, MPI_COMM_WORLD);
}

void writeMatriceToDisk(string name, matrix matrice[MATRIX_SIZE][MATRIX_SIZE], ofstream *outputFileStream) {
  *outputFileStream << "# Matrice: " << name << endl << endl;

  int row, column;

  for (row = 0; row < MATRIX_SIZE; row++) {
    for (column = 0; column < MATRIX_SIZE; column++) {
      *outputFileStream << matrice[row][column];

      if (column != MATRIX_SIZE - 1) {
        *outputFileStream << ",";
      }
    }

    *outputFileStream << endl;
  }

  *outputFileStream << endl << endl;
}

void writeMatriceToDisk(string name, vector<vector<matrix>> matrice, ofstream *outputFileStream) {
  *outputFileStream << "# Matrice: " << name << endl << endl;
  
  int row, column;
  
  for (row = 0; row < MATRIX_SIZE; row++) {
    for (column = 0; column < MATRIX_SIZE; column++) {
      *outputFileStream << matrice[row][column];
      
      if (column != MATRIX_SIZE - 1) {
        *outputFileStream << ",";
      }
    }
    
    *outputFileStream << endl;
  }
  
  *outputFileStream << endl << endl;
}

void printMatrice(matrix matrice[MATRIX_SIZE][MATRIX_SIZE]) {
  int row, column;
  
  for (row = 0; row < MATRIX_SIZE; row++) {
    for (column = 0; column < MATRIX_SIZE; column++) {
      cout << matrice[row][column];
      
      if (column != MATRIX_SIZE - 1) {
        cout << ",";
      }
    }
    
    cout << endl;
  }
}

void writeMatricesToDisk() {
  ofstream outputFileStream("/Volumes/Shared Folders/Module-3/DerivedData/MPI Matrix Multiplication/Build/Products/Debug/MM-Parallel.txt");

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
  
  matrixC.reserve(MATRIX_SIZE);
  for (int i = 0; i < MATRIX_SIZE; i++) {
    matrixC[i].reserve(MATRIX_SIZE);
  }

  MPI_Init(NULL, NULL);

  int worldSize = 9;
  MPI_Comm_size(MPI_COMM_WORLD, &worldSize);

  int worldRank = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &worldRank);

  char processorName[MPI_MAX_PROCESSOR_NAME];
  int nameLen;
  MPI_Get_processor_name(processorName, &nameLen);

  printf("Processor: %s, rank: %d out of %d processors\n", processorName, worldRank, worldSize);
  
  if (worldRank == 0) {
    // Master node
    
    printThreadInfo();
    
    startTimer();
    
    generateMatrices();
    
    stopTimer();
    
    printf("Generate Matrices: %.3fs\n", durationBetweenTimers());
  }
  
  MPI_Barrier(MPI_COMM_WORLD);

  MPI_Bcast(&matrixA, MATRIX_SIZE*MATRIX_SIZE, MPI_UNSIGNED_LONG_LONG, 0, MPI_COMM_WORLD);
  MPI_Bcast(&matrixB, MATRIX_SIZE*MATRIX_SIZE, MPI_UNSIGNED_LONG_LONG, 0, MPI_COMM_WORLD);

  MPI_Barrier(MPI_COMM_WORLD);

  if (worldRank == 0) startTimer();
  
  multiplyMatrices(worldRank, worldSize);
  
  MPI_Barrier(MPI_COMM_WORLD);

  if (worldRank == 0) {
    stopTimer();
    printf("Multiply Matrices: %.3fs\n", durationBetweenTimers());
  }

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
