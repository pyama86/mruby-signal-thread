SignalThread.trap(:HUP, "hoge", "fuga") do |h,f|
  p h,f
end

puts "wait..."
loop { sleep 1 }
