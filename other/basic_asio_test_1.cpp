#include <boost/asio.hpp>
#include <iostream>

int main()
{

	boost::asio::io_context io;
	std::cout << "start\n";
	boost::asio::steady_timer t( io, boost::asio::chrono::seconds(2) );
	t.wait();
	std::cout << "Hello, sync world!\n";
}
