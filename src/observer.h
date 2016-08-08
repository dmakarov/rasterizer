/**
   \file observer.h

   Created by Dmitri Makarov on 16-08-07.
   Copyright © 2016 Dmitri Makarov. All rights reserved.
 */

#ifndef observer_h
#define observer_h

#include <algorithm>
#include <list>

class Subject;

class Observer {
public:
  virtual ~Observer() {}
  virtual void update(Subject& subject) = 0;

protected:
  Observer() {}
};

class Subject {
public:
  virtual ~Subject() {}

  virtual void attach(Observer* o) {
    observers.push_back(o);
  }

  virtual void detach(Observer* o) {
    auto E = observers.end();
    auto it = std::find(observers.begin(), E, o);
    if (it != E) {
      observers.erase(it);
    }
  }

  virtual void notify() {
    for (auto& it : observers) {
      it->update(*this);
    }
  }

protected:
  Subject() {}

private:
  std::list<Observer*> observers;
};

#endif /* observer_h */

// Local Variables:
// mode: c++
// End:
