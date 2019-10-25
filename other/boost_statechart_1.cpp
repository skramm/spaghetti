/*
Example taken from https://www.youtube.com/watch?v=Apm2KNj8GW4
*/

#include <iostream>
#include <boost/statechart/state_machine.hpp>
#include <boost/statechart/simple_state.hpp>
#include <boost/statechart/transition.hpp>
#include <boost/statechart/custom_reaction.hpp>

using namespace std;

namespace sc=boost::statechart;

struct lostMoney : sc::event<lostMoney> {};
struct gotMoney : sc::event<gotMoney> {};

struct HappyState;
struct SadState;
struct Person: sc::state_machine<Person,HappyState> {};


struct HappyState: sc::simple_state<HappyState,Person>
{
	HappyState()
	{
		cout << "Happy !\n";
	}
	typedef sc::transition<lostMoney,SadState> reactions;
};

struct SadState: sc::simple_state<SadState,Person>
{
	SadState()
	{
		cout << "Sad !\n";
	}
	typedef sc::transition<gotMoney,HappyState> reactions;
};


int main()
{
	cout << "start\n";
	Person sm;
	sm.initiate();
	sm.process_event( lostMoney() );
	sm.process_event( gotMoney() );
}

