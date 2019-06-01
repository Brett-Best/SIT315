//
//  CoreMLModelBuilder.swift
//  Recaptium
//
//  Created by Brett Best on 31/5/19.
//

import Foundation
import CMPI

class CoreMLModelBuilder {
  enum Error: Swift.Error {
    case missingMLDataFolder
    case unableToCreateDirectory(at: URL, underlyingError: Swift.Error)
  }
  
  let mlDataURL: URL
  let dataset: String
  
  weak var environment: Environment!
  
  let recaptionDatasetFolderName: String
  let recaptionModelsFolderName: String
  
  init(dataset: String, environment: Environment) throws {
    self.dataset = dataset
    recaptionDatasetFolderName = "Recaptium_" + dataset
    recaptionModelsFolderName = "Recaptium-Models"
    
    self.environment = environment
    
    mlDataURL = Bundle.main.bundleURL.deletingLastPathComponent().appendingPathComponent("ML-Data", isDirectory: true)
    
    guard FileManager.default.fileExists(atPath: mlDataURL.path) else {
      throw Error.missingMLDataFolder
    }
  }
  
  func configureFolderStructure() throws {
    let recaptionDatasetURL = mlDataURL.appendingPathComponent(recaptionDatasetFolderName, isDirectory: true)
    let recaptionModelsURL = mlDataURL.appendingPathComponent(recaptionModelsFolderName, isDirectory: true)
    
    try createRecaptionRootFolder(url: recaptionDatasetURL)
    try createModelsFolderIfNeeded(url: recaptionModelsURL)
    
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
  
  func createModelsFolderIfNeeded(url: URL) throws {
    if environment.worldRank == 0 {
      if FileManager.default.fileExists(atPath: url.path) {
        return
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
    operationQueue.qualityOfService = .userInteractive
    
    let trainingSymlinkOperations = try symlinkDirectoryInDataset(name: "Training", baseURL: trainingURL)
    let validationSymlinkOperations = try symlinkDirectoryInDataset(name: "Validation", baseURL: validationURL)
    
    operationQueue.addOperations(trainingSymlinkOperations, waitUntilFinished: false)
    operationQueue.addOperations(validationSymlinkOperations, waitUntilFinished: false)
    
    operationQueue.waitUntilAllOperationsAreFinished()
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
    
    return filteredContents.map { url -> BlockOperation in
      return BlockOperation {
        let linkURL = baseURL.appendingPathComponent(url.lastPathComponent, isDirectory: true)
        do {
          try FileManager.default.linkItem(at: url, to: linkURL)
        } catch {
          print("[ERROR] failed to link item with error: \(error)")
        }
      }
    }
  }
  
  func build(completion: @escaping ((Bool) -> Void)) {
    let recaptionDatasetURL = mlDataURL.appendingPathComponent(recaptionDatasetFolderName, isDirectory: true)
    
    let trainingDataURL = recaptionDatasetURL.appendingPathComponent("\(environment.worldRank)/Training", isDirectory: true)
    let validationDataURL = recaptionDatasetURL.appendingPathComponent("\(environment.worldRank)/Validation", isDirectory: true)
    let evaluationDataURL = mlDataURL.appendingPathComponent("\(dataset)/evaluation", isDirectory: true)
    let modelWriteURL = mlDataURL.appendingPathComponent(recaptionModelsFolderName, isDirectory: true).appendingPathComponent("\(dataset)_\(environment.worldRank)_\(environment.worldSize)", isDirectory: false).appendingPathExtension("mlmodel")

    DispatchQueue.global(qos: .userInteractive).async { [unowned self] in
      do {
        _ = try ImageClassifierBuilder(trainingDataURL: trainingDataURL, validationDataURL: validationDataURL, writeURL: modelWriteURL, evaluationURL: self.environment.evaluationEnabled ? evaluationDataURL : nil)
        completion(true)
      } catch {
        print("[ERROR] \(error)")
        completion(false)
      }
    }
  }
  
}
