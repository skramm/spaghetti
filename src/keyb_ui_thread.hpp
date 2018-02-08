/**
\file keyb_ui_thread.hpp
*/
#include <thread>
#include <mutex>

extern std::mutex* g_mutex;

template<typename FSM>
void
UI_thread( const FSM* fsm )
{
	{
		std::lock_guard<std::mutex> lock(*g_mutex);
		std::cout << "Thread start, enter key anytime\n";
	}
	bool quit(false);
    do
    {
		char key;
		std::cin >> key;
		{
			std::lock_guard<std::mutex> lock(*g_mutex);
			std::cout << "**********************KEY FETCH: " << key;

			switch( key )
			{
				case 'a':
					std::cout << ": switch to warning mode\n";
					fsm->processEvent( EV_WARNING_ON );
				break;
				case 'b':
					std::cout << ": switch to normal mode\n";
					fsm->processEvent( EV_WARNING_OFF );
				break;
				case 'c':
					std::cout << ": reset\n";
					fsm->processEvent( EV_RESET );
				break;
				case 'x':
					std::cout << ": x: QUIT\n";
					fsm->stop();
					quit = true;
				break;

				default:
					std::cout << ": invalid key" << std::endl;
			}
		}
    }
    while( !quit );
	fsm->printLoggedData( std::cout );
}

