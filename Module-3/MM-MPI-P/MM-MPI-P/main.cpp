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

using namespace std;

#define MATRIX_SIZE 2'000
#define MAX_VALUE 1'000'000UL
#define matrix unsigned long long
#define THREADS_TO_USE 8

matrix matrixA[MATRIX_SIZE][MATRIX_SIZE];
matrix matrixB[MATRIX_SIZE][MATRIX_SIZE];
matrix matrixC[MATRIX_SIZE][MATRIX_SIZE];

struct timespec startTimespec, endTimespec;

void generateMatrices() {
  int row, column;
  
  for (row = 0; row < MATRIX_SIZE; row++) {
    for (column = 0; column < MATRIX_SIZE; column++) {
      matrixA[row][column] = rand() % MAX_VALUE;
      matrixB[row][column] = rand() % MAX_VALUE;
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
        matrixC[row][column] += matrixA[row][offset] * matrixB[offset][column];
      }
    }
  }
  
  return;
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

void writeMatricesToDisk() {
  ofstream outputFileStream("MM-Parallel.txt");
  
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
  srand((int)time(NULL));
  
  printThreadInfo();
  
  startTimer();
  
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
