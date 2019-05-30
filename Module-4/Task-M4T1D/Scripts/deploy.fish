#!/usr/local/bin/fish

rm -rf .mpi
mkdir .mpi
cp -R $BUILT_PRODUCTS_DIR/ .mpi/
mv .mpi/$PRODUCT_NAME .mpi/$MPI_EXECUTABLE_NAME
mv ".mpi/$PRODUCT_NAME.swiftmodule/" ".mpi/$MPI_EXECUTABLE_NAME.swiftmodule/"

rm "/Users/brettbest/Public/SIT315-Repo/Module-4/Task-M4T1D/Config.xcconfig"
touch "/Users/brettbest/Public/SIT315-Repo/Module-4/Task-M4T1D/Config.xcconfig"
echo "MPI_EXECUTABLE_NAME=$PRODUCT_NAME-"(random) >> "/Users/brettbest/Public/SIT315-Repo/Module-4/Task-M4T1D/Config.xcconfig"
