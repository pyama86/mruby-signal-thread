SignalThread.trap(:HUP, "hup") do |h|
  p h
end

SignalThread.trap(:USR1, "user1") do |h|
  p h
end

puts "wait..."
loop { sleep 1 }
