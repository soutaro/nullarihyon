Encoding.default_external = Encoding::UTF_8

module Nullarihyon
  class Xcode
    attr_reader :analyzer_path
    attr_reader :resource_dir_path
    attr_reader :jobs
    attr_reader :env
    attr_reader :only_latest
    attr_reader :project_path

    attr_accessor :debug

    def initialize(analyzer_path, resource_dir_path, jobs, only_latest, env=ENV)
      @analyzer_path = analyzer_path
      @resource_dir_path = resource_dir_path
      @jobs = jobs
      @env = env
      @only_latest = only_latest

      @project_path = Pathname(env['PROJECT_FILE_PATH'])
      @project = Xcodeproj::Project.open(@project_path)
      @target = @project.targets.find {|target| target.name == env['TARGETNAME']}
      @configuration = env["CONFIGURATION"]
      @build_dir = Pathname(env["CONFIGURATION_BUILD_DIR"])

      debug = false
    end

    def header_search_paths
      self.class.tokenize_command_line(env["HEADER_SEARCH_PATHS"]).map {|path| Pathname(path) }
    end

    def framework_search_paths
      self.class.tokenize_command_line(env["FRAMEWORK_SEARCH_PATHS"]).map {|path| Pathname(path) }
    end

    def other_cflags
      self.class.tokenize_command_line(env["OTHER_CFLAGS"])
    end

    def arc_enabled?
      env["CLANG_ENABLE_OBJC_ARC"] == "YES"
    end

    def modules_enabled?
      env["CLANG_ENABLE_MODULES"] == "YES"
    end

    def objects_dir_path
      variant = env["variant"]
      key = "OBJECT_FILE_DIR_#{variant}"
      Pathname(env[key]) + arch
    end

    def sdkroot_path
      Pathname(env["SDKROOT"])
    end

    def filters
      file = project_path.parent + "nullfilter"
      if file.file?
        file.readlines.map {|x|
          x.chomp.gsub(/#(.*)/, "").strip
        }.select {|filter|
          filter.length > 0
        }
      else
        []
      end
    end

    def prefix_header_path
      if env["GCC_PRECOMPILE_PREFIX_HEADER"] == "YES"
        Pathname(env["GCC_PREFIX_HEADER"])
      end
    end

    def hmap_paths
      temp_dir_path = Pathname(env["TARGET_TEMP_DIR"])
      Dir.glob((temp_dir_path + "*.hmap").to_s)
    end

    def preprocessor_definitions
      self.class.tokenize_command_line(env["GCC_PREPROCESSOR_DEFINITIONS"])
    end

    def configuration
      Configuration.new(analyzer_path, resource_dir_path).tap do |config|
        config.sysroot_path = sdkroot_path

        config.arc_enabled = arc_enabled?
        config.modules_enabled = modules_enabled?
        config.assertions_blocked = true
        config.debug = debug

        filters.each do |filter|
          config.add_filter filter
        end

        framework_search_paths.each do |path|
          config.add_header_search_path :framework, path
        end

        header_search_paths.each do |path|
          config.add_header_search_path :include, path
        end

        config.add_header_search_path :include, objects_dir_path

        hmap_paths.each do |path|
          config.add_header_search_path :include, path
        end

        config.arch = arch

        other_cflags.each do |flag|
          config.add_other_flag flag
        end

        preprocessor_definitions.each do |opt|
          config.add_other_flag "-D#{opt}"
        end

        if prefix_header_path && prefix_header_path.file?
          config.add_other_flag "-include", prefix_header_path.realpath.to_s
        end
      end
    end

    def sources
      @target.source_build_phase.files_references.map {|file| file.real_path }.select {|path| path.extname == ".m" }
    end

    def arch
      env["arch"]
    end

    def last_check_result_path(source, objects_dir)
      basename = source.basename(source.extname).to_s + ".null"
      objects_dir + basename
    end

    def object_file_path(source, objects_dir)
      basename = source.basename(source.extname).to_s + ".o"
      objects_dir + basename
    end

    def need_check?(source, objects_dir)
      object_path = object_file_path(source, objects_dir)
      result_path = last_check_result_path(source, objects_dir)

      !object_path.file? || !result_path.file? || result_path.mtime < object_path.mtime
    end

    def run_analyzer(source, commandline)
      Open3.capture2e(*commandline)
    end

    def check(config, source, objects_dir, force)
      io = StringIO.new

      commandline = config.commandline(source).flatten
      io.puts commandline.join(" ")

      if force || need_check?(source, objects_dir)
        output, _ = run_analyzer(source, commandline)

        last_check_result_path(source, objects_dir).open("w") {|io| io.print output }

        io.print output
      else
        if only_latest
          io.puts "Skip printing result because the code is not updated..."
        else
          io.puts "Read from cache created at #{last_check_result_path(source, objects_dir).mtime}..."
          io.print last_check_result_path(source, objects_dir).read
        end
      end

      io.string
    end

    def config_updated?(objects_dir, config)
      config_path = objects_dir + "nullarihyon.config"
      last_config = config_path.file? ? config_path.read : "no config"
      new_config = config.commandline.flatten.to_s

      config_path.open('w') {|io| io.write(new_config) }

      last_config != new_config
    end

    def run(io)
      config = configuration
      objects_dir = objects_dir_path

      force = config_updated?(objects_dir, config)

      input_queue = Queue.new
      output_queue = Queue.new

      sources.each do |path|
        input_queue << path
      end
      jobs.times do
        input_queue << :done
      end

      workers = jobs.times.map do
        Thread.new do
          loop do
            path = input_queue.pop
            break if path == :done

            output = check(config, path, objects_dir, force)

            output_queue << output
          end
        end
      end

      output_thread = Thread.new do
        loop do
          output = output_queue.pop
          break if output == :done

          io.print output
        end
      end

      workers.each do |worker| worker.join end
      output_queue << :done
      output_thread.join
    end

    def self.tokenize_command_line(line)
      return [] unless line

      scanner = StringScanner.new(line)

      tokens = []

      while true
        scanner.scan(/ +/)
        scanner.scan(/(\"([^\"]*)\")|(\'([^\']*)\')|([^ ]+)/)

        token = scanner[2] || scanner[4] || scanner[5]
        if token
          tokens << token
        else
          break
        end
      end

      tokens
    end
  end
end
