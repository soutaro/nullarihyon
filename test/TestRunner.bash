# analyzer=../build/driver/Debug/nullarihyon-core bash TestRunner.bash objc/*.m

for input in $@; do
  ${analyzer} -debug ${input} -- -Xclang -verify -fobjc-arc -fmodules
done
