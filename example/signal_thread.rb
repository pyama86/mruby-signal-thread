a = "a"
b = "b"

SignalThread.trap(:HUP) do
  puts a
end

SignalThread.trap(:USR1) do
  puts b
end

puts "wait..."
loop { sleep 1 }
