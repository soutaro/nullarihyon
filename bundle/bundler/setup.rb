require 'rbconfig'
# ruby 1.8.7 doesn't define RUBY_ENGINE
ruby_engine = defined?(RUBY_ENGINE) ? RUBY_ENGINE : 'ruby'
ruby_version = RbConfig::CONFIG["ruby_version"]
path = File.expand_path('..', __FILE__)
$:.unshift "#{path}/../#{ruby_engine}/#{ruby_version}/gems/i18n-0.7.0/lib"
$:.unshift "#{path}/../#{ruby_engine}/#{ruby_version}/gems/json-1.8.3/lib"
$:.unshift "#{path}/../#{ruby_engine}/#{ruby_version}/gems/minitest-5.9.0/lib"
$:.unshift "#{path}/../#{ruby_engine}/#{ruby_version}/gems/thread_safe-0.3.5/lib"
$:.unshift "#{path}/../#{ruby_engine}/#{ruby_version}/gems/tzinfo-1.2.2/lib"
$:.unshift "#{path}/../#{ruby_engine}/#{ruby_version}/gems/activesupport-4.2.6/lib"
$:.unshift "#{path}/"
$:.unshift "#{path}/../#{ruby_engine}/#{ruby_version}/gems/claide-1.0.0/lib"
$:.unshift "#{path}/../#{ruby_engine}/#{ruby_version}/gems/colored-1.2/lib"
$:.unshift "#{path}/../#{ruby_engine}/#{ruby_version}/gems/xcodeproj-1.0.0/lib"
