# print frontend commandline

require "optparse"
require "pathname"

$analyzer_path = nil
$resource_dir_path = nil

OptionParser.new do |opt|
  opt.on("--analyzer=PATH") {|path|
    $analyzer_path = Pathname(path).realpath
  }
  opt.on("--resource-dir=PATH") {|path|
    $resource_dir_path = Pathname(path).realpath
  }
end.parse!(ARGV)

dir_path = Pathname(__dir__).realpath
file_path = Pathname(__FILE__).realpath

puts "/usr/bin/ruby -I#{dir_path + "lib"}:#{dir_path + "bundle"} #{file_path} xcode --analyzer=#{$analyzer_path} --resource-dir=#{$resource_dir_path}"
