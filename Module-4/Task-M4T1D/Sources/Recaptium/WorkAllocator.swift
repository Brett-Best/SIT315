//
//  WorkAllocator.swift
//  Recaptium
//
//  Created by Brett Best on 31/5/19.
//

import Foundation

class WorkAllocator {
  enum InitError: Swift.Error {
    case missingMLDataBundle
  }
  
  let mlDataBundle: Bundle
  let dataset: String
  
  init(dataset: String) throws {
    self.dataset = dataset
    
    let mlDataBundleURL = Bundle.main.bundleURL.deletingLastPathComponent().appendingPathComponent("ML-Data", isDirectory: true)
    
    guard let bundle = Bundle(url: mlDataBundleURL), FileManager.default.fileExists(atPath: bundle.bundlePath) else {
      throw InitError.missingMLDataBundle
    }
    
    mlDataBundle = bundle
  }
  
  func listDataFolders() {
    let urls = mlDataBundle.urls(forResourcesWithExtension: nil, subdirectory: "\(dataset)/Training")
    print(urls!.map { $0.absoluteURL })
  }
  
}
