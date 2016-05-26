# analyzer=../build/driver/Debug/nullabilint-core bash TestRunner.bash objc/*.m

for input in $@; do
  ${analyzer} ${input} -- -Xclang -verify -fobjc-arc -fmodules
done
