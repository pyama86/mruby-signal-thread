assert('SignalThread#failed') do
  t = SignalThread.new { 1 + 1 }
  usleep 10 * 1000
  assert_false t.failed?

  t = SignalThread.new { raise "Something" }
  usleep 10 * 1000
  assert_true t.failed?

  t = SignalThread.new { nomethod }
  usleep 10 * 1000
  assert_true t.failed?
end

assert('SignalThread#exception') do
  t = SignalThread.new { raise "Something" }
  usleep 10 * 1000
  assert_kind_of RuntimeError, t.exception
  assert_equal "Something", t.exception.message
end
