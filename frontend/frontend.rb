require "optparse"
require "pathname"
require "yaml"
require "pp"
require "open3"

$Verbose = false
$Config = Pathname("null.yml")
$Executable = "nullarihyon-core"

additional_options = []

OptionParser.new do |opt|
  opt.on("-v", "--verbose") { $Verbose = true }
  opt.on("-c CONFIG", "--config=CONFIG") {|path| $Config = Pathname(path) }
  opt.on("-e PATH", "--executable=CONFIG") {|executable| $Executable = executable }
  opt.on("-X OPTION") {|option| additional_options << option }
end.parse!(ARGV)

unless $Config.file?
  STDERR.puts "Configuration file #{$Config} not found."
end

config = $Config.file? ? YAML.load($Config.open) : {}

options = (config[:commandline_options] || []) + additional_options

command_line = [$Executable] + ARGV + ["--"] + options
puts command_line.join(" ")
system *command_line

