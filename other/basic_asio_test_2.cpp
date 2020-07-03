#include <boost/asio.hpp>
#include <iostream>


void print(const boost::system::error_code& /*e*/)
{
  std::cout << "Hello, async callback world!\n";
}



int main()
{
	boost::asio::io_context io;

	std::cout << "start\n";
	boost::asio::steady_timer t(io, boost::asio::chrono::seconds(2));
	t.async_wait(&print);

	std::cout << "start run()\n";
	io.run();

	std::cout << "End\n";
}
