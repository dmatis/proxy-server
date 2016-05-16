# Proxy Server

#### A Proxy Server that can restrict access to specific websites

## Program Features:

0. Multi-threaded to allow for simultaneous connections
1. Caching to provide faster response times
2. Filters all traffic from specified domains

## To Start:

0. Download entire project
1. Type *make* in the command line to run the provided Makefile

## To Run:

**From the command line, type:**<br />
*./proxyFilter PORT banned.txt*<br />

*PORT* is the desired port to run the server on<br />
*banned.txt* is the file containing list of banned websites<br />

**Once the server is running, open another terminal and run:**<br />
*nc localhost PORT < request.txt*

where *request.txt* contains the URL request

**You should now see the content of the website displayed in the client terminal, or an error code indicating it is a blacklisted site**

Once working, you can configure your browser's proxy settings to see the results in the browser window

