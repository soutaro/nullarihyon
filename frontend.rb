require "optparse"
require "pathname"
require "yaml"
require "pp"
require "open3"

$Verbose = false
$Config = Pathname(".nullabilint.yml")
$Executable = "./tool"

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

for arg in ARGV
  command_line = [$Executable, arg, "--"] + options

  stdout, stderr, status = Open3.capture3(*command_line)

  if $Verbose || !status.success?
    puts command_line.join(" ")
  end

  unless status.success?
    STDERR.puts "Failed to execute analyzer..."
    STDERR.puts stderr
    exit 1
  else
    if $Verbose
      STDERR.puts stderr
    end

    STDOUT.puts stdout
  end
end

