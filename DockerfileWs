FROM ubuntu:18.04

RUN apt-get update && apt-get install -y software-properties-common build-essential git cmake

RUN mkdir /opt/install
WORKDIR /opt/install

RUN git clone https://github.com/biometric-technologies/nist-biometric-image-software-nbis.git &&  \
    chmod +x -R nist-biometric-image-software-nbis &&  \
    cd /opt/install/nist-biometric-image-software-nbis  \
    && ./setup.sh /usr/local --64 --without-X11  \
    && make config  \
    && make it  \
    && make install LIBNBIS=no

RUN git clone https://github.com/usnistgov/biomdi.git &&  \
    chmod +x -R biomdi &&  \
    cd /opt/install/biomdi  \
    && make  \
    && make install

WORKDIR /opt/install

COPY . .

RUN mkdir build-converter  \
    && cd build-converter  \
    && cmake ..  \
    && make \
    && make install

FROM ubuntu:18.04

LABEL maintainer="slava.goronostal@gmail.com"

COPY --from=0 /usr/local/lib /usr/local/lib
#COPY --from=0 /opt/install/build-converter/*.a /opt/install/build-converter/*.so /usr/local/lib/

RUN apt-get update \
    && apt-get install -y build-essential openjdk-11-jre \
    && rm -rf /var/lib/apt/lists/* \
    && apt-get clean -y
