# Simple thread-based tcp proxy

Simple tcp proxy that opens 2 threads per connection

Example usage:

  * Forward ports 80 and 443 to 216.58.212.110 `./tcp_proxy -i 216.58.212.110`
  * Forward ports 23 and 80 to 192.168.1.5 `./tcp_proxy -i 192.168.1.5 -h 23,80`
  * Forward port 80 to netflix.com `./tcp_proxy -h netflix.com -h 80`
