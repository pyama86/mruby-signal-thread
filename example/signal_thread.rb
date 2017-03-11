class SignalThread
  def self.trap(sig, *arg, &block)
    self.mask(sig)
    pr = Proc.new do
      SignalThread.wait(sig) do
        block.call(arg)
      end
    end

    Thread.new(pr) do |pr|
      pr.call
    end
  end
end

SignalThread.trap(:HUP, "hoge", "fuga") do |h,f|
  p h,f
end

puts "wait..."
loop { sleep 1 }
