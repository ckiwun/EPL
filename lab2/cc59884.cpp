#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

#include "cc59884.h"
#include "CraigUtils.h"
#include "Event.h"
#include "ObjInfo.h"
#include "Params.h"
#include "Random.h"
#include "Window.h"

#ifdef _MSC_VER
using namespace epl;
#endif
using namespace std;
using String = std::string;

Initializer<cc59884> __cc59884_initializer;

String cc59884::species_name(void) const
{
	return "cc59884";
}

Action cc59884::encounter(const ObjInfo& info)
{
	if (info.species == species_name()) {
		/* don't be cannibalistic */
		set_course(info.bearing + M_PI);
		return LIFEFORM_IGNORE;
	}
	else {
		hunt_event->cancel();
		SmartPointer<cc59884> self = SmartPointer<cc59884>(this);
		hunt_event = new Event(0.0, [self](void) { self->myhunt(); });
		return LIFEFORM_EAT;
	}
}

void cc59884::initialize(void) {
	LifeForm::add_creator(cc59884::create, "cc59884");
}

/*
* REMEMBER: do not call set_course, set_speed, perceive, or reproduce
* from inside the constructor!!!!
* you must wait until the object is actually alive
*/
cc59884::cc59884() {
	SmartPointer<cc59884> self = SmartPointer<cc59884>(this);
	new Event(0, [self](void) { self->startup();});
}

cc59884::~cc59884() {}

void cc59884::startup(void) {
	set_course(drand48() * 2.0 * M_PI);
	set_speed(2 + 5.0 * drand48());
	SmartPointer<cc59884> self = SmartPointer<cc59884>(this);
	update_turn_around = 0;
	hunt_event = new Event(0, [self](void) { self->myhunt(); });
}

void cc59884::spawn(void) {
	SmartPointer<cc59884> child = new cc59884;
	reproduce(child);
}


Color cc59884::my_color(void) const {
	return BLUE;
}

SmartPointer<LifeForm> cc59884::create(void) {
	return new cc59884;
}


void cc59884::myhunt(void) {
	const String fav_food ="Algae";

	hunt_event = nullptr;
	if (health() == 0.0) { return; } // we died

	double my_perceive_range = health() * 50;
	if (my_perceive_range < 10) my_perceive_range = 10;
	ObjList prey = perceive(my_perceive_range);

	double best_d = HUGE;
	bool found_fav_food = false;
	
	//algae first
		for (ObjList::iterator i = prey.begin(); i != prey.end(); ++i) {
			if ((*i).species == fav_food) {
				if (best_d >(*i).distance) {
					set_course((*i).bearing);
					set_speed(10);
					best_d = (*i).distance;
					found_fav_food = true;
					update_turn_around=0;
				}
			}
		}
	//if no ones around, go the opposite way
	if (!found_fav_food) {
		if (!update_turn_around) {
			update_turn_around++;
			set_speed(1.0 + 1.0*drand48());
		}
		else {
			update_turn_around = 0;
			double new_course = get_course() + M_PI;
			if (new_course > 2 * M_PI) new_course = new_course - 2 * M_PI;
			set_course(new_course);
			set_speed(2 + 5.0*drand48());
		}
	}
	SmartPointer<cc59884> self = SmartPointer<cc59884>(this);
	hunt_event = new Event(10.0, [self](void) { self->myhunt(); });

	if (health() >= 4.0) spawn();
}
