# ruby TestRunner.rb --analyzer=../build/driver/Debug/nullarihyon-core objc/*.m

require "optparse"
require "pathname"

$Analyzer = nil

OptionParser.new do |opt|
  opt.on("--analyzer=PATH") {|path| $Analyzer = path }
end.parse!(ARGV)

unless $Analyzer
  puts "Tell me where analzer is located: --analyzer=../../some/where/nullarihyon-core"
  exit
end

failed = false

ARGV.each do |name|
  path = Pathname(name)

  lines = path.readlines

  options = ["-debug"]
  if lines.first.chomp =~ /^\/\/ Option: +(.+)$/
    options += $1.split
  end

  command_line = [$Analyzer] + options + [name] + ["--"] + %w(-Xclang -verify -fobjc-arc -fmodules)
  puts command_line.join(" ")
  unless system(*command_line)
    puts "💢 failed"
    failed = true
  end
end

exit(failed ? 1 : 0)
