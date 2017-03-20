class SignalThread
  def self.trap(sig, &block)
    strsig = sig.to_s
    m = Mutex.new # :global => true
    m.lock
    mask(strsig)
    pr = Proc.new do
      wait(strsig) do
        m.unlock
        block.call
      end
    end

    Thread.new(pr) do |pr|
      pr.call
    end
  end
end
