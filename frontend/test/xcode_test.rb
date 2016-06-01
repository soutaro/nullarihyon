require "test_helper"

module Nullarihyon
  class XcodeTest < MiniTest::Spec
    before :each do
      @dir = Pathname(Dir.mktmpdir)

      @analyzer_path = @dir + "analyzer"
      @analyzer_path.open("w") {|io| }

      @resource_dir_path = @dir + "resource_dir"
      @resource_dir_path.mkpath

      @sysroot_path = @dir + "sysroot"
      @sysroot_path.mkpath

      @xcode_dir = @dir + "xcode"
      @sdk_root = @xcode_dir + "Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator9.3.sdk"
      @sdk_root.mkpath

      @build_dir = @dir + "build/Debug-iphonesimulator"
      @build_dir.mkpath

      @objects_dir_path = @build_dir + "TestProgram.build/Objects-normal/x86_64"
      @objects_dir_path.mkpath
    end

    after :each do
      @dir.rmtree
    end

    let(:env) {
      {
        "PROJECT_FILE_PATH" => (Pathname(__dir__) + "data/TestProgram/TestProgram.xcodeproj").realpath.to_s,
        "TARGETNAME" => "TestProgram",
        "SDKROOT" => @sdk_root.to_s,
        "CONFIGURATION" => "Debug",
        "CONFIGURATION_BUILD_DIR" => @build_dir.to_s,
        "CLANG_ENABLE_MODULES" => "NO",
        "CLANG_ENABLE_OBJC_ARC" => "YES",
        "FRAMEWORK_SEARCH_PATHS" => '/path/to/Frameworks "/another/path/to/Frameworks"',
        "HEADER_SEARCH_PATHS" => '/path/to/include "/another/path/to/include"',
        "OTHER_CFLAGS" => '-iquote /some/directory -isystem /another/directory',
        "OBJECT_FILE_DIR_normal" => (@build_dir + "TestProgram.build/Objects-normal").to_s,
        "arch" => "x86_64",
        "variant" => "normal",
      }
    }

    let (:xcode) {
      Xcode.new(@analyzer_path, @resource_dir_path, 1, env)
    }

    it "does something" do
      xcode
    end

    describe "#configuration" do
      it "has analyzer_path and resource_dir_path associated to xcode" do
        assert_equal @analyzer_path, xcode.configuration.analyzer_path
        assert_equal @resource_dir_path, xcode.configuration.resource_dir_path
      end

      it "has sysroot_path from xcode.sdkroot_path" do
        assert_equal @sdk_root, xcode.configuration.sysroot_path
      end

      it "has arc_enabled and modules_enabled associated to xcode" do
        assert xcode.configuration.arc_enabled
        refute xcode.configuration.modules_enabled
      end

      it "has assertions_blocked true" do
        assert xcode.configuration.assertions_blocked
      end

      it "has framework search paths" do
        config = xcode.configuration

        assert config.header_search_paths.include?([:framework, Pathname("/path/to/Frameworks")])
        assert config.header_search_paths.include?([:framework, Pathname("/another/path/to/Frameworks")])
      end

      it "has header search paths" do
        config = xcode.configuration

        assert config.header_search_paths.include?([:include, Pathname("/path/to/include")])
        assert config.header_search_paths.include?([:include, Pathname("/another/path/to/include")])
      end

      it "has objects_dir_path as header search path, for Swift bridging header" do
        config = xcode.configuration

        assert config.header_search_paths.include?([:include, xcode.objects_dir_path])
      end

      it "has other flags" do
        assert_equal %w(-iquote /some/directory -isystem /another/directory), xcode.configuration.other_flags
      end
    end

    describe "#sources" do
      it "returns array of .m source code" do
        sources = xcode.sources

        assert_equal 3, sources.size
        assert(sources.any? {|path| path.basename.to_s == "ViewController.m" })
        assert(sources.any? {|path| path.basename.to_s == "AppDelegate.m" })
        assert(sources.any? {|path| path.basename.to_s == "main.m" })
      end
    end

    describe "#sdkroot_path" do
      it "returns SDKROOT from env" do
        assert_equal @sdk_root, xcode.sdkroot_path
      end
    end

    describe "#objects_dir_path" do
      it "returns directory to have object files" do
        assert_equal Pathname(@build_dir + "TestProgram.build/Objects-normal/x86_64"), xcode.objects_dir_path
      end
    end

    describe "#framework_search_paths" do
      it "returns FRAMEWORK_SEARCH_PATHS from env" do
        assert_equal [Pathname("/path/to/Frameworks"), Pathname("/another/path/to/Frameworks")], xcode.framework_search_paths
      end
    end

    describe "#header_search_paths" do
      it "returns HEADER_SEARCH_PATHS from env" do
        assert_equal [Pathname("/path/to/include"), Pathname("/another/path/to/include")], xcode.header_search_paths
      end
    end

    describe "#other_cflags" do
      it "returns OTHER_CFLAGS from env" do
        assert_equal %w(-iquote /some/directory -isystem /another/directory), xcode.other_cflags
      end
    end

    describe "#arc_enabled?" do
      it "reads CLANG_ENABLE_OBJC_ARC from env" do
        assert xcode.arc_enabled?
      end
    end

    describe "#modules_enabled?" do
      it "reads CLANG_ENABLE_MODULES from env" do
        refute xcode.modules_enabled?
      end
    end

    describe ".tokenize_command_line" do
      it "splits command line to tokens" do
        assert_equal %w(a b c), Xcode.tokenize_command_line("a b c")
      end

      it "splits quoted command line" do
        assert_equal ["hello world", "good morning"], Xcode.tokenize_command_line("\"hello world\" 'good morning'")
      end
    end
  end
end
