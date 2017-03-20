class SignalThread
  def self.trap(sig, &block)
    strsig = sig.to_s
    mask(strsig)
    pr = Proc.new do
      wait(strsig) do
        block.call
      end
    end

    Thread.new(pr) do |pr|
      pr.call
    end
  end
end
