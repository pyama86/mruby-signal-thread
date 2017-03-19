class SignalThread
  def self.trap(sig, &block)
    m = Mutex.new # :global => true
    m.lock
    mask(sig)
    pr = Proc.new do
      wait(sig) do
        m.unlock
        block.call
      end
    end

    Thread.new(pr) do |pr|
      pr.call
    end
  end
end
