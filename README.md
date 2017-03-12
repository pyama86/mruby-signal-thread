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
SignalThread.trap(:HUP) do
  puts "foo"
end

puts "wait..."
loop { sleep 1 }
```

```bash
$ mruby/bin/mruby example/signal_thread.rb &
wait...
$ kill -HUP $(pidof mruby)
foo
```

## License
under the MIT License:
- see LICENSE file
