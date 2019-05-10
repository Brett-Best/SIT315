//
//  main.cpp
//  Traffic Control Simulator
//
//  Created by Brett Best on 10/5/19.
//  Copyright © 2019 Brett Best. All rights reserved.
//

#include <iostream>
#include <stdio.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

using namespace std;

struct TrafficSignalEntry {
  long long timestamp;
  int id;
  int numberOfCars;
};

void readCSV(const string &csvPath) {
  ifstream csvFile;
  csvFile.open(csvPath.c_str());
  
  if (!csvFile.is_open()) {
    printf("Path is wrong!");
    exit(EXIT_FAILURE);
  }
  
  vector<TrafficSignalEntry> trafficSignalEntries;
  
  string line;
  vector <string> vec;
  getline(csvFile, line); // Skip header row
  
  while (getline(csvFile,line)) {
    if (line.empty()) {
      continue; // Skip empty lines
    }
    
    istringstream iStringStream(line);
    string lineStream;
    string::size_type sz;
    
    vector <long long> row;
    
    while (getline(iStringStream, lineStream, ','))
    {
      row.push_back(stoll(lineStream,&sz));
    }
    
    TrafficSignalEntry trafficSignalEntry = TrafficSignalEntry { row[0], (int)row[1], (int)row[2] };
    trafficSignalEntries.push_back(trafficSignalEntry);
  }
  
}

int main(int argc, const char * argv[]) {
  readCSV("trafficData.csv");
  
  return 0;
}
