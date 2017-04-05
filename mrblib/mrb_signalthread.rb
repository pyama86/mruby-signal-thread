class SignalThread < Thread
  def self.trap(sig, &block)
    strsig = sig.to_s
    mask(strsig)
    pr = Proc.new do
      wait(strsig) do
        block.call
      end
    end

    self.new(pr) do |pr|
      pr.call
    end
  end

  def self.trap_once(sig, &block)
    strsig = sig.to_s
    mask(strsig)
    pr = Proc.new do
      wait(strsig)
      block.call
    end

    self.new(pr) do |pr|
      pr.call
    end
  end

  def kill(sig)
    _kill(sig.to_s)
  end
end
