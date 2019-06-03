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
  let vnCoreMlRequests: [VNCoreMLRequest]
  
  let completedRequestsLock = NSLock()
  let operationQueue = OperationQueue()
  
  var completedRequests: UInt = 0
  
  init(environment: Environment, models: [MLModel]) throws {
    self.environment = environment
    
    operationQueue.qualityOfService = .userInteractive
    
    vnCoreMlModels = try models.compactMap { model in
      return try VNCoreMLModel(for: model)
    }
    
    vnCoreMlRequests = vnCoreMlModels.map {
      let request = VNCoreMLRequest(model: $0) { request, error in
        let classifications = request.results as? [VNClassificationObservation]
//        print(classifications?.first)
//        print("[SLAVE \(environment.worldRank)] Processed ML Request")
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
  
  func processImages(at url: URL, completion: (() -> Void)) {
    let contents: [URL]
    do {
       contents = try FileManager.default.contentsOfDirectory(at: url, includingPropertiesForKeys: nil, options: .skipsHiddenFiles).sorted {
        $0.lastPathComponent < $1.lastPathComponent
      }
    } catch {
      print("[ERROR] \(error)")
      completion()
      return
    }
    
    let totalChunks = Int(environment.worldSize) * 20
    let chunkedContents = contents.split(into: totalChunks)
    var chunkIndex = environment.worldRank
    
    if environment.worldRank == 0 {
      let nextChunkIndexLock = NSLock()
      var _nextChunkIndex: Int32 = environment.worldSize
      
      var nextChunkIndex: Int32 {
        get {
          defer { nextChunkIndexLock.unlock() }
          nextChunkIndexLock.lock()
          return _nextChunkIndex
        }
        
        set {
          nextChunkIndexLock.lock()
          _nextChunkIndex = newValue < totalChunks ? newValue : -1
          nextChunkIndexLock.unlock()
        }
      }
      
    
      let workAllocatorQueue = OperationQueue()
      workAllocatorQueue.qualityOfService = .userInteractive
      
      let operations = (0..<environment.worldSize).map { rank in
        BlockOperation {
          if rank == 0 {
            
          } else {
            var completed: CBool = false
            print("[MASTER] waiting for slave \(rank)")
            MPI_Recv(&completed, 1, MPI_C_BOOL, rank, 0, MPI_COMM_WORLD, nil)
            
            while nextChunkIndex != -1 {
              var chunkIndexToSend = nextChunkIndex
              nextChunkIndex = nextChunkIndex + 1
              print("[MASTER] Sent \(chunkIndex) to \(rank)")
              MPI_Send(&chunkIndexToSend, 1, MPI_INT32_T, rank, 0, MPI_COMM_WORLD)
              
              var completed: CBool = false
              print("[MASTER] waiting for slave \(rank)")
              MPI_Recv(&completed, 1, MPI_C_BOOL, rank, 0, MPI_COMM_WORLD, nil)
            }
            
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
        print("[SLAVE \(environment.worldRank)] Processing index: \(chunkIndex) \(chunkIndex == -1)")
        var completed: CBool = true
        let dispatchSemaphore = DispatchSemaphore(value: 0)
        
        let imageURLsToProcess = chunkedContents[Int(chunkIndex)]
        let totalImagesToProcess = imageURLsToProcess.count
        imageURLsToProcess.forEach {
          processImage(at: $0) { [unowned self] completedRequests in
            print("[SLAVE \(self.environment.worldRank)] Image Processed \(completedRequests) of \(totalImagesToProcess)")
            if totalImagesToProcess == completedRequests {
              print("[SLAVE \(self.environment.worldRank)] ALL Images Processed")
              self.completedRequests = 0
              dispatchSemaphore.signal()
            }
          }
        }
        
        dispatchSemaphore.wait()
        
        print("[SLAVE \(environment.worldRank)] Request next chunk")
        
        MPI_Send(&completed, 1, MPI_C_BOOL, 0, 0, MPI_COMM_WORLD)
        print("[SLAVE \(environment.worldRank)] Sent to master")
        MPI_Recv(&chunkIndex, 1, MPI_INT32_T, 0, 0, MPI_COMM_WORLD, nil)
        print("[SLAVE \(environment.worldRank)] Received index: \(chunkIndex)")
      }
    }
    
    print("[SLAVE \(environment.worldRank)] ALL FINISHED")
  }
  
}
