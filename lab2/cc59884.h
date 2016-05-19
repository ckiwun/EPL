#if !(_cc59884_h)
#define _cc59884_h 1

#include <memory>
#include "LifeForm.h"
#include "Init.h"

class cc59884 : public LifeForm {
protected:
	static void initialize(void);
	void spawn(void);
	void myhunt(void);
	void startup(void);
	Event* hunt_event;
	int update_turn_around;
public:
	cc59884(void);
	~cc59884(void);
	Color my_color(void) const;   // defines LifeForm::my_color
	static SmartPointer<LifeForm> create(void);
	virtual std::string species_name(void) const;
	virtual Action encounter(const ObjInfo&);
	friend class Initializer<cc59884>;
};


#endif /* !(_cc59884_h) */
