import Foundation
import CMPI

private var argc = CommandLine.argc
private var argv: UnsafeMutablePointer<UnsafeMutablePointer<Int8>?>? = CommandLine.unsafeArgv

MPI_Init(&argc, &argv)

let environment = Environment(argc: argc, argv: CommandLine.arguments)

let workAllocator: WorkAllocator

do {
  try workAllocator = WorkAllocator(dataset: "fruits-360")
} catch {
  fatalError(error.localizedDescription)
}

workAllocator.listDataFolders()

MPI_Finalize()
