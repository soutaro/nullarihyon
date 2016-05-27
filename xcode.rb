require "pathname"
load (Pathname(__FILE__).parent + "bundle/bundler/setup.rb")

require "optparse"
require "yaml"
require "pp"
require "open3"
require "xcodeproj"
require "strscan"
require "thread"

$Verbose = false
$Config = Pathname(".nullabilint.yml")
$Executable = "nullabilint-core"
$AdditionalOptions = []
$Jobs = 4

OptionParser.new do |opt|
  opt.on("-v", "--verbose") { $Verbose = true }
  opt.on("-c CONFIG", "--config=CONFIG") {|path| $Config = Pathname(path) }
  opt.on("-e PATH", "--executable=CONFIG") {|executable| $Executable = executable }
  opt.on("-X OPTION") {|option| $AdditionalOptions << option }
  opt.on("-j JOBS") {|jobs| $Jobs = jobs.to_i }
end.parse!(ARGV)

project_path = ENV["PROJECT_FILE_PATH"]
target_name = ENV["TARGETNAME"]

unless project_path && target_name
  puts 'Start from Xcode "Run Script Phase"...'
  exit
end

project = Xcodeproj::Project.open(project_path)
target = project.targets.find {|target| target.name == target_name }

unless target
  puts "Could not find target #{target_name}..."
  exit
end

options = []

options.push "-F"
options.push ENV["CONFIGURATION_BUILD_DIR"]

source_files = target.source_build_phase.files.map {|file| file.file_ref.real_path }.select {|path| path.extname == ".m" }

objects_dir_key = "OBJECT_FILE_DIR_#{ENV["variant"]}"
objects_dir = Pathname(ENV[objects_dir_key]) + ENV["arch"]

options.push "-I"
options.push objects_dir.to_s

if objects_dir.directory?
  timestamp_file = objects_dir + "nullabilint"

  if timestamp_file.file?
    updated_objects = objects_dir.children.select do |object_path|
      object_path.file? && object_path.extname == ".o" && object_path.mtime > timestamp_file.mtime
    end

    source_files = source_files.select {|src_path|
      updated_objects.find {|obj_path|
        obj_path.basename(obj_path.extname) == src_path.basename(src_path.extname)
      }
    }
  end

  timestamp_file.open("w") {|io| io.puts } # touch
end

if source_files.count == 0
  exit
end

config = $Config.file? ? YAML.load($Config.open) : {}

all_options = (config[:commandline_options] || []) + options + $AdditionalOptions

input_queue = Queue.new
output_queue = Queue.new

source_files.each do |path|
  input_queue << path
end

threads = $Jobs.times.map {
  input_queue << nil

  Thread.new do
    while path = input_queue.pop
      command_line = [$Executable, path.to_s, "--"] + all_options
      output_queue << command_line.join(" ")

      output, status = Open3.capture2e(*command_line)

      output_queue << output
    end
  end
}

output_thread = Thread.new do
  while output = output_queue.pop
    if output == :end
      break
    end

    puts output
  end
end

threads.each {|t| t.join }
output_queue << :end
output_thread.join

