# guile-sctp
This is an extension for [guile](https://www.gnu.org/software/guile/) adding basic SCTP support.

## Installation
### Linux (Ubuntu)
For installing the required packages run:
```
sudo apt-get install guile-2.0 guile-2.0-dev libsctp-dev
```
Then download the sources, compile them and install the files:
```
wget https://github.com/nplab/guile-sctp/releases/download/v1.2.5/guile-sctp-1.2.5.tar.gz
tar xvfz guile-sctp-1.2.5.tar.gz
cd guile-sctp-1.2.5
./configure --prefix=/usr
make
sudo make install
```
### FreeBSD
For installing the required packages run:
```
sudo pkg install wget guile2
```
Then download the sources, compile them and install the files:
```
wget https://github.com/nplab/guile-sctp/releases/download/v1.2.5/guile-sctp-1.2.5.tar.gz
tar xvfz guile-sctp-1.2.5.tar.gz
cd guile-sctp-1.2.5
./configure
make
sudo make install
```
### Solaris
For installing the required packages run:
```
sudo pkg install guile
```
Then download the sources, compile them and install the files:
```
wget https://github.com/nplab/guile-sctp/releases/download/v1.2.5/guile-sctp-1.2.5.tar.gz
tar xvfz guile-sctp-1.2.5.tar.gz
cd guile-sctp-1.2.5
./configure
make
sudo make install
```
