assert("SignalThread#cancel") do
  start = Time.now.to_f
  t = SignalThread.new { sleep 10 }
  v = t.cancel
  t.join if t.alive?
  fin = Time.now.to_f

  assert_true (fin - start) < 1.0
  assert_equal :canceled, v
end
