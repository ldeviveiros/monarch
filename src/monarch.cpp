#include "monarch.hpp"

Monarch::Monarch(){

  //empty, just here to make the compiler throw a
  //fit if someone tries to call a default constructor

}

//read constructor
Monarch::Monarch(std::string filename){
	MonarchIO *eggPTR = new MonarchIO(std::string filename);
  	MonarchEgg *egg BreakEgg(MonarchIO *eggPTR);
  	MonarchEgg *egg = ParseHeader(MonarchIO *,MonarchEgg *);
  }

//write constructor
Monarch::Monarch(int sample_length,int sample_rate,std::string filename){

  //fix me

}

//simulation constructor
Monarch::Monarch(bool signal,int sample_length,int sample_rate,std::string filename){

  //fix me

}

Monarch::~Monarch(){

  //fix me

}

Monarch::mPrintHeader() {


}

Monarch::mPrintNextEvent() {


}

Monarch::mPrintNextRecord() {


}
