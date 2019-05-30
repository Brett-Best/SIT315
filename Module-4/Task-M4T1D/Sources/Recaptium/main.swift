import CMPI

var argc = CommandLine.argc
var argv: UnsafeMutablePointer<UnsafeMutablePointer<Int8>?>? = CommandLine.unsafeArgv

MPI_Init(&argc, &argv)

var worldSize = Int32()
MPI_Comm_size(MPI_COMM_WORLD, &worldSize)

var worldRank = Int32()
MPI_Comm_rank(MPI_COMM_WORLD, &worldRank)

if (worldRank == 0) {
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

var cProcessorName = [CChar](repeating: 0, count: Int(MPI_MAX_PROCESSOR_NAME))
var processorNameResultLen = Int32()

MPI_Get_processor_name(&cProcessorName, &processorNameResultLen)

let processorName = withUnsafePointer(to: &cProcessorName[0]) {
  return String(cString: $0)
}

print("👋🗺  World Rank: \(worldRank), World Size: \(worldSize), Processor Name: \(processorName)")

MPI_Finalize()
