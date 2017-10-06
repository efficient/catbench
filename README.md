CATBench
========
This repository contains the code for CATBench, the Intel Cache Allocation Technology benchmarking suite described in our tech report, "Simple Cache Partitioning for Networked Workloads."

License
-------
With the exception of any orphaned branches (including but not limited to those containing third-party patches to the Linux kernel), the entire contents and history of this repository are distributed under the following license:

	Copyright 2015, 2016, and 2017 Carnegie Mellon University

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

		http://www.apache.org/licenses/LICENSE-2.0

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.

Setup and invocation
====================
The rest of this document describes how to set everything up and rerun the experiments from the paper.  You'll need a LAN containing an etcd directory server, CAT server, and MICA client.  Furthermore, the latter two (experimental) machines will need two direct DPDK-capable links.

Software versions
-----------------
To obtain the results in the paper, we used:
* Linux 4.4.0, patched with Fenghua Yu <fenghua.yu@intel.com>'s 32-patch x86/intel_rdt series (available on the patches branch)
* GCC 6
* etcd 2.2.1, available from the etcd project at https://github.com/coreos/etcd/releases?after=v2.3.0-alpha.0
* DPDK 16.07
* TensorFlow 1.0
* Hadoop 2.7.4
* gRPC 1.6.6

However, we recommend these more modern versions of some of these, which were the last ones tested (and these steps assume them):
* Linux 4.13.0
* GCC 6.3.0
* etcd 3.1.8
* DPDK 16.11
* TensorFlow 1.3

Directory server
----------------
You need to run an etcd instance on some machine besides the two you use to run the experiments.  This is used for bootstrapping MICA clients so that they discover the server automatically.  On whatever box you choose, run the following commands (replacing IP with that box's external IP):

	~$ sudo apt-get install etcd-server
	~$ sudo systemctl stop etcd
	~$ etcd --listen-client-urls=http://IP:50505 --advertise-client-urls=http://IP:50505
	# then leave it running

Server setup
------------
On the server (i.e. the machine that has CAT), follow the steps in the following sections.  Note that you can skip some of the third-party systems if you don't want to run all the mites and contenders.

### CATBench
You'll need to check out this repository and build a prerequisite tool.  We'll assume it's done in the root of your home directory:

	~$ git clone git://github.com/efficient/catbench
	~$ make -C catbench setprgrp
	~$

### Linux
Build, install, and boot the Linux kernel:

	~$ git clone -b v4.13 git://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux
	~$ cd linux
	~/linux$ make nconfig # and say y to CONFIG_INTEL_RDT_A
	~/linux$ make bindeb-pkg
	~/linux$ cd dpkg -i linux-*.deb
	~/linux$ reboot
	# and wait for it to come back up, making sure it boots into the new kernel

### Perf
Install the matching version of Perf, then cd back to your home directory:

	~$ cd linux/tools/perf
	~/linux/tools/perf$ make
	~/linux/tools/perf$ ~/catbench/package `uname -r | cut -d. -f1-2`
	~/linux/tools/perf$ sudo dpkg -i linux-perf-*.deb
	~/linux/tools/perf$ cd
	~$

### DPDK
This repository contains rules to clone and build the correct version of DPDK.  Just do:

	~$ make -C catbench/clients lockserver-ng
	~$

### MICA 2
Below, replace HOSTNAME with the FQDN of the etcd server you set up earlier.

	~$ git clone git://github.com/efficient/mica2-catbench
	~$ cd mica2-catbench/build
	~/mica2-catbench/build$ ln -s ../../catbench/external/dpdk
	~/mica2-catbench/build$ ln -s ../src/mica/test/server.json
	~/mica2-catbench/build$ sed -i s/YOUR_ETCD_HOST/HOSTNAME/ server.json
	~/mica2-catbench/build$ cmake ..
	~/mica2-catbench/build$ make
	~/mica2-catbench/build$ cd
	~$

### gRPC
You'll need to build both upstream gRPC and our framework integration:

	~$ git clone -b v1.6.6 git://github.com/grpc/grpc
	# follow the upstream instructions to build and install grpc, then cd back hame
	~$ git clone -b grpc git://github.com/efficient/mica2-catbench grpc-catbench
	# edit line 210 of grpc-catbench/src/mica/test/netbench.cc and substitute the IP address of your server
	~$ cd grpc-catbench/build
	~/grpc-catbench/build$ ln -s ../../catbench/external/dpdk
	~/grpc-catbench/build$ cmake ..
	~/grpc-catbench/build$ make
	~/grpc-catbench/build$ cd
	~$

### Lepton
The framework integration is all within this repository, so just build upstream and make a couple symlinks:

	~$ git clone -b 1.2.1 git://github.com/dropbox/lepton
	~$ cd lepton
	~/lepton$ ./autogen.sh
	~/lepton$ ./configure
	~/lepton$ make
	~/lepton$ cd
	~$ ln -s ../lepton/lepton catbench
	~$ ln -s ../lepton/images catbench
	~$

### TensorFlow (TM)
You need to install the latest version of TensorFlow and generate the MNIST input data (which needs to be redone if you reboot because it's stored under /tmp):

	~$ pip3 install tensorflow # or pip for Python 2
	~$ git clone git://github.com/tensorflow/models
	~$ python3 models/official/mnist/convert_to_records.py # or python, depending on your choice above
	~$ ln -s models/official/mnist/mnist.py catbench
	~$

### Hadoop
You'll need to download a 2.7.x release of Hadoop from http://hadoop.apache.org/releases.html.  Then extract and symlink it and generate the input data:

	~$ tar -xvf hadoop-2.7.*.tar.gz
	~$ ln -s hadoop-2.7.* catbench/hadoop
	~$ cd catbench
	~/catbench$ bash generate_hadoop_input.sh
	~/catbench$ cd
	~$

### SSH creds
This account on the server will need to be able to SSH authenticate to your MICA client machine without a password.  If necessary, set up pubkey authentication, replacing USER@CLIENT with the appropriate username/hostname combo:

	~$ yes "" | ssh-keygen
	~$ ssh-copy-id USER@CLIENT
	~$

### sudo access
The server needs to be able to sudo on the client across SSH, and you won't want to be promtped for a password in the middle of the experiment.  If you don't want to configure the client account for NOPASSWD, you can do this with your CLIENT_PASSWORD on the server (otherwise, just make it an empty file):

	~$ echo CLIENT_PASSWORD >catbench/.network_rtt_pw
	~$

Client setup
------------
Make sure this machine is running a recent (4-something, and maybe as recent as 4.13) Linux kernel and has at least GCC 5.  Then:
* Follow the same CATBench instructions as for the server.
* To build DPDK, do: `~$ make -C catbench/clients lockclient`
* Follow the same MICA 2 instructions as for the server, but replace every instance of `server.json` with `netbench.json`
* Follow the same gRPC instructions as for the server.

Initialization
--------------
After each reboot, you need to perform some initialization steps before running any experiments.  We provide sample scripts for this purpose; however, they will likely require some tweaking for your site, especially where they make reference to PCI device identifiers (which can be discovered using lspci) and kernel network interface names (which are listed using ip link).  One of the directly-patched NICs will belong to DPDK, and will be used for MICA's network traffic during the experiment; the other will continue to be managed by the kernel, and will be used for an SSH control channel.  Make sure you're consistent across machines in your choice between the two (i.e. don't patch one machine's userland NIC to the other's kernel NIC).

### Server
There are two different scripts, the former of which probably won't require any changes on your part.  Once you've updated the interfaces in the latter, do:

	~$ ln -s catbench/external/dpdk
	~$ catbench/setup/after_reboot
	~$ catbench/setup/after_reboot_module

Also remember to regenerate the TensorFlow input data (see above) if you need to run that contender after rebooting.

### Client
On this box, there's only one script to run, but you'll again need to modify it to reflect your PCI device ID (instead of 02:00.0) and kernel interface name (not p1p2):

	~$ ln -s catbench/external/dpdk
	~$ catbench/setup/after_reboot_router

Running experiments!
--------------------
Let's assume you want to rerun one of the experiments from the paper.  The command used to invoke it is recorded in the data file from the original run.

### Original data files
The data files containing the full results from the experimental runs presented in the paper may be downloaded from: 
https://github.com/efficient/catbench/releases

Assuming you placed the resulting archive in your home directory, extract it with:

	~$ tar xzf ~/Tech report data files.tar.gz
	~$

### Launching a run
Now move into this repository and reinvoke the experiment using a command read back from the appropriate data file.  For example, to regenerate one of the repetitions whose median is graphed in Figure 3b, working against client account/machine USER@HOST you would do:

	~$ cd catbench
	~/catbench$ ./driver `jaguar/jaguar get ~/Tech\ report\ data\ files/fig3/b/tensorflow-mite_throughput-2mb-4ways-skew99-hot95.json meta.command | sed 's/wimpy/USER@HOST/'`
	# and wait for hours

When the experiment (finally) completes, the results will be in a timestamped JSON file in that directory.

### Better reproducability
Most of the experimental results record additional details about the environment under which they were collected, including the repository revision and running kernel; these can be found under the `meta` section of the corresponding JSONs.  Extract them with jaguar, jq, or a similar utility.  You'll probably need to change other things about your environment/installations in order to get past versions running, though.

### Running your own experiments
This is largely left as an exercise to the reader.  See ./network_rtt's USAGE string and look at how existing runs were invoked for reference/inspiration.

A few things that might help, though:
* Make sure you prefix the environment variable assignments and ./network_rtt invocation with ./driver, which is the script responsible for writing everything to the JSON file.
* The experiment's independent variable is controlled by the mode switch.
* You specify the contender data collection strategy, mite, and contender using a comma-separated argument that refers to plugin scripts located in the networked subdirectory.  You can also try to author your own plugins in this directory if you're an overachiever.

### Batch runs
If you need to launch a bunch of experimental runs in sequence, but want to be able to monitor their progress and might need to alter the queue as you see the results of existing runs, check the USAGE string of the ./batch script.  You can make a jobfile that records what you want to run, and this script will update it with timestamps as each job completes, taking care to reload its contents after each run in case you've made live updates of your own.
