assert('SignalThread#trap') do
  q = Queue.new

  th1 = SignalThread.trap(:HUP) do
    q.push 2
  end

  th2 = SignalThread.trap(:USR1) do
    q.push 3
  end

  th1.kill :HUP
  usleep 1000
  assert_equal 2, q.pop

  th2.kill :USR1
  usleep 1000
  assert_equal 3, q.pop
end

assert('SignalThread#trap with RTSignal') do
  begin
    q = Queue.new
    t = SignalThread.trap(:RT1) do
      q.push 10
    end
    t.kill :RT1
    usleep 1000
    assert_equal 10, q.pop
  rescue ArgumentError => e
    assert_equal "unsupported signal", e.message
  end
end

assert('SignalThread#trap_once') do
  q = Queue.new
  t = SignalThread.trap_once(:USR1) do
    q.push 4
  end

  t.kill :USR1
  usleep 1000
  assert_equal 4, q.pop
  assert_false t.alive?
end

assert('SignalThread#trap, detailed: true') do
  q = Queue.new
  t = SignalThread.trap(:USR2, detailed: true) do |info|
    q.push info.class.to_s.to_sym
  end

  t.kill :USR2
  usleep 1000
  assert_equal :SigInfo, q.pop
end

assert('SignalThread#thread_id') do
  q = Queue.new
  th = SignalThread.trap(:HUP) do
    q.push 2
  end

  SignalThread.kill_by_thread_id th.thread_id, :HUP
  usleep 1000
  assert_equal 2, q.pop
end
