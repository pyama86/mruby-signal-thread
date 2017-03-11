# mruby-signal-thread   [![Build Status](https://travis-ci.org/pyama86/mruby-signal-thread.svg?branch=master)](https://travis-ci.org/pyama86/mruby-signal-thread)
SignalThread class
## install by mrbgems
- add conf.gem line to `build_config.rb`

```ruby
MRuby::Build.new do |conf|

    # ... (snip) ...

    conf.gem :github => 'pyama86/mruby-signal-thread'
end
```
## example
```ruby
p SignalThread.hi
#=> "hi!!"
t = SignalThread.new "hello"
p t.hello
#=> "hello"
p t.bye
#=> "hello bye"
```

## License
under the MIT License:
- see LICENSE file
