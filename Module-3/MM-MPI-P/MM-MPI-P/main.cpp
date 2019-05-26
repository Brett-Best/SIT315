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

#define MATRIX_SIZE 20
#define MAX_VALUE 10UL
#define matrix unsigned long long
#define THREADS_TO_USE 8

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

void multiplyMatrices(int threadId) {
  int row, column, offset;
  
  int chunkSize = max(MATRIX_SIZE / THREADS_TO_USE, 1);
  int from = chunkSize * threadId;
  
  if (from > MATRIX_SIZE - 1) {
    printf("Thread ID: %i, unneeded.\n", threadId);
    return;
  }
  
  int to = (threadId + 1) == THREADS_TO_USE ? MATRIX_SIZE : chunkSize * (threadId + 1);
  
  printf("Thread ID: %i, From: %i, To: %i\n", threadId, from, to-1);
  
  for (row = from; row < to; row++) {
    for (column = 0; column < MATRIX_SIZE; column++) {
      for (offset = 0; offset < MATRIX_SIZE; offset++) {
        matrixC[row+(column*MATRIX_SIZE)] += matrixA[row+(offset*MATRIX_SIZE)] * matrixB[offset+(column*MATRIX_SIZE)];
      }
    }
  }
  
  return;
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
  ofstream outputFileStream("MM-MPI-P.txt");
  
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
  
  printThreadInfo();
  
  startTimer();
  
  matrixA = (matrix*)calloc(MATRIX_SIZE*MATRIX_SIZE, sizeof(matrix));
  matrixB = (matrix*)calloc(MATRIX_SIZE*MATRIX_SIZE, sizeof(matrix));
  matrixC = (matrix*)calloc(MATRIX_SIZE*MATRIX_SIZE, sizeof(matrix));
  
  generateMatrices();
  
  stopTimer();
  
  printf("Generate Matrices: %.3fs\n", durationBetweenTimers());
  
  startTimer();
  
  thread threads[THREADS_TO_USE];
  
  for (int threadId = 0; threadId < THREADS_TO_USE; threadId++) {
    threads[threadId] = thread(multiplyMatrices, threadId);
  }
  
  for (int threadId = 0; threadId < THREADS_TO_USE; threadId++) {
    threads[threadId].join();
  }
  
  stopTimer();
  
  printf("Multiply Matrices: %.3fs\n", durationBetweenTimers());
  
  startTimer();
  
  writeMatricesToDisk();
  
  stopTimer();
  
  printf("Write Output: %.3fs\n", durationBetweenTimers());
  
  return 0;
}
