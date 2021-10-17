FROM balenalib/raspberry-pi-debian:bullseye

RUN curl http://ftp.de.debian.org/debian/pool/main/libs/libseccomp/libseccomp2_2.5.2-2_armhf.deb  \
    > libseccomp2_2.5.2-2_armhf.deb
RUN dpkg -i libseccomp2_2.5.2-2_armhf.deb
CMD ["bash"]
