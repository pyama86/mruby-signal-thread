class SignalThread
  def self.trap(sig, *arg, &block)
    mask(sig)
    pr = Proc.new do
      wait(sig) do
        block.call(arg)
      end
    end

    Thread.new(pr) do |pr|
      pr.call
    end
  end
end
