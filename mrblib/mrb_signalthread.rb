class SignalThread < Thread
  def self.trap(sig, options={}, &block)
    strsig = sig.to_s
    mask(strsig)
    pr = if options[:detailed]
           Proc.new do
             SignalThread.waitinfo(strsig) do |info|
               block.call(info)
             end
           end
         else
           Proc.new do
             SignalThread.wait(strsig) do
               block.call
             end
           end
         end

    self.new(pr) do |pr|
      pr.call
    end
  end

  def self.trap_once(sig, options={}, &block)
    strsig = sig.to_s
    mask(strsig)
    pr = if options[:detailed]
           Proc.new do
             info = SignalThread.waitinfo(strsig)
             block.call(info)
           end
         else
           Proc.new do
             SignalThread.wait(strsig)
             block.call
           end
         end

    self.new(pr) do |pr|
      pr.call
    end
  end

  def kill(sig)
    _kill(sig.to_s)
  end
end
