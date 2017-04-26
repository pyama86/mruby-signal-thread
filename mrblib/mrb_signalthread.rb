class SignalThread < Thread
  def self.trap(sig, options={}, &block)
    strsig = sig.to_s
    mask(strsig)

    if options[:detailed]
      self.new(block) do |pr|
        while true
          info = SignalThread.waitinfo(strsig)
          pr.call(info)
        end
      end
    else
      self.new(block) do |pr|
        while true
          SignalThread.wait(strsig)
          pr.call
        end
      end
    end
  end

  def self.trap_once(sig, options={}, &block)
    strsig = sig.to_s
    mask(strsig)

    if options[:detailed]
      self.new(block) do |pr|
        info = SignalThread.waitinfo(strsig)
        pr.call(info)
      end
    else
      self.new(block) do |pr|
        SignalThread.wait(strsig)
        pr.call
      end
    end
  end

  def kill(sig)
    _kill(sig.to_s)
  end
end
