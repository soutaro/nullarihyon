# Make executable file to be located as ${PREFIX}/bin/nullarihyon
# make_frontend.rb [bin dir path] [tool path] [lib prefix for clang (prefix/lib/nullarihyon/1.2.3/clang)] [frontend dir path] [version]
#  executable: [bin dir path]/nullarihyon
#  analyzer tool path: [tool path]
#  -resource-dir [lib prefix for clang]/3.8.0
#  frontend: [frontend dir path]/lib/..., [frontend dir path]/bundle/...
#  version: version of this tool

require "pathname"
require "fileutils"

if ARGV.size == 5
  pwd = Pathname(".").realpath
  bin_dir_path = (pwd + ARGV[0]).cleanpath
  analyzer_path = (pwd + ARGV[1]).cleanpath
  resource_dir_base_path = (pwd + ARGV[2]).cleanpath
  script_lib_dir_path = (pwd + ARGV[3]).cleanpath
  version = ARGV[4]

  resource_dir_path = resource_dir_base_path.children.select {|path|
    path.basename.to_s =~ /\d/
  }.last

  bin_path = bin_dir_path + "nullarihyon"
  bin_dir_path.mkpath

  puts "Installing to #{bin_path}..."

  bin_path.open("w") do |io|
    io.puts <<EOS

$ANALYZER_PATH = "#{analyzer_path}"
$RESOURCE_DIR_PATH = "#{resource_dir_path}"

$LOAD_PATH << "#{script_lib_dir_path + "lib"}"
$LOAD_PATH << "#{script_lib_dir_path + "bundle"}"

$VERSION = "#{version}"

require 'nullarihyon/cli'
Nullarihyon::CLI.start(ARGV)
EOS

  FileUtils.chmod 0755, bin_path.to_s
  end
else
  puts "$ make_frontend.rb [AnalyzerPath] [ResourceDirPath] [ScriptLibDirPath] [version]"
end
