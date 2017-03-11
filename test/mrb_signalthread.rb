##
## SignalThread Test
##

assert("SignalThread#hello") do
  t = SignalThread.new "hello"
  assert_equal("hello", t.hello)
end

assert("SignalThread#bye") do
  t = SignalThread.new "hello"
  assert_equal("hello bye", t.bye)
end

assert("SignalThread.hi") do
  assert_equal("hi!!", SignalThread.hi)
end
