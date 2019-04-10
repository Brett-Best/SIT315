//
//  main.cpp
//  Matrix Multiplication - Sequential
//
//  Created by Brett Best on 9/4/19.
//  Copyright © 2019 Brett Best. All rights reserved.
//

#include <iostream>
#include <stdio.h>
#include <thread>

#define MATRIX_SIZE 3
#define MAX_VALUE 5UL
#define matrix unsigned long long

matrix matrixA[MATRIX_SIZE][MATRIX_SIZE];
matrix matrixB[MATRIX_SIZE][MATRIX_SIZE];
matrix matrixC[MATRIX_SIZE][MATRIX_SIZE];

clock_t t_start;
clock_t t_end;

void generateMatrices() {
  int row, column;
  for (row = 0; row < MATRIX_SIZE; row++)
  {
    for (column = 0; column < MATRIX_SIZE; column++)
    {
      matrixA[row][column] = rand()%MAX_VALUE;
      matrixB[row][column] = rand()%MAX_VALUE;
    }
  }
}

void multiplyMatrices() {
  int row, column, offset;
  
  for (row = 0; row < MATRIX_SIZE; row++)
  {
    for (column = 0; column < MATRIX_SIZE; column++)
    {
      for (offset = 0; offset < MATRIX_SIZE; offset++)
      {
        matrixC[row][column] += matrixA[row][offset] * matrixB[offset][column];
      }
    }
  }
}

void printThreadInfo() {
  unsigned int hardwareConcurrency = std::thread::hardware_concurrency();
  std::cout << "Number of cores: " << hardwareConcurrency << std::endl;;
}

void startTimer() {
  t_start = clock();
}

void stopTimer() {
  t_end = clock();
}

double durationBetweenTimers() {
  return ((double)(t_end - t_start)) / CLOCKS_PER_SEC;
}

int main(int argc, const char * argv[]) {
  printThreadInfo();
  
  startTimer();
  
  generateMatrices();
  
  stopTimer();
  
  printf("%.3f\n", durationBetweenTimers());
  
  startTimer();
  
  multiplyMatrices();
  
  stopTimer();
  
  printf("%.3f\n", durationBetweenTimers());
  
  return 0;
}
