require "nullarihyon"

module Nullarihyon
  class CLI < Thor
    def self.sdks
      Configuration.sdk_paths(Pathname(`xcode-select -p`.chomp))
    end

    class_option :analyzer, type: :string, desc: "Path to analyzer executable", default: $ANALYZER_PATH
    class_option :resource_dir, type: :string, desc: "Path to -resource-dir option", default: $RESOURCE_DIR_PATH

    desc "check FILE...", "Run analyzer for FILE"
    option :sdk, type: :string, enum: sdks.keys, desc: "Name of SDK (will be used for -isysroot option)"
    option :sdk_path, type: :string, desc: "Path to SDK (will be passed as -isysroot option, overwrite --sdk)"
    option :flags, type: :array, desc: "Other clang/cc flags to analyzer"
    option :arc, type: :boolean, default: true, desc: "Enable ARC"
    option :modules, type: :boolean, default: true, desc: "Enable modules"
    option :block_assertions, type: :boolean, default: true, desc: "Ignore NSAssert family"
    def check(*files)
      config = Configuration.new(Pathname(options[:analyzer]).realpath, Pathname(options[:resource_dir]).realpath)
      config.arc_enabled = options[:arc]
      config.modules_enabled = options[:modules]
      config.assertions_blocked = options[:block_assertions]

      if options[:sdk]
        config.sysroot_path = CLI.sdks[options[:sdk]]
      end

      if options[:sdk_path]
        config.sysroot_path = Pathname(options[:sdk_path])
      end

      if options[:flags]
        options[:flags].each do |flag|
          config.add_other_flag flag
        end
      end

      files.each do |file|
        commandline = config.commandline(file).flatten
        puts Rainbow(commandline.join(" ")).blue
        system *commandline
      end
    end
  end
end
