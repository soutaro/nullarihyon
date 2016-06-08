require "test_helper"

module Nullarihyon
  class ConfigurationTest < MiniTest::Spec
    before :each do
      @dir = Pathname(Dir.mktmpdir)

      @analyzer_path = @dir + "analyzer"
      @analyzer_path.open("w") {|io| }

      @resource_dir_path = @dir + "resource_dir"
      @resource_dir_path.mkpath

      @sysroot_path = @dir + "sysroot"
      @sysroot_path.mkpath

      @xcode_dir = @dir + "xcode"
      @sdk_paths = [
        @xcode_dir + "Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.11.sdk",
        @xcode_dir + "Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator.sdk",
        @xcode_dir + "Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator9.3.sdk",
        @xcode_dir + "Platforms/Unknown.platform/Developer/SDKs/unknown.sdk"
      ]
      @sdk_paths.each do |path|
        path.mkpath
      end
    end

    after :each do
      @dir.rmtree
    end

    let(:config) { Configuration.new(@analyzer_path, @resource_dir_path) }

    describe Configuration do
      describe "#commandline" do
        it "starts with analyzer path, --, and -resource-dir option" do
          assert_equal @analyzer_path.to_s, config.commandline[0]
          assert_equal "--", config.commandline[1]
          assert_equal ["-resource-dir", @resource_dir_path.to_s], config.commandline[2]
        end

        it "accepts files" do
          commandline = config.commandline("a.m")
          assert_equal "a.m", commandline[1], "source code"
          assert_equal "--", commandline[2], "separator"
          assert commandline[3], "has more options"
        end

        it "contains -x objective-c option" do
          assert config.commandline.include?(["-x", "objective-c"])
        end

        it "contains -isysroot option" do
          config.sysroot_path = @sysroot_path
          assert config.commandline.include?(["-isysroot", @sysroot_path.to_s])
        end

        it "contains -fobjc-arc if arc_enabled" do
          config.arc_enabled = true
          assert config.commandline.include?("-fobjc-arc")
        end

        it "contains -fno-objc-arc if arc_enabled is false" do
          config.arc_enabled = false
          assert config.commandline.include?("-fno-objc-arc")
        end

        it "contains -fmodules if modules_enabled" do
          config.modules_enabled = true
          assert config.commandline.include?("-fmodules")
        end

        it "contains -fno-modules if modules_enabled is false" do
          config.modules_enabled = false
          assert config.commandline.include?("-fno-modules")
        end

        it "contains NS_BLOCK_ASSERTIONS definition if assertions_blocked" do
          config.assertions_blocked = true
          assert config.commandline.include?("-DNS_BLOCK_ASSERTIONS=1")
        end

        it "contains header search path" do
          config.add_header_search_path :include, @dir + "include"
          config.add_header_search_path :framework, @dir + "frameworks"
          config.add_header_search_path :iquote, @dir + "iquote"
          config.add_header_search_path :isystem, @dir + "isystem"

          assert config.commandline.include?(["-I", (@dir + "include").to_s])
          assert config.commandline.include?(["-F", (@dir + "frameworks").to_s])
          assert config.commandline.include?(["-iquote", (@dir + "iquote").to_s])
          assert config.commandline.include?(["-isystem", (@dir + "isystem").to_s])
        end

        it "contains -debug option" do
          config.debug = true
          assert config.commandline.include?("-debug")
        end

        it "contains other flags" do
          config.add_other_flag "-arch", "i386"
          assert config.commandline.include?(["-arch", "i386"])
        end

        it "contains -arch flag if given" do
          config.arch = :i386
          assert config.commandline.include?(["-arch", "i386"])
        end

        it "contains -filter flags" do
          config.add_filter "FooClass"
          config.add_filter "BarClass"

          assert config.commandline.include?(["-filter", "FooClass"])
          assert config.commandline.include?(["-filter", "BarClass"])
        end
      end

      describe ".sdk_paths" do
        it "returns hash of SDKs" do
          paths = Configuration.sdk_paths(@xcode_dir)

          assert_equal @xcode_dir + "Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator.sdk", paths["iphonesimulator"]
          assert_equal @xcode_dir + "Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator9.3.sdk", paths["iphonesimulator9.3"]
          assert_equal @xcode_dir + "Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.11.sdk", paths["macosx"], "macosx is alias"
          assert_equal @xcode_dir + "Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.11.sdk", paths["macosx10.11"]

          assert_nil paths["appletvos"], "not-found SDK is not listed in hash"
          assert_nil paths["unknown"], "unknown SDK is not listed in hash"
        end
      end

      describe ".sdk_path" do
        it "returns path for given sdk name" do
          path = Configuration.sdk_path(:iphonesimulator, @xcode_dir)
          assert_equal @xcode_dir + "Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator.sdk", path
        end
      end
    end
  end
end
