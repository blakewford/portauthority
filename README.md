# portauthority
Static analysis tool for uncertain heterogeneous computing


**clang setup instructions**
git clone https://github.com/llvm/llvm-project

cd llvm-project
mkdir build
cd build

cmake -G "Unix Makefiles" \[options\] ../llvm

cmake -j 16 -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD="AVR" -DLLVM_TARGETS_TO_BUILD="X86;AArch64" ~/llvm-project/llvm
cmake -j 16 -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra;libcxx;libcxxabi;compiler-rt;polly" ~/llvm-project/llvm

clang version 10.0.0 (https://github.com/llvm/llvm-project 926d283893ae764cfba0360badcd7cc9fef4dfe5) (https://github.com/llvm-mirror/llvm.git 0b7752e32297b432fba9c84141d152f02cf56705)
