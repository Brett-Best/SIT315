__kernel void multiplyMatrices(const int fromRow, const int matrixSize, const __global unsigned long* matrixA, const __global unsigned long* matrixB, __global unsigned long* matrixBuffer) {
  // Calculate the dot product for the specified row/column.
  const int globalRow = get_global_id(0) + fromRow; // Row (fromRow...matrixSize)
  const int globalColumn = get_global_id(1); // Col (0...matrixSize)
  
  unsigned long dotProduct = 0;
  int index = globalRow - fromRow;
  int bufferIndex = globalColumn+(index*matrixSize);

  for (int offset = 0; offset < matrixSize; offset++) {
    dotProduct += matrixA[globalColumn+(offset*matrixSize)] * matrixB[offset+(globalRow*matrixSize)];
  }
  
  matrixBuffer[bufferIndex] = dotProduct;
}

