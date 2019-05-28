//#define matrix unsigned long long
//
//__kernel void multiply_matrices(const int M, const int N, const int K, const __global int* A, const __global int* B, __global int* C) {
//  // Thread identifiers
//  const int globalRow = get_global_id(0); // Row ID of C (0..M)
//  const int globalCol = get_global_id(1); // Col ID of C (0..N)
//
////  printf("Global Row: %d, Global Col: %d\n", globalRow, globalCol);
//  // Compute a single element (loop over K)
//  int acc = 0;
//  for (matrix k=0; k<K; k++) {
//    acc += B[k*M + globalRow] * A[globalCol*K + k];
////    printf("(%d,%d), values = (%d, %d)\n ", k*M + globalRow, globalCol*K + k, A[k*M + globalRow] , B[globalCol*K + k]);
//  }
//
//  // Store the result
////  printf("(%d,%d), loc = %d, value = %d\n ", globalRow, globalCol, globalCol*M + globalRow, acc);
//  printf("Global Row: %d, Global Col: %d, Total: %i\n", globalRow, globalCol, acc);
//  C[globalCol*M + globalRow] = acc;
//}


__kernel void multiply_matrices(const int from, const int matrixSize, const __global int* matrixA, const __global int* matrixB, __global int* matrixBuffer) {

  // Thread identifiers
  const int globalRow = get_global_id(0) + from; // Row ID of C (0..M)
  const int globalColumn = get_global_id(1); // Col ID of C (0..N)
  
  int dotProduct = 0;
  int index = globalRow - from;
  int bufferIndex = globalColumn+(index*matrixSize);

  for (int offset = 0; offset < matrixSize; offset++) {
    dotProduct += matrixA[globalColumn+(offset*matrixSize)] * matrixB[offset+(globalRow*matrixSize)];
  }
  
  // Store the result
//  printf("[CL] Global Row: %d, Global Col: %d, From: %i, Index: %i, Buffer Index: %i, Dot Product: %i, Matrix Size: %i\n", globalRow, globalColumn, from, index, bufferIndex, dotProduct, matrixSize);
  matrixBuffer[bufferIndex] = dotProduct;
}

