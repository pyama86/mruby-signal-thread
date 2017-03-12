MRuby::Build.new do |conf|
  toolchain :gcc
  conf.gembox 'default'
  conf.gem mgem: 'mruby-sleep'
  conf.gem mgem: 'mruby-process'
  conf.gem :github => 'pyama86/mruby-thread', branch: 'support-migrate-proc'
  conf.linker.libraries << ['pthread']
  conf.gem File.expand_path(File.dirname(__FILE__))
  conf.enable_test
end
