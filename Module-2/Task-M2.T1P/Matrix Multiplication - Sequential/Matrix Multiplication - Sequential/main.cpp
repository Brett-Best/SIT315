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
#include <fstream>

using namespace std;

#define MATRIX_SIZE 2'000
#define MAX_VALUE 1'000'000UL
#define matrix unsigned long long

matrix matrixA[MATRIX_SIZE][MATRIX_SIZE];
matrix matrixB[MATRIX_SIZE][MATRIX_SIZE];
matrix matrixC[MATRIX_SIZE][MATRIX_SIZE];

clock_t t_start;
clock_t t_end;

void generateMatrices() {
  int row, column;
  
  for (row = 0; row < MATRIX_SIZE; row++) {
    for (column = 0; column < MATRIX_SIZE; column++) {
      matrixA[row][column] = rand() % MAX_VALUE;
      matrixB[row][column] = rand() % MAX_VALUE;
    }
  }
}

void multiplyMatrices() {
  int row, column, offset;
  
  for (row = 0; row < MATRIX_SIZE; row++) {
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
  ofstream outputFileStream("MM-Sequential.txt");
  
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
  cout << "Number of cores: " << hardwareConcurrency << endl;;
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
  srand((int)time(NULL));
  
  printThreadInfo();
  
  startTimer();
  
  generateMatrices();
  
  stopTimer();
  
  printf("Generate Matrices: %.3fs\n", durationBetweenTimers());
  
  startTimer();
  
  multiplyMatrices();
  
  stopTimer();
  
  printf("Multiply Matrices: %.3fs\n", durationBetweenTimers());
  
  startTimer();
  
  writeMatricesToDisk();
  
  stopTimer();
  
  printf("Write Output: %.3fs\n", durationBetweenTimers());
  
  return 0;
}
