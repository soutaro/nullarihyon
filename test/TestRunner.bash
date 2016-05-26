# analyzer=../build/driver/Debug/nullabilint-core bash TestRunner.bash objc/*.m

for input in $@; do
  ${analyzer} -debug ${input} -- -Xclang -verify -fobjc-arc -fmodules
done
