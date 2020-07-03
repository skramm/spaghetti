#include <boost/asio.hpp>
#include <iostream>


void print(const boost::system::error_code& /*e*/)
{
  std::cout << "Hello, async callback world!\n";
}



int main()
{
	boost::asio::io_context io;
{
	boost::asio::executor_work_guard<boost::asio::io_context::executor_type> wg = boost::asio::make_work_guard(io);
}
	std::cout << "start\n";
	boost::asio::steady_timer t(io, boost::asio::chrono::seconds(2));
//	t.async_wait(&print);

	std::cout << "start run()\n";

	io.run(); // won't stop, even after end of delay

	std::cout << "End\n";  // never reached
}
