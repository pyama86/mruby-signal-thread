assert('SignalThread#trap') do
  a = 1
  SignalThread.trap(:HUP) do
    a = 2
  end

  SignalThread.trap(:USR1) do
    a = 3
  end

  Process.kill :HUP, Process.pid
  usleep 1000
  assert_true a == 2

  Process.kill :USR1, Process.pid
  usleep 1000
  assert_true a == 3
end

assert('SignalThread#trap with RTSignal') do
  begin
    a = 0
    SignalThread.trap(:RT1) do
      a = 10
    end
    SignalThread.queue Process.pid, :RT1
    usleep 1000
    assert_true a == 10
  rescue ArgumentError => e
    assert_equal "unsupported signal", e.message
  end
end
