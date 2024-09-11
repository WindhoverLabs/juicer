FROM ubuntu:20.04
RUN apt update
ARG DEBIAN_FRONTEND=noninteractive
ENV TERM xterm-256color
RUN apt-get update && \
      apt-get -y install sudo

RUN apt-get install -y gcc-multilib
RUN apt-get install -y g++-multilib
RUN apt-get install -y libdwarf-dev
RUN apt-get install -y make
RUN apt-get install -y libelf-dev
RUN apt-get install -y libsqlite3-dev
RUN apt-get install -y libssl-dev
RUN apt-get install -y doxygen
RUN apt-get install -y gcovr

RUN mkdir /home/docker
COPY . /home/docker/juicer


RUN cd /home/docker/juicer && make clean
RUN cd /home/docker/juicer && make docs

RUN cd /home/docker/juicer && make
RUN cd /home/docker/juicer && make clean
RUN cd /home/docker/juicer && make all
WORKDIR /home/docker/juicer/build
RUN ./juicer-ut "[Enumeration]"
RUN ./juicer-ut "[main_test#1]"
RUN ./juicer-ut "[main_test#2]"
RUN ./juicer-ut "[main_test#3]"
RUN ./juicer-ut "[main_test#4]"
RUN ./juicer-ut "[main_test#5]"
RUN ./juicer-ut "[main_test#6]"
RUN ./juicer-ut "[main_test#7]"
RUN ./juicer-ut "[main_test#8]"
RUN ./juicer-ut "[main_test#9]"
RUN ./juicer-ut "[main_test#10]"
RUN ./juicer-ut "[main_test#11]"
RUN ./juicer-ut "[main_test#12]"
RUN ./juicer-ut "[main_test#13]"
RUN ./juicer-ut "[main_test#14]"
RUN ./juicer-ut "[main_test#15]"
RUN ./juicer-ut "[main_test#16]"
RUN ./juicer-ut "[main_test#17]"
RUN ./juicer-ut "[Module]"
RUN ./juicer-ut "[Symbol]"


RUN ./juicer-ut "[main_test_dwarf4#1]"
RUN ./juicer-ut "[main_test_dwarf4#2]"
RUN ./juicer-ut "[main_test_dwarf4#3]"
RUN ./juicer-ut "[main_test_dwarf4#4]"
RUN ./juicer-ut "[main_test_dwarf4#5]"
RUN ./juicer-ut "[main_test_dwarf4#6]"
RUN ./juicer-ut "[main_test_dwarf4#7]"
RUN ./juicer-ut "[main_test_dwarf4#8]"
RUN ./juicer-ut "[main_test_dwarf4#9]"
RUN ./juicer-ut "[main_test_dwarf4#10]"
RUN ./juicer-ut "[main_test_dwarf4#11]"
RUN ./juicer-ut "[main_test_dwarf4#12]"
RUN ./juicer-ut "[main_test_dwarf4#13]"
RUN ./juicer-ut "[main_test_dwarf4#14]"
RUN ./juicer-ut "[main_test_dwarf4#15]"
RUN ./juicer-ut "[main_test_dwarf4#16]"
RUN ./juicer-ut "[main_test_dwarf4#17]"
RUN cd /home/docker/juicer && make coverage
#Useful for CI
RUN cd /home/docker/juicer &&  gcovr --filter /home/docker/juicer/src/ --object-directory /home/docker/juicer/build/ut_obj/ --xml  coverage.gcov 



