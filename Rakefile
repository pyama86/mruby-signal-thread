MRUBY_CONFIG=File.expand_path(ENV["MRUBY_CONFIG"] || "build_config.rb")

file :mruby do
  sh "git clone git://github.com/mruby/mruby.git"
end

desc "compile binary"
task :compile => :mruby do
  sh "cd mruby && MRUBY_CONFIG=#{MRUBY_CONFIG} rake all"
end

desc "test"
task :test => :mruby do
  sh "cd mruby && MRUBY_CONFIG=#{MRUBY_CONFIG} rake all test"
end

desc "cleanup"
task :clean do
  sh "cd mruby && rake deep_clean"
end

desc "run docker"
task :docker do
  sh "docker build -t mruby:signal-thread ."
  sh "docker run -v `pwd`:/tmp/mruby-signal-thread -w /tmp/mruby-signal-thread -it mruby:signal-thread /bin/bash"
end

task :default => :test
