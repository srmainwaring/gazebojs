/*
 * Copyright 2013 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/
#include "GazeboPubSub.hh"


#include <algorithm>
#include <boost/thread.hpp>

#include "OgreMaterialParser.hh"
#include "json2pb.h"

#include <gazebo/common/SystemPaths.hh>

#define MAX_NUM_MSG_SIZE 1000

using namespace gzscript;
using namespace gzweb;
using namespace std;

static bool trace = false;

void Trace(const char *m)
{
  if(trace) cout << "[GazeboPubSub] "  << m << endl;

}

/////////////////////////////////////////////////
class MocType : public gazebo::msgs::WorldControl
{
  public: std::string GetTypeName() const {return globalName;}
  public: static std::string globalName;

};

std::string MocType::globalName;

/////////////////////////////////////////////////
GzPublisher::GzPublisher(gazebo::transport::NodePtr &_node, const char* _type, const char* _topic)
          :Publisher(_type, _topic)
{
  MocType::globalName = _type;
  this->pub = _node->Advertise< MocType >(this->topic);
}

/////////////////////////////////////////////////
GzPublisher::~GzPublisher()
{
}

/////////////////////////////////////////////////
void GzPublisher::Publish(const char *msg)
{
   // create a protobuf message
   boost::shared_ptr<google::protobuf::Message> pb = gazebo::msgs::MsgFactory::NewMsg(this->type);
   // fill it with json data
   json2pb(*pb, msg, strlen(msg) );
   // publish it
   this->pub->Publish( *(pb.get()) );
   // pb auto cleans up
}

/////////////////////////////////////////////////
void GazeboPubSub::Advertise(const char *_type, const char *_topic)
{
  MocType::globalName = _type;
  this->advertisePub = this->node->Advertise< MocType >(_topic);
}

/////////////////////////////////////////////////
GzSubscriber::GzSubscriber(gazebo::transport::NodePtr &_node, const char* _type, const char* _topic, bool _latch)
          :Subscriber(_type, _topic, _latch)
{
    Trace("GzSubscriber::GzSubscriber ");
    Trace(_topic);

    string t(_topic);
    this->sub = _node->Subscribe(t,
       &GzSubscriber::GzCallback, this, _latch);
}

/////////////////////////////////////////////////
void GzSubscriber::GzCallback(const string &_msg)
{
  Trace("GzCallback");
  // make an empty protobuf
  boost::shared_ptr<google::protobuf::Message> pb = gazebo::msgs::MsgFactory::NewMsg(this->type);
  // load it with the gazebo data
  pb->ParseFromString(_msg);
  // translate it to json
  const google::protobuf::Message& cpb = *(pb.get());
  string json = pb2json( cpb );
  // send it to the script engine
  this->Callback(json.c_str());
  // pb auto cleans up
}

/////////////////////////////////////////////////
GzSubscriber::~GzSubscriber()
{
  // clean up sub
  Trace( "GzSubscriber::GzCallback");

  // unsubscribe has a problem in Gazebo (Issue #602)
  // this->sub->Unsubscribe();

  // so instead, we'll simply reset
  this->sub.reset();
}

/////////////////////////////////////////////////
Publisher *GazeboPubSub::CreatePublisher(const char* _type, const char *_topic)
{
  Publisher *pub = new GzPublisher(this->node, _type, _topic);
  return pub;
}

/////////////////////////////////////////////////
vector<string> GazeboPubSub::GetMaterials()
{

  vector<string> v;

  std::list<std::string> paths = gazebo::common::SystemPaths::Instance()->GetModelPaths();
  for(std::list<std::string>::iterator it= paths.begin(); it != paths.end(); it++)
  {
    string path = *it;
    this->materialParser->Load(path);
    string mats =  this->materialParser->GetMaterialAsJson();
    v.push_back(mats);
  }
  return v;
}

/////////////////////////////////////////////////
GazeboPubSub::GazeboPubSub()
{
  gazebo::transport::init();
  gazebo::transport::run();

  // Create our node for communication
  this->node.reset(new gazebo::transport::Node());
  this->node->Init();

  // Gazebo topics
  string worldControlTopic = "~/world_control";

  // For controling world
  this->worldControlPub =
      this->node->Advertise<gazebo::msgs::WorldControl>(
      worldControlTopic);

  this->materialParser = new gzweb::OgreMaterialParser();
}

/////////////////////////////////////////////////
GazeboPubSub::~GazeboPubSub()
{
  Trace("GazeboPubSub::~GazeboPubSub()");

  this->worldControlPub.reset();

  delete this->materialParser;

  this->node->Fini();
  this->node.reset();
}

/////////////////////////////////////////////////
std::string GazeboPubSub::GetSdfVer(){
  return SDF_VERSION;
}
