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
  puts command_line.join(" ")

  status = Open3.popen3 *command_line do |stdin, stdout, stderr, thread|
    stdin.close

    out = Thread.new do
      while line = stdout.gets
        STDOUT.print line
      end
    end

    err = Thread.new do
      while line = stderr.gets
        STDERR.print line if $Verbose
      end
    end

    thread.join()
    out.join
    err.join

    thread.value
  end

  unless status.success?
    STDERR.puts "Failed to execute analyzer..."
  end
end

