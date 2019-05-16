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

void printNumberOfCars(); // Forward declare the print number of cars function

#pragma mark -

vector<TrafficSignalMetric> trafficSignalMetrics; // Array of traffic signal metrics, 1 per hour
mutex trafficSignalMetricsMutex; // Traffic signal metrics mutex for variable above



class Consumer { // A consumer class that reads from the buffer and creates metrics
  
  long long maxTime = 0; // Not actually used
  TrafficSignalEntry lastTrafficSignalEntry; // The last signal entry processed

public:
  void processData(int threadId, int threadCount) { // Process some data
    bool shouldFinish = false; // Variable to determine if the consumer is completed and should finish
    
    while(!shouldFinish) { // Keep going until the consumer should finish
      shouldFinish = loadData(threadId, threadCount); // Load some data
    }
  }
  
  bool loadData(int threadId, int threadCount) { // Loads data from the traffic signal entries
    trafficSignalEntriesReadMutex.lock(); // Lock the variable while we access it
    
    if (trafficSignalEntries.size() == 0) { // If no entries to process on the buffer
      trafficSignalEntriesReadMutex.unlock(); // Unlock the traffic signal entries
      return handleEmptySignalEntries(threadId, threadCount); // A method to check if all the producers are completed and no more data to process
    }

    TrafficSignalEntry trafficSignalEntry = trafficSignalEntries[0]; // Get the next traffic signal from the buffer
    trafficSignalEntries.erase(trafficSignalEntries.begin()); // Erase the entry we have just read from the buffer
    
    unsigned long size = trafficSignalEntries.size(); // Get the size of the buffer that is remaining for logging purposes (validation)
    printf("CONSUMED - Thread Id: %i, Timestamp: %lli, Id: %i, Number of cars: %i, Total number of cars: %i, Size: %lu\n", threadId, trafficSignalEntry.timestamp, trafficSignalEntry.id, trafficSignalEntry.numberOfCars, 0, size);
    createNewTrafficMetricIfNeeded(trafficSignalEntry); // Create a new traffic metric if one doesnt exist already
    updateTrafficMetric(trafficSignalEntry); // Update the traffic signal metric with this traffic signal entry
    trafficSignalEntriesReadMutex.unlock(); // Unlock the traffic signal entries mutex
    
    return false;
    }
  
  
  bool handleEmptySignalEntries(int threadId, int threadCount) { // A function to determine if no more entries will be added to the buffer as all producers completed
    producersCompletedMutex.lock();
    
    if (producersCompleted == PRODUCERS_TO_USE) { // Check to see if all the producers are done using the lock as appropriate
      producersCompletedMutex.unlock();
      trafficSignalMetricsMutex.lock();
      trafficSignalMetrics.back().sort(); // Sort the last traffic signal metric as no more data will be added
      trafficSignalMetricsMutex.unlock();
      printf("CONSUMER FINISHED\n");
      return true;
    }
    
    producersCompletedMutex.unlock();
    
    return false;
  }
  
  void createNewTrafficMetricIfNeeded(TrafficSignalEntry trafficSignalEntry) { // Create a new traffic signal metric for the traffic signal if an appropriate one doesn't exist
    long long index = indexForTrafficSignalEntry(trafficSignalEntry); // generate an index for the appropriate metric (this is based on the 60min duration of a metric)
    
    trafficSignalMetricsMutex.lock(); // While we are going to add to the traffic signal metrics, lock it
    if (trafficSignalMetrics.size() <= index) { // We need to create a new traffic signal metrics
      if (index != 0) {
        trafficSignalMetrics[index-1].sort(); // Sort the previous traffic signal metric
      }
      
      long long startTimeStamp = firstTrafficSignalEntry.timestamp + 60*60*index; // Calculate a start time stamp for the metric
      long long endTimeStamp = startTimeStamp + 60*60; // Calculate a end time stamp for the metric
      trafficSignalMetrics.push_back(TrafficSignalMetric{ startTimeStamp, endTimeStamp }); // Create + add the metric to the vector
      
      TrafficSignalMetric metric = trafficSignalMetrics[index]; // retrieve the metric from the vector for logging purposes and my validation
      printf("METRIC CREATED - Start: %llu, End: %llu, Map Count: %lu, Metric Count: %lu\n", metric.startTimeStamp, metric.endTimeStamp, metric.trafficSignalToCarsMap.size(), trafficSignalMetrics.size());
    }
    trafficSignalMetricsMutex.unlock(); // No loonger need the lock
  }
  
  void updateTrafficMetric(TrafficSignalEntry trafficSignalEntry) { // Update the traffic signal entry by adding a new signal entry to it
    long long index = indexForTrafficSignalEntry(trafficSignalEntry); // generate an index for the appropriate metric (this is based on the 60min duration of a metric)
    trafficSignalMetricsMutex.lock(); // Lock the traffic signal metrics
    TrafficSignalMetric *trafficSignalMetric = &trafficSignalMetrics[index]; // Get the traffic signal metric that needs updating
    trafficSignalMetric->trafficSignalToCarsMap[trafficSignalEntry.id] += trafficSignalEntry.numberOfCars; // Add to the entry to the map value
    printf("METRIC UPDATED - Start: %llu, End: %llu, Lights-Cars: [", trafficSignalMetric->startTimeStamp, trafficSignalMetric->endTimeStamp); // Just for logging purposess and self validation
    for (const auto &pair : trafficSignalMetric->trafficSignalToCarsMap) { // 
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
