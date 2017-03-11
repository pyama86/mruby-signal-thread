MRuby::Gem::Specification.new('mruby-signal-thread') do |spec|
  spec.license = 'MIT'
  spec.authors = 'pyama86'
  spec.add_dependency 'mruby-thread'  , github: 'pyama86/mruby-thread', branch: 'support-migrate-proc'
end
