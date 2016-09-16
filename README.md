# guile-sctp
This is an extension for [guile](https://www.gnu.org/software/guile/) to add basic support for
using SCTP.

## Installation
### Linux (Ubuntu)
For installing the required packages run:
```
sudo apt-get install guile-2.0 guile-2.0-dev libsctp-dev
```
Then download the sources, compile and install it:
```
wget https://github.com/nplab/guile-sctp/releases/download/v1.2.2/guile-sctp-1.2.2.tar.gz
tar xvfz guile-sctp-1.2.2.tar.gz
cd guile-sctp-1.2.2
./configure --prefix=/usr
make
sudo make install
```
### FreeBSD
For installing the required packages run:
```
sudo pkg install wget guile2
```
Then download the sources, compile and install it:
```
wget https://github.com/nplab/guile-sctp/releases/download/v1.2.2/guile-sctp-1.2.2.tar.gz
tar xvfz guile-sctp-1.2.2.tar.gz
cd guile-sctp-1.2.2
./configure
make
sudo make install
```
