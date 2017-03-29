assert('SignalThread#trap') do
  a = 1
  SignalThread.trap(:HUP) do
    a = 2
  end

  SignalThread.trap(:USR1) do
    a = 3
  end

  Process.kill :HUP, Process.pid
  sleep 1
  a == 2

  Process.kill :USR1, Process.pid
  sleep 1
  a == 3
end

assert('SignalThread#kill') do
  a = 1
  th = SignalThread.trap(:HUP) do
    a = 2
  end

  th.alive?
  th.kill :HUP
  sleep 1
  a == 2
end
