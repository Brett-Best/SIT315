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
#include <thread>
#include <mutex>
#include <map>
#include <math.h>

using namespace std;

#define PRODUCERS_TO_USE 2 // Specify how many producers to use
#define CONSUMERS_TO_USE 2 // Specify how many consumers to use
#define READ_BUFFER_SIZE 5 // Max number of entries to have in memory
#define TRAFFIC_LIGHTS 3 // This isn't used

#pragma mark - Timers
struct timespec startTimespec, endTimespec; // Use for timing how long the project runs for

void startTimer() { // Starts a timer
  clock_gettime(CLOCK_MONOTONIC, &startTimespec);
}

void stopTimer() { // Stops the timer
  clock_gettime(CLOCK_MONOTONIC, &endTimespec);
}

double durationBetweenTimers() { // Calculate the time between start and finish
  double seconds = (endTimespec.tv_sec - startTimespec.tv_sec);
  seconds += (endTimespec.tv_nsec - startTimespec.tv_nsec) / 1000000000.0;
  
  return seconds;
}

#pragma mark - Traffic

struct TrafficSignalEntry { // Model to represent the entry in each CSV line
  long long timestamp;
  int id;
  int numberOfCars;
};

struct TrafficSignalMetric { // Model to represent a an hour of traffic
  long long startTimeStamp; // When the sample begins
  long long endTimeStamp; // When the sample ends
  map<int, int> trafficSignalToCarsMap; // a map of traffic signal ids to number of cars
  map<int, int, greater<int>> carsToTrafficSignalMap; // a map of the number of cars to traffic light - sorted using greater
  
  void sort() { // Sort the values included in this traffic metric
    if (trafficSignalToCarsMap.size() == carsToTrafficSignalMap.size()) { // Determine if we have already sorted this traffic metric
      printf("ALREADY SORTED\n");
      return; // Already sorted!
    }
    
    for (const auto &pair : trafficSignalToCarsMap) { // Insert new pair into the carsToTrafficSignalMap (so that it sorts)
      carsToTrafficSignalMap.insert(make_pair(pair.second, pair.first));
    }
    
    printf("METRIC SORTED - Start: %llu, End: %llu, Lights-Cars: [", startTimeStamp, endTimeStamp);
    for (const auto &pair : carsToTrafficSignalMap) { // Simmply log the information to the console.
      printf("%i-%i cars, ", pair.second, pair.first);
    }
    printf("]\n");
  }
};

TrafficSignalEntry firstTrafficSignalEntry; // This is populated with the first traffic signal from the data
vector<TrafficSignalEntry> trafficSignalEntries; // This is a buffer of the traffic signals
int trafficSignalEntriesRead = 0; // Count of how many traffic signal read so far

int producersCompleted = 0; // Count of how many producers have finished
mutex producersCompletedMutex; // A mutex used when accessing the producersCompleted variable above

mutex trafficSignalEntriesReadMutex; // A mutex used when accessing the trafficSignalEntries variable (above a few lines up)

#pragma mark -

class Producer { // A producer class that adds to the buffer
  
public:
  void readCSV(const string &csvPath, int threadId, int totalThreads) { // A method to read from the CSV
    ifstream csvFile; // Create a stream to access the CSV file with
    csvFile.open(csvPath.c_str()); // Open the file and convert the path to a c string
    
    if (!csvFile.is_open()) { // Unable to open the file, perhaps the path is wrong
      printf("Path is wrong!");
      exit(EXIT_FAILURE);
    }
    
    string lineData; // The current line data
    getline(csvFile, lineData); // Skip header row
    
    int line = 0; // The current line
    int linesProcessed = 0; // The amount of lines processed
    
    while (getline(csvFile,lineData)) { // Enumerate over the contents of the CSV
      bool shouldProcess = (linesProcessed * totalThreads) + threadId == line; // Work out if this producer should process this line by thread id / thread count
      
      if (!shouldProcess) { // Go onto next line as this producer isn't supposed to process the current line
        line++;
        continue;
      }
      
      if (lineData.empty()) {
        continue; // Skip empty lines
      }
      
      istringstream iStringStream(lineData); // Create a string stream from the line data
      string lineStream;
      string::size_type sz;
      
      vector <long long> row; // TO sure the command separated values
      
      while (getline(iStringStream, lineStream, ','))
      {
        row.push_back(stoll(lineStream,&sz)); // Add all values to the row vector
      }
      
      TrafficSignalEntry trafficSignalEntry = TrafficSignalEntry { row[0], (int)row[1], (int)row[2] }; // Create a traffic signal entry with the data
      if (line == 0) firstTrafficSignalEntry = trafficSignalEntry; // If the first line store it in the variable
      
      bool processed = false;
      
      while (!processed) { // We are going to try to add to the trraffic signal entries
        trafficSignalEntriesReadMutex.lock(); // Get a lock on the traffic signal entries
        
        if (trafficSignalEntriesRead == line) {
          unsigned long size = trafficSignalEntries.size();
          if (size >= READ_BUFFER_SIZE) { // Buffer is full, delay processing
            trafficSignalEntriesReadMutex.unlock(); // Unlock the traffic signal entries
            printf("DELAYED - Thread Id: %i, Timestamp: %lli, Id: %i, Number of cars: %i, Size: %lu\n", threadId, trafficSignalEntry.timestamp, trafficSignalEntry.id, trafficSignalEntry.numberOfCars, size);
            continue;
          }
          
          processed = true;
          trafficSignalEntriesRead++;
          trafficSignalEntries.push_back(trafficSignalEntry); // Add to the traffic signal entries buffer
          size = trafficSignalEntries.size();
          printf("PROCESSED - Thread Id: %i, Timestamp: %lli, Id: %i, Number of cars: %i, Size: %lu\n", threadId, trafficSignalEntry.timestamp, trafficSignalEntry.id, trafficSignalEntry.numberOfCars, size);
        }
        
        trafficSignalEntriesReadMutex.unlock(); // unlock the traffic signal entries
      }
      
      linesProcessed++;
      line++;
    }
    
    producersCompletedMutex.lock(); // Lock the producers completed count
    producersCompleted++; // Add to that count
    producersCompletedMutex.unlock(); // Unlock the producers count
  }
};

void printNumberOfCars();

#pragma mark -

vector<TrafficSignalMetric> trafficSignalMetrics;
mutex trafficSignalMetricsMutex;



class Consumer {
  
  long long maxTime = 0;
  TrafficSignalEntry lastTrafficSignalEntry;
  
public:
  void processData(int threadId, int threadCount) {
    bool shouldFinish = false;
    
    while(!shouldFinish) {
      shouldFinish = loadData(threadId, threadCount);
    }
  }
  
  bool loadData(int threadId, int threadCount) {
    trafficSignalEntriesReadMutex.lock();
    
    if (trafficSignalEntries.size() == 0) {
      trafficSignalEntriesReadMutex.unlock();
      return handleEmptySignalEntries(threadId, threadCount);
    }

    TrafficSignalEntry trafficSignalEntry = trafficSignalEntries[0];
    trafficSignalEntries.erase(trafficSignalEntries.begin());
    
    unsigned long size = trafficSignalEntries.size();
    printf("CONSUMED - Thread Id: %i, Timestamp: %lli, Id: %i, Number of cars: %i, Total number of cars: %i, Size: %lu\n", threadId, trafficSignalEntry.timestamp, trafficSignalEntry.id, trafficSignalEntry.numberOfCars, 0, size);
    createNewTrafficMetricIfNeeded(trafficSignalEntry);
    updateTrafficMetric(trafficSignalEntry);
    trafficSignalEntriesReadMutex.unlock();
    
    return false;
    }
  
  
  bool handleEmptySignalEntries(int threadId, int threadCount) {
    producersCompletedMutex.lock();
    
    if (producersCompleted == PRODUCERS_TO_USE) {
      producersCompletedMutex.unlock();
      trafficSignalMetricsMutex.lock();
      trafficSignalMetrics.back().sort();
      trafficSignalMetricsMutex.unlock();
      printf("CONSUMER FINISHED\n");
      return true;
    }
    
    producersCompletedMutex.unlock();
    
    return false;
  }
  
  void createNewTrafficMetricIfNeeded(TrafficSignalEntry trafficSignalEntry) {
    long long index = indexForTrafficSignalEntry(trafficSignalEntry);
    
    trafficSignalMetricsMutex.lock();
    if (trafficSignalMetrics.size() <= index) {
      if (index != 0) {
        trafficSignalMetrics[index-1].sort();
      }
      
      long long startTimeStamp = firstTrafficSignalEntry.timestamp + 60*60*index;
      long long endTimeStamp = startTimeStamp + 60*60;
      trafficSignalMetrics.push_back(TrafficSignalMetric{ startTimeStamp, endTimeStamp });
      
      TrafficSignalMetric metric = trafficSignalMetrics[index];
      printf("METRIC CREATED - Start: %llu, End: %llu, Map Count: %lu, Metric Count: %lu\n", metric.startTimeStamp, metric.endTimeStamp, metric.trafficSignalToCarsMap.size(), trafficSignalMetrics.size());
    }
    trafficSignalMetricsMutex.unlock();
  }
  
  void updateTrafficMetric(TrafficSignalEntry trafficSignalEntry) {
    long long index = indexForTrafficSignalEntry(trafficSignalEntry);
    trafficSignalMetricsMutex.lock();
    TrafficSignalMetric *trafficSignalMetric = &trafficSignalMetrics[index];
    trafficSignalMetric->trafficSignalToCarsMap[trafficSignalEntry.id] += trafficSignalEntry.numberOfCars;
    printf("METRIC UPDATED - Start: %llu, End: %llu, Lights-Cars: [", trafficSignalMetric->startTimeStamp, trafficSignalMetric->endTimeStamp);
    for (const auto &pair : trafficSignalMetric->trafficSignalToCarsMap) {
      printf("%i-%i cars, ", pair.second, pair.first);
    }
    printf("]\n");
    trafficSignalMetricsMutex.unlock();
  }
  
  long long indexForTrafficSignalEntry(TrafficSignalEntry trafficSignalEntry) {
    long long offset = trafficSignalEntry.timestamp-firstTrafficSignalEntry.timestamp;
    return (long long)floor(offset/60.0/60.0);
  }
  
};

#pragma mark -

int main(int argc, const char * argv[]) {
  
  thread producerThreads[PRODUCERS_TO_USE];
  thread consumerThreads[CONSUMERS_TO_USE];
  vector<Producer> producers;
  vector<Consumer> consumers;
  
  startTimer();
  
  for (int threadId = 0; threadId < PRODUCERS_TO_USE; threadId++) {
    Producer producer = Producer();
    
    producers.push_back(producer);
    producerThreads[threadId] = thread(&Producer::readCSV, producer, "trafficData.csv", threadId, PRODUCERS_TO_USE);
  }
  
  for (int threadId = 0; threadId < CONSUMERS_TO_USE; threadId++) {
    Consumer consumer = Consumer();
    
    consumers.push_back(consumer);
    consumerThreads[threadId] = thread(&Consumer::processData, consumer, threadId, CONSUMERS_TO_USE);
  }
  
  for (int threadId = 0; threadId < PRODUCERS_TO_USE; threadId++) {
    producerThreads[threadId].join();
  }
  
  for (int threadId = 0; threadId < CONSUMERS_TO_USE; threadId++) {
    consumerThreads[threadId].join();
  }
  
  stopTimer();
  
  printf("Read CSV: %.3fs\n", durationBetweenTimers());
  
  return 0;
}
