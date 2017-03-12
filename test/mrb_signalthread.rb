assert('SignalThread#trap') do
  a = 1
  th = SignalThread.trap(:HUP) do
    a = 2
  end

  Process.kill :HUP, Process.pid
  sleep 1
  a == 2
end
