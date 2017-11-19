# Dirty, there is no need to be nice
FROM debian:buster

ENV XSYSGUARD_VERSION=0.2.1

RUN apt-get update
RUN apt-get install -y build-essential devscripts lintian
RUN apt-get install -y wget libiw-dev libsensors4-dev libimlib2-dev libstatgrab-dev statgrab xsltproc source-highlight python docbook-xsl

RUN mkdir /build \
&& cd /build \
&& wget https://github.com/mixalturek/xsysguard/archive/${XSYSGUARD_VERSION}.tar.gz \
&& mv ${XSYSGUARD_VERSION}.tar.gz xsysguard_${XSYSGUARD_VERSION}.orig.tar.gz \
&& tar -xf xsysguard_${XSYSGUARD_VERSION}.orig.tar.gz

RUN cd /build/xsysguard-${XSYSGUARD_VERSION} && debuild -us -uc
