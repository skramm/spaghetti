#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <iostream>


void print(const boost::system::error_code& /*e*/, boost::asio::steady_timer* t, int* c )
{
	std::cout << "async callback count=" << *c << "\n";
	if( *c < 3 )
	{
		t->expires_from_now( boost::asio::chrono::seconds(1) );
		(*c)++;
		t->async_wait(boost::bind(print, boost::asio::placeholders::error, t, c));
	}
}



int main()
{
	boost::asio::io_context io;

	int count = 0;
	std::cout << "start\n";
	boost::asio::steady_timer t(io, boost::asio::chrono::seconds(2));

	t.async_wait(
		boost::bind(
			print,
			boost::asio::placeholders::error, &t, &count
		)
	);

	std::cout << "start run()\n";

	io.run();

	std::cout << "End\n";
}
