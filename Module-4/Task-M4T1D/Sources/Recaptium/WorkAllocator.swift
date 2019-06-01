//
//  WorkAllocator.swift
//  Recaptium
//
//  Created by Brett Best on 31/5/19.
//

import Foundation
import CMPI

class WorkAllocator {
  enum Error: Swift.Error {
    case missingMLDataFolder
    case unableToCreateDirectory(at: URL, underlyingError: Swift.Error)
  }
  
  let mlDataURL: URL
  let dataset: String
  
  weak var environment: Environment!
  
  let recaptionDatasetFolderName: String
  
  init(dataset: String, environment: Environment) throws {
    self.dataset = dataset
    recaptionDatasetFolderName = "Recaptium_" + dataset
    
    self.environment = environment
    
    mlDataURL = Bundle.main.bundleURL.deletingLastPathComponent().appendingPathComponent("ML-Data", isDirectory: true)
    
    guard FileManager.default.fileExists(atPath: mlDataURL.path) else {
      throw Error.missingMLDataFolder
    }
  }
  
  func configureFolderStructure() throws {
    let recaptionDatasetURL = mlDataURL.appendingPathComponent(recaptionDatasetFolderName, isDirectory: true)
    
    try createRecaptionRootFolder(url: recaptionDatasetURL)
    
    MPI_Barrier(MPI_COMM_WORLD)
    
    try symlinkDataset(url: recaptionDatasetURL)
  }
  
  func createRecaptionRootFolder(url: URL) throws {
    if environment.worldRank == 0 {
      if FileManager.default.fileExists(atPath: url.path) {
        try FileManager.default.removeItem(at: url)
      }
      
      do {
        try FileManager.default.createDirectory(at: url, withIntermediateDirectories: false)
      } catch {
        throw Error.unableToCreateDirectory(at: url, underlyingError: error)
      }
    }
  }
  
  func symlinkDataset(url: URL) throws {
    let childDatasetURL = url.appendingPathComponent("\(environment.worldRank)", isDirectory: true)
    let trainingURL = childDatasetURL.appendingPathComponent("Training", isDirectory: true)
    let validationURL = childDatasetURL.appendingPathComponent("Validation", isDirectory: true)
    
    do {
      try FileManager.default.createDirectory(at: childDatasetURL, withIntermediateDirectories: false)
      try FileManager.default.createDirectory(at: trainingURL, withIntermediateDirectories: false)
      try FileManager.default.createDirectory(at: validationURL, withIntermediateDirectories: false)
    } catch {
      throw Error.unableToCreateDirectory(at: childDatasetURL, underlyingError: error)
    }
    
    let operationQueue = OperationQueue()
    
    let trainingSymlinkOperations = try symlinkDirectoryInDataset(name: "Training", baseURL: trainingURL)
    let validationSymlinkOperations = try symlinkDirectoryInDataset(name: "Validation", baseURL: validationURL)
    
    operationQueue.addOperations(trainingSymlinkOperations, waitUntilFinished: false)
    operationQueue.addOperations(validationSymlinkOperations, waitUntilFinished: false)
    
    operationQueue.qualityOfService = .userInteractive
    
    operationQueue.waitUntilAllOperationsAreFinished()
    
    MPI_Barrier(MPI_COMM_WORLD)
  }
  
  func symlinkDirectoryInDataset(name: String, baseURL: URL) throws -> [BlockOperation] {
    let subdirectory = mlDataURL.appendingPathComponent(dataset, isDirectory: true).appendingPathComponent(name, isDirectory: true)
    
    guard FileManager.default.fileExists(atPath: subdirectory.path) else {
      return []
    }
    
    let contents = try FileManager.default.contentsOfDirectory(at: subdirectory, includingPropertiesForKeys: nil, options: .skipsHiddenFiles).sorted {
      $0.lastPathComponent < $1.lastPathComponent
    }
    
    let filteredContents = contents.split(into: Int(environment.worldSize))[Int(environment.worldRank)]
    
    print(filteredContents.count)
    
    return filteredContents.map { url -> BlockOperation in
      return BlockOperation {
        let linkURL = baseURL.appendingPathComponent(url.lastPathComponent, isDirectory: true)
//        try! FileManager.default.createSymbolicLink(atPath: linkURL.path, withDestinationPath: url.path)
//        try! FileManager.default.createSymbolicLink(at: linkURL, withDestinationURL: url)
//        try! FileManager.default.linkItem(at: url, to: linkURL)
      }
    }
  }
  
  func build(completion: @escaping ((Bool) -> Void)) {
    let recaptionDatasetURL = mlDataURL.appendingPathComponent(recaptionDatasetFolderName, isDirectory: true)
    
    let trainingDataURL = recaptionDatasetURL.appendingPathComponent("\(environment.worldRank)/Training", isDirectory: true)
    let validationDataURL = recaptionDatasetURL.appendingPathComponent("\(environment.worldRank)/Validation", isDirectory: true)

    DispatchQueue.global(qos: .userInteractive).async {
      do {
        _ = try ImageClassifierBuilder(trainingDataURL: trainingDataURL, validationDataURL: validationDataURL)
        completion(true)
      } catch {
        print(error.localizedDescription)
        completion(false)
      }
    }
  }
  
}
