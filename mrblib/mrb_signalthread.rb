class SignalThread
  def self.trap(sig, &block)
    mask(sig)
    pr = Proc.new do
      wait(sig) do
        block.call
      end
    end

    Thread.new(pr) do |pr|
      pr.call
    end
  end
end
