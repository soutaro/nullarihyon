module Nullarihyon
  class Configuration
    attr_reader :analyzer_path
    attr_reader :resource_dir_path

    attr_accessor :sysroot_path

    attr_accessor :arc_enabled
    attr_accessor :modules_enabled
    attr_accessor :assertions_blocked
    attr_accessor :arch

    attr_reader :header_search_paths
    attr_reader :other_flags

    def initialize(analyzer_path, resource_dir_path)
      @analyzer_path = analyzer_path
      @resource_dir_path = resource_dir_path

      arc_enabled = true
      modules_enabled = true
      assertions_blocked = true

      @header_search_paths = []
      @other_flags = []
    end

    def add_header_search_path(kind, path)
      header_search_paths << [kind, path]
    end

    def add_other_flag(*args)
      case args.size
      when 0
        # skip
      when 1
        other_flags << args.first
      else
        other_flags << args
      end
    end

    def commandline(*files)
      array = []

      array << analyzer_path.to_s

      array += files.map {|path| path.to_s }

      array << "--"

      array << ["-resource-dir", resource_dir_path.to_s]
      array << ["-x", "objective-c"]

      if sysroot_path
        array << ["-isysroot", sysroot_path.to_s]
      end

      array << (arc_enabled ? "-fobjc-arc" : "-fno-objc-arc")
      array << (modules_enabled ? "-fmodules" : "-fno-modules")
      array << "-DNS_BLOCK_ASSERTIONS=1" if assertions_blocked

      for header_search_path in header_search_paths
        case header_search_path[0]
        when :include
          option = "-I"
        when :framework
          option = "-F"
        when :iquote
          option = "-iquote"
        when :isystem
          option = "-isystem"
        end

        if option
          array << [option, header_search_path[1].to_s]
        end
      end

      if arch
        array << ["-arch", arch.to_s]
      end

      array += other_flags

      array
    end

    def self.sdk_paths(xcode_path)
      # Prefer simulator SDKs
      platform_names = %w(AppleTVSimulator MacOSX WatchSimulator iPhoneSimulator)

      paths = {}

      platform_names.each do |name|
        platform_path = xcode_path + "Platforms" + "#{name}.platform"
        if platform_path.directory?
          (platform_path + "Developer" + "SDKs").children.each do |path|
            if path.extname == ".sdk"
              name = path.basename(path.extname).to_s.downcase
              paths[name] = path
            end
          end
        end
      end

      # macosx SDK would be missing; use the latest versioned sdk installed
      unless paths["macosx"]
        osx_sdk_keys = paths.keys.select {|name| name =~ /^macosx\d/ }.sort
        paths["macosx"] = paths[osx_sdk_keys.last]
      end

      paths
    end

    def self.sdk_path(sdk, xcode_path)
      sdk_paths(xcode_path)[sdk.to_s]
    end
  end
end
