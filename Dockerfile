FROM centos:latest
RUN yum -y groupinstall "Development Tools"
RUN yum -y install ruby \
    ruby-gems \
    yum \
    wget \
    git

RUN gem install rake
ADD . /tmp/mruby-signal-thread
WORKDIR /tmp/mruby-signal-thread
CMD rake test
