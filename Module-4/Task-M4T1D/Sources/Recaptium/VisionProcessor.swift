//
//  VisionProcessor.swift
//  Recaptium
//
//  Created by Brett Best on 2/6/19.
//

import CoreML
import Vision
import CMPI

class VisionProcessor {
  
  let environment: Environment
  
  let vnCoreMlModels: [VNCoreMLModel]
  var vnCoreMlRequests: [VNCoreMLRequest] = []
  
  let completedRequestsLock = NSLock()
  let operationQueue = OperationQueue()
  
  var completedRequests: UInt = 0
  var completedRequestsOutput: String = ""
  
  init(environment: Environment, models: [MLModel]) throws {
    self.environment = environment
    
    operationQueue.qualityOfService = .userInteractive
    
    vnCoreMlModels = try models.compactMap { model in
      return try VNCoreMLModel(for: model)
    }
    
    vnCoreMlRequests = vnCoreMlModels.map {
      let request = VNCoreMLRequest(model: $0) { request, error in
        let classifications = request.results as? [VNClassificationObservation]
        
        let topClassification = classifications?.filter { observation -> Bool in
          observation.confidence >= 0.95
        }.first
        
        if let topClassification = topClassification {
          self.completedRequestsOutput.append("\(topClassification.identifier) (\(Int(topClassification.confidence*100))%), ")
        }
      }
      
      request.usesCPUOnly = environment.worldRank != 1
      
      return request
    }
  }
  
  func processImage(at url: URL, completion: @escaping ((_ completedRequests: UInt) -> Void)) {
    let handler = VNImageRequestHandler(url: url)
    
    operationQueue.addOperation {
      do {
        try handler.perform(self.vnCoreMlRequests)
      } catch {
        print("[ERROR] Failed to process \(url) \(error)")
      }
      
      self.completedRequestsLock.lock()
      self.completedRequests = self.completedRequests + 1
      completion(self.completedRequests)
      self.completedRequestsLock.unlock()
    }
  }
  
  func processImages(at url: URL) {
    let contents: [URL]
    do {
       contents = try FileManager.default.contentsOfDirectory(at: url, includingPropertiesForKeys: nil, options: .skipsHiddenFiles).sorted {
        $0.lastPathComponent < $1.lastPathComponent
      }
    } catch {
      print("[ERROR] \(error)")
      return
    }
    
    var totalChunks = Int(environment.worldSize-1) * 30 // The GPU is about 30x faster than the CPU.
    let chunkedContents = contents.split(into: totalChunks, shouldPad: false)
    totalChunks = chunkedContents.count
    var chunkIndex = environment.worldRank
    
    if environment.worldRank == 0 {
      let nextChunkIndexLock = NSLock()
      var nextChunkIndex: Int32 = environment.worldSize
    
      let workAllocatorQueue = OperationQueue()
      workAllocatorQueue.qualityOfService = .userInteractive
      
      let operations = (0..<environment.worldSize).map { rank in
        BlockOperation {
          if rank == 0 {
            // Master node doesn't support running ML at the moment, for separation of concerns I decided against adding in this complexity as it would've required a better abstraction of the threading login + work distribution
          } else {
            var completed: CBool = false
            print("[MASTER] waiting for slave \(rank)")
            MPI_Recv(&completed, 1, MPI_C_BOOL, rank, 0, MPI_COMM_WORLD, nil)
            
            nextChunkIndexLock.lock()
            var shouldContinueLooping = nextChunkIndex != -1
            while shouldContinueLooping {
              var chunkIndexToSend = nextChunkIndex
              nextChunkIndex = nextChunkIndex + 1
              if !(nextChunkIndex < totalChunks) {
                nextChunkIndex = -1
                print("[MASTER] Next Chunk is -1")
              }
              nextChunkIndexLock.unlock()
              
              print("[MASTER] Sent \(chunkIndex) to \(rank)")
              MPI_Send(&chunkIndexToSend, 1, MPI_INT32_T, rank, 0, MPI_COMM_WORLD)
              
              var completed: CBool = false
              print("[MASTER] waiting for slave \(rank)")
              MPI_Recv(&completed, 1, MPI_C_BOOL, rank, 0, MPI_COMM_WORLD, nil)
              
              nextChunkIndexLock.lock()
              shouldContinueLooping = nextChunkIndex != -1
            }
            nextChunkIndexLock.unlock()
            
            print("[MASTER] COMPLETED \(rank)")
            var completedInt: Int32 = -1
            MPI_Send(&completedInt, 1, MPI_INT32_T, rank, 0, MPI_COMM_WORLD)
          }
        }
      }
      
      workAllocatorQueue.addOperations(operations, waitUntilFinished: true)
      print("[MASTER] All Operations Completed")
    } else {
      while chunkIndex != -1 {
        print("[SLAVE \(environment.worldRank)] Processing index: \(chunkIndex)")
        var completed: CBool = true
        let dispatchSemaphore = DispatchSemaphore(value: 0)
        
        let imageURLsToProcess = chunkedContents[Int(chunkIndex)]
        let totalImagesToProcess = imageURLsToProcess.count
        imageURLsToProcess.forEach {
          completedRequestsOutput = "[ClASSIFICATION] \($0.lastPathComponent): "
          processImage(at: $0) { [unowned self] completedRequests in
            print("[SLAVE \(self.environment.worldRank)] Image Processed \(completedRequests) of \(totalImagesToProcess)")
            if totalImagesToProcess == completedRequests {
              print("[SLAVE \(self.environment.worldRank)] ALL Images Processed")
              self.completedRequests = 0
              print(self.completedRequestsOutput)
              self.completedRequestsOutput = ""
              dispatchSemaphore.signal()
            }
          }
        }
        
        if !imageURLsToProcess.isEmpty {
          dispatchSemaphore.wait()
        } else {
          assertionFailure("This should never get hit, however, if it does the chunking of work has an error in it.")
        }
        
        print("[SLAVE \(environment.worldRank)] Request next chunk, completed chunk \(chunkIndex)")
        
        MPI_Send(&completed, 1, MPI_C_BOOL, 0, 0, MPI_COMM_WORLD)
        print("[SLAVE \(environment.worldRank)] Sent to master")
        MPI_Recv(&chunkIndex, 1, MPI_INT32_T, 0, 0, MPI_COMM_WORLD, nil)
        print("[SLAVE \(environment.worldRank)] Received index: \(chunkIndex)")
      }
    }
    
    print("[SLAVE \(environment.worldRank)] ALL FINISHED")
  }
  
}
