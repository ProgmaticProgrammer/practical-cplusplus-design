// Copyright 2016 Adam B. Singer
// Contact: PracticalDesignBook@gmail.com
//
// This file is part of pdCalc.
//
// pdCalc is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// pdCalc is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with pdCalc; if not, see <http://www.gnu.org/licenses/>.

#include "PublisherObserverTest.h"

using std::set;
using std::string;
using std::vector;
using std::shared_ptr;
using std::make_shared;

#include <string>

class StringEventData : public pdCalc::EventData
{
public:
    StringEventData(std::string s) : str_(s) { }
    const std::string& state() const { return str_; }

private:
    std::string str_;
};

class ConcretePublisher : private pdCalc::Publisher
{
public:
    ConcretePublisher();
    ~ConcretePublisher();

    void registerSingleEvent(const std::string& name = "singleEvent");
    std::set<std::string> listEvents() const;

    using pdCalc::Publisher::attach;
    using pdCalc::Publisher::listEventObservers;
    using pdCalc::Publisher::raise;
    using pdCalc::Publisher::detach;

private:

    void registerMultipleEvents();

    ConcretePublisher(const ConcretePublisher&) = delete;
    ConcretePublisher& operator=(const ConcretePublisher&) = delete;
    ConcretePublisher(ConcretePublisher&&) = delete;
    ConcretePublisher& operator=(ConcretePublisher&&) = delete;
};

class ConcreteObserver : public pdCalc::Observer
{
public:
    ConcreteObserver(const std::string&);
    ~ConcreteObserver();

    bool notified() const { return notified_; }
    void setNotified(bool b) { notified_ = b; }
    const std::string& state() const { return state_; }

private:
    void notifyImpl(std::shared_ptr<pdCalc::EventData>);

    ConcreteObserver(const ConcreteObserver&) = delete;
    ConcreteObserver& operator=(const ConcreteObserver&) = delete;
    ConcreteObserver(ConcreteObserver&&) = delete;
    ConcreteObserver& operator=(ConcreteObserver&&) = delete;

    bool notified_;
    std::string state_;
};

ConcretePublisher::ConcretePublisher()
{ 
    registerSingleEvent();
    registerMultipleEvents();
}

ConcretePublisher::~ConcretePublisher()
{ }

void ConcretePublisher::registerSingleEvent(const string& name)
{
    registerEvent(name);
    
    return;
}

void ConcretePublisher::registerMultipleEvents()
{
    vector<string> events = { "multipleEventOne", "multipleEventTwo"};
    registerEvents(events);
    return;
}

set<string> ConcretePublisher::listEvents() const
{
    return pdCalc::Publisher::listEvents();
}

ConcreteObserver::ConcreteObserver(const std::string& name)
: Observer{name}
, notified_{false}
, state_{""}
{
}

ConcreteObserver::~ConcreteObserver()
{ }

void ConcreteObserver::notifyImpl(std::shared_ptr<pdCalc::EventData> data)
{
    notified_ = true;
    std::shared_ptr<StringEventData> p = std::dynamic_pointer_cast<StringEventData>(data);
    if(p) state_ = p->state();
    return;
}

void PublisherObserverTest::init()
{
    publisher_ = new ConcretePublisher;
}

void PublisherObserverTest::cleanup()
{
    delete publisher_;
}

void PublisherObserverTest::testRegister()
{
    // note that register calls appear in Publisher constructor
    auto events = publisher_->listEvents();
    auto end = events.end();
    
    QVERIFY(events.find("singleEvent") != end);
    QVERIFY(events.find("multipleEventOne") != end);
    QVERIFY(events.find("multipleEventTwo") != end);
    
    return;
}

void PublisherObserverTest::testRegisterDuplicate()
{
    try
    { 
        publisher_->registerSingleEvent("singleEvent");
    }
    catch(pdCalc::Exception& e)
    {
        QCOMPARE(e.what(), string{"Event already registered"});
    }
}

void PublisherObserverTest::testAttachObservers()
{
    // attach an observer to an event
    publisher_->attach("singleEvent", make_shared<ConcreteObserver>("singleEventObserver"));

    // attach it again
    try
    {
        publisher_->attach("singleEvent", make_shared<ConcreteObserver>("singleEventObserver"));
    }
    catch(pdCalc::Exception& e)
    {
        QCOMPARE(e.what(), string{"Observer already attached to publisher"});
    }

    // check that an event with no observers is empty but present
    auto obsSetMultipleEventOne = publisher_->listEventObservers("multipleEventOne");
    QVERIFY(obsSetMultipleEventOne.size() == 0);

    // check that existing observers are present and nonexisting ones are empty
    auto obsSet = publisher_->listEventObservers("singleEvent");
    auto end = obsSet.end();
    QVERIFY(obsSet.find("singleEventObserver") != end);
    QCOMPARE(obsSet.find("madeUpEvent"), end);

    // attach multiple observers to one event
    publisher_->attach("multipleEventOne", make_shared<ConcreteObserver>("observerOne"));
    publisher_->attach("multipleEventOne", make_shared<ConcreteObserver>("observerTwo"));

    obsSet = publisher_->listEventObservers("multipleEventOne");
    end = obsSet.end();
    QVERIFY(obsSet.find("observerOne") != end);
    QVERIFY(obsSet.find("observerTwo") != end);
    QCOMPARE(obsSet.find("madeUpEvent"), end);

    return;
}

void PublisherObserverTest::testRaiseEvent()
{
    // bad form (holding raw ptr to shared_ptr) for real code--ok for testing
    auto p1 = new ConcreteObserver{"observerOne"};
    auto p2 = new ConcreteObserver{"observerTwo"};

    publisher_->attach("multipleEventOne", shared_ptr<pdCalc::Observer>{p1});
    publisher_->attach("multipleEventOne", shared_ptr<pdCalc::Observer>{p2});

    // check that event has not been raised
    QCOMPARE(p1->notified(), false);
    QCOMPARE(p1->notified(), false);

    // check that raising a not observed event did not trigger observation
    publisher_->raise("multipleEventTwo", std::make_shared<pdCalc::EventData>());

    QCOMPARE(p1->notified(), false);
    QCOMPARE(p1->notified(), false);

    // check that raising observed event was indeed observed
    publisher_->raise("multipleEventOne", std::make_shared<pdCalc::EventData>());

    QCOMPARE(p1->notified(), true);
    QCOMPARE(p1->notified(), true);

    return;
}

void PublisherObserverTest::testDetachObservers()
{
    // bad form (holding raw ptr to shared_ptr) for real code--ok for testing
    //auto p1 = std::make_shared<ConcreteObserver>("observerOne");
//    shared_ptr<pdCalc::Observer> p1{ raw };

    publisher_->attach("multipleEventOne", std::make_shared<ConcreteObserver>("observerOne"));
    publisher_->attach("multipleEventOne", std::make_shared<ConcreteObserver>("observerTwo"));

    // check event has two observers
    set<string> obsSetMultipleEventOne = publisher_->listEventObservers("multipleEventOne");
    QVERIFY(obsSetMultipleEventOne.size() == 2);

    // detach event
    //QVERIFY(p1 == nullptr);
    auto p1 = publisher_->detach("multipleEventOne", "observerOne");

    // check the correct observer remains
    obsSetMultipleEventOne = publisher_->listEventObservers("multipleEventOne");
    auto end = obsSetMultipleEventOne.end();
    QVERIFY( obsSetMultipleEventOne.find("observerTwo") != end);
    QCOMPARE( obsSetMultipleEventOne.find("observerOne"), end);

    // check the the returned pointer exists and is the correct one
    QVERIFY( p1 != nullptr );
    //QCOMPARE( p1.get(), raw );

    // check error condition on detaching something not being observed
    try
    {
        publisher_->detach("madeUpEvent", "observerTwo");
    }
    catch(pdCalc::Exception& e)
    {
        QCOMPARE(e.what(), string{"Publisher does not support event 'madeUpEvent'"});
    }

    try
    {
        publisher_->detach("multipleEventOne", "observerOne");
    }
    catch(pdCalc::Exception& e)
    {
        QCOMPARE(e.what(), string{"Cannot detach observer because observer not found"});
    }

    return;
}

void PublisherObserverTest::testGetState()
{
    auto raw = new ConcreteObserver{"observer"};
    QCOMPARE(raw->state(), string{""});

    publisher_->attach("singleEvent", shared_ptr<pdCalc::Observer>{raw});
    publisher_->raise( "singleEvent", std::make_shared<StringEventData>( "sample state" ) );

    QCOMPARE(raw->state(), string{"sample state"});

    return;
}
