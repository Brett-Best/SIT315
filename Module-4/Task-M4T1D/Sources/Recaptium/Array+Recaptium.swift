//
//  Array+Recaptium.swift
//  Recaptium
//
//  Created by Brett Best on 1/6/19.
//

import Foundation

extension Array {
  
  func split(into chunks: Int, shouldPad: Bool = true) -> [[Element]] {
    var chunkedArray = chunked(into: Int(ceil(Double(count)/Double(chunks))))
    if chunkedArray.count != chunks && shouldPad {
      chunkedArray.append(contentsOf: [[Element]](repeating: [], count: chunks-chunkedArray.count))
    }
    return chunkedArray
  }
  
  func chunked(into size: Int) -> [[Element]] {
    return stride(from: 0, to: count, by: size).map { index in
      Array(self[index ..< Swift.min(index + size, count)])
    }
  }
  
}

