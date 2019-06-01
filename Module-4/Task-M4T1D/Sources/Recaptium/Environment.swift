//
//  Environment.swift
//  Recaptium
//
//  Created by Brett Best on 31/5/19.
//

import CMPI

class Environment {
  let worldSize: Int32
  let worldRank: Int32
  let processorName: String
  
  let createModelsEnabled: Bool
  let evaluationEnabled: Bool
  let analyseImagesEnabled: Bool
  
  let argc: Int32
  let argv: [String]
  
  init(argc: Int32, argv: [String]) {
    self.argc = argc
    self.argv = argv
    
    createModelsEnabled = argv.contains("--create-models")
    evaluationEnabled = argv.contains("--evaluate")
    analyseImagesEnabled = argv.contains("--analyse-images")
    
    var worldSize = Int32()
    MPI_Comm_size(MPI_COMM_WORLD, &worldSize)
    self.worldSize = worldSize
    
    var worldRank = Int32()
    MPI_Comm_rank(MPI_COMM_WORLD, &worldRank)
    self.worldRank = worldRank
    
    var cProcessorName = [CChar](repeating: 0, count: Int(MPI_MAX_PROCESSOR_NAME))
    var processorNameResultLen = Int32()
    
    MPI_Get_processor_name(&cProcessorName, &processorNameResultLen)
    
    let processorName = withUnsafePointer(to: &cProcessorName[0]) {
      return String(cString: $0)
    }
    
    self.processorName = processorName
  }
}

extension Environment {
  
  func printEnvironmentInfo() {
    if (worldRank == 0) {
      print("Argc: \(argc), Argv: \(argv)")
      
      var majorVersion = Int32()
      var minorVersion = Int32()
      
      MPI_Get_version(&majorVersion, &minorVersion)
      
      var cLibraryVersion = [CChar](repeating: 0, count: Int(MPI_MAX_LIBRARY_VERSION_STRING))
      var libraryVersionResultLen = Int32()
      
      MPI_Get_library_version(&cLibraryVersion, &libraryVersionResultLen)
      
      let libraryVersion = withUnsafePointer(to: &cLibraryVersion[0]) {
        return String(cString: $0)
      }
      
      print("MPI Version: \(majorVersion).\(minorVersion), \(libraryVersion)\n")
    }
    
    MPI_Barrier(MPI_COMM_WORLD)
    
    print("👋🗺  World Rank: \(worldRank), World Size: \(worldSize), Processor Name: \(processorName)")
    
    MPI_Barrier(MPI_COMM_WORLD)
  }
  
}
