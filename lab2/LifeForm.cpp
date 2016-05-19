/* main test simulator */
#include <iostream>
#include "CraigUtils.h"
#include "Window.h"
#include "tokens.h"
#include "ObjInfo.h"
#include "QuadTree.h" 
#include "Params.h"
#include "LifeForm.h"
#include "Event.h"
#include "Random.h"

using namespace std;


template <typename T>
void bound(T& x, const T& min, const T& max) {
	assert(min < max);
	if (x > max) { x = max; }
	if (x < min) { x = min; }
}



ObjInfo LifeForm::info_about_them(SmartPointer<LifeForm> neighbor) {
	ObjInfo info;

	info.species = neighbor->species_name();
	info.health = neighbor->health();
	info.distance = pos.distance(neighbor->position());
	info.bearing = pos.bearing(neighbor->position());
	info.their_speed = neighbor->speed;
	info.their_course = neighbor->course;
	return info;
}

//move around
void LifeForm::update_position(void) {
	if (!is_alive) return;
	//cout << "update position\n";
	if (get_speed() == 0) return;
	double time_delta = Event::now() - update_time;
	if (time_delta < 0.001) {
		//cout << "delta < 0.001" << endl;
		return;
	}
	double distance = time_delta * get_speed();
	energy -= movement_cost(get_speed(), time_delta);
	if (energy < min_energy) {
		die();
		return;
	}

	Point pos_old = pos;
	//cout << "position change from " << pos_old.xpos << "," << pos_old.ypos << " to ";
	Point pos_new = pos + Point{ distance*cos(get_course()), distance*sin(get_course()) };
	//cout << pos_new.xpos << "," << pos_new.ypos << endl;
	if (space.is_out_of_bounds(pos_new)) {
		//cout << "are you stuck here" << endl;
		die();
	}
	else {
		pos = pos_new;
		space.update_position(pos_old, pos_new);
		update_time = Event::now();
	}
}

void LifeForm::border_cross(void){
	update_position();
	check_encounter();
	compute_next_move();
}

void LifeForm::compute_next_move(void) {
	//cout << "compute_next_move\n";
	if (!is_alive) return;
	if (get_speed() == 0) return;
	bool direction;
	double dist_to_edge = space.distance_to_edge(pos,get_course());
	//cout << "dist to edge : " <<  dist_to_edge << endl;
	double speed_on_direction = direction ? get_speed()*fabs(cos(get_course())) : get_speed()*fabs(sin(get_course()));
	double time_to_edge = dist_to_edge / get_speed() + Point::tolerance;
	SmartPointer<LifeForm> self = SmartPointer<LifeForm>(this);
	border_cross_event = new Event{ time_to_edge, [self](void) {self->border_cross(); } };
}

void LifeForm::region_resize(void){
	//cout << "region resize call back" << endl;
	update_position();
	border_cross_event->cancel();
	compute_next_move();
}

void LifeForm::set_course(double dir) {
	update_position();
	border_cross_event->cancel();
	course = dir;
	compute_next_move();
}

void LifeForm::set_speed(double v) {
	if (v > max_speed) v = max_speed;
	update_position();
	border_cross_event->cancel();
	speed = v;
	compute_next_move();
}

//encounter
void LifeForm::check_encounter(void) {
	//cout << "check_encounter\n";
	if (!is_alive) return;
	std::vector<SmartPointer<LifeForm>> observed_objs = space.nearby(pos, encounter_distance);
	//cout << "pos is " << pos.xpos << "," << pos.ypos << endl;
	//cout << " with " << observed_objs.size() << " objs" << endl;
	//if (!observed_objs.empty()) {
	//	cout << observed_objs[0]->species_name() << endl;
	//	cout << "observed obj's position is " << observed_objs[0]->pos.xpos << "," << observed_objs[0]->pos.ypos << endl;
	//}
	SmartPointer<LifeForm> self = SmartPointer<LifeForm>(this);
	if (!observed_objs.empty())
		resolve_encounter(observed_objs[0]);//assume always encounter one object
}

void LifeForm::resolve_encounter(SmartPointer<LifeForm> hello) {//TODO
	//cout << "resolve encounter" << endl;
	if (!is_alive) return;
	energy -= encounter_penalty;
	if (energy < min_energy) {
		die();
		return;
	}
	SmartPointer<LifeForm> self = SmartPointer<LifeForm>(this);
	//cout << "self species is " << self->species_name() << " and hello species is " << hello->species_name() << endl;
	//cout << "self pos is " << self->pos.xpos << "," << self->pos.ypos << endl;
	//cout << "hello pos is " << hello->pos.xpos << "," << hello->pos.ypos << endl;
	//assert(self.operator->() != hello.operator->());
	int self_encounter_result = self->encounter(info_about_them(hello));
	int hello_encounter_result = hello->encounter(hello->info_about_them(self));
	if (hello->is_alive == false || self->is_alive == false)  return;
	if (self_encounter_result == LIFEFORM_EAT&&hello_encounter_result == LIFEFORM_IGNORE) {
		double rand_num = drand48();
		if (rand_num < eat_success_chance(energy, hello->energy))
			eat(hello);
	}
	else if (self_encounter_result == LIFEFORM_EAT&&hello_encounter_result == LIFEFORM_EAT) {
		double rand_num1 = drand48();
		double rand_num2 = drand48();
		double rand_num3 = drand48();
		if (rand_num1 < eat_success_chance(energy, hello->energy) && !rand_num2 < eat_success_chance(hello->energy, energy))
			eat(hello);
		else if (!rand_num1 < eat_success_chance(energy, hello->energy) && rand_num2 < eat_success_chance(hello->energy, energy))
			hello->eat(self);
		else if (rand_num1 < eat_success_chance(energy, hello->energy) && !rand_num2 < eat_success_chance(hello->energy, energy)) {
			switch (encounter_strategy) {
			case EVEN_MONEY:
				if (rand_num3 <= 0.5)
					eat(hello);
				else
					hello->eat(self);
				break;
			case BIG_GUY_WINS:
				if (self->energy >= hello->energy)
					eat(hello);
				break;
			case UNDERDOG_IS_HERE:
				if (rand_num3 <= energy/ energy + hello->energy)
					eat(hello);
				else
					hello->eat(self);
				break;
			case FASTER_GUY_WINS:
				if (self->get_speed() >= hello->get_speed())
					eat(hello);
				break;
			case SLOWER_GUY_WINS:
				if (self->get_speed() <= hello->get_speed())
					eat(hello);
				break;
			}
		}
	}
}

void LifeForm::eat(SmartPointer<LifeForm> prey) {
	if (!is_alive) return;
	SmartPointer<LifeForm> self = SmartPointer<LifeForm>(this);
	double prey_energy = prey->energy;
	energy -= eat_cost_function();
	if (energy < min_energy) {
		die();
		return;
	}
	new Event(digestion_time, [self, prey_energy](void) {self->gain_energy(prey_energy); });
	prey->die();
}

void LifeForm::gain_energy(double prey_energy) {
	if (!is_alive) return;
	energy += eat_efficiency*prey_energy;
}

void LifeForm::age(void){
	if (!is_alive) return;
	energy -= age_penalty;
	if (energy < min_energy) {
		die();
		return;
	}
	SmartPointer<LifeForm> self = SmartPointer<LifeForm>(this);
	new Event(age_frequency, [self](void) {self->age(); });
}

void LifeForm::reproduce(SmartPointer<LifeForm> child){
	//insert child to quadtree
	//cout << "reproduce" << endl;
	if (!is_alive) return;
	double rp_time_delta = Event::now() - reproduce_time;
	if (rp_time_delta < min_reproduce_time) return;
	SmartPointer<LifeForm> nearest;
	reproduce_time = Event::now();

	do {
		double spawn_distance = drand48()*reproduce_dist;
		double spawn_rad = drand48() * 2.0 * M_PI;
		Point delta(spawn_distance*cos(spawn_rad), spawn_distance*sin(spawn_rad));
		child->pos = pos + delta;
		nearest = space.closest(child->pos);
	} while (nearest && nearest->position().distance(child->pos)
		<= encounter_distance||space.is_occupied(child->pos));

	child->start_point = child->pos;
	space.insert(child, child->pos, [child]() { child->region_resize(); });
	child->is_alive = true;
	child->energy = (energy / 2)*(1 - reproduce_cost);
	energy = child->energy;
	if (energy < min_energy) {
		die();
		child->die();
		return;
	}
}

ObjList LifeForm::perceive(double dist) {
	//cout << "perceive" << endl;
	ObjList objlist;
	if (!is_alive) return objlist;
	if (dist < min_perceive_range) dist = min_perceive_range;
	if (dist > max_perceive_range) dist = max_perceive_range;
	energy -= perceive_cost(dist);
	if (energy < min_energy) {
		die();
		return objlist;
	}
	std::vector<SmartPointer<LifeForm>> observed_objs = space.nearby(pos,dist);
	for (int i = 0; i < observed_objs.size(); i++)
		objlist.push_back(info_about_them(observed_objs[i]));
	return objlist;
}


