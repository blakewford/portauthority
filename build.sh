#~/gcc/gcc/testsuite/c-c++-common/
#~/gcc/gcc/testsuite/gcc.c-torture/execute/
for file in $1*.c
do
  gcc "$file" -o "${file%.c}"
done
